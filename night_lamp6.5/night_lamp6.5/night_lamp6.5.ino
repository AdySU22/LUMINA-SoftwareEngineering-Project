/* ESP32-S2 Night Lamp
   - NeoPixel RGB ring
   - High-power white LED
   - Web UI (SoftAP)
   - Alarms with configurable sunrise ramp + alarm type (LED / beeper)
   - Time from browser (no RTC)
   - Default state in flash
   - Party mode + music sync (sound sensor on pin 6)
   - Alarm ramp test from Advanced Settings

   Pins (change here if needed):
     rgbPin    = 3  (WS2812 / NeoPixel ring)
     boostPin  = 1  (boost converter enable for HP LED)
     pwmPin    = 2  (HP LED PWM, analogWrite)
     buttonPin = 4  (state / cancel button, active LOW)
     soundPin  = 6  (digital sound sensor, HIGH on beat)
     buzzerPin = 7  (active buzzer, HIGH = ON)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <time.h>

#include "index_html.h"
#include "alarms_html.h"
#include "style_css.h"
#include "script_js.h"

// ---------------- Pins ----------------
const int rgbPin    = 3;
const int boostPin  = 1;
const int pwmPin    = 2;
const int buttonPin = 4;
const int soundPin  = 6;
const int buzzerPin = 7;

// ---------------- NeoPixel ----------------
const int numPixels = 9;
Adafruit_NeoPixel pixels(numPixels, rgbPin, NEO_GRB + NEO_KHZ800);

// ---------------- Web server ----------------
WebServer server(80);

// ---------------- Internal time (no RTC) ----------------
volatile uint32_t baseEpoch   = 0;  // UTC epoch seconds at last sync
volatile uint32_t baseMillis  = 0;  // millis() at last sync
volatile int32_t  tzOffsetMin = 0;  // minutes; browser getTimezoneOffset()

uint32_t nowEpochUTC() {
  if (baseEpoch == 0) return 0;
  uint32_t elapsed = (millis() - baseMillis) / 1000;
  return baseEpoch + elapsed;
}

uint32_t nowEpochLocal() {
  uint32_t utc = nowEpochUTC();
  if (utc == 0) return 0;
  if (tzOffsetMin == 0) return utc;
  return utc - (uint32_t)(tzOffsetMin * 60);
}

// ---------------- State machine ----------------
volatile bool isrButtonPressed = false;
bool webOverride   = false;
int  currentState  = 0;   // 0..4
int  savedState    = 0;

// Web RGB values (from UI)
uint8_t webR = 255, webG = 255, webB = 255, webBri = 255;
uint8_t webHighPower = 0; // 0..255

// ---------------- Alarm list ----------------
struct AlarmItem {
  uint32_t id;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  daysMask;     // bit0=Sun..bit6=Sat; 0 = everyday
  bool     enabled;
  uint32_t lastFireMin;  // kept for compatibility; not heavily used now
};

static const int MAX_ALARMS = 8;
AlarmItem alarms[MAX_ALARMS];
int alarmCount = 0;

// ---------------- Alarm behavior & ramp ----------------
Preferences prefs;

bool    defaultSaved    = false;
uint8_t defaultStateNVS = 0;
uint8_t defaultR   = 255;
uint8_t defaultG   = 255;
uint8_t defaultB   = 255;
uint8_t defaultBri = 255;
uint8_t defaultHP  = 0;

// Alarm configuration (stored in flash, controlled from Advanced Settings)
uint32_t alarmRampLeadSec = 600;   // seconds before alarm time to start ramp (default 10 min)
bool     alarmUseLED      = true;  // whether HP LED participates in alarm
bool     alarmUseBuzzer   = false; // whether buzzer rings at alarm time
uint32_t alarmTimeoutSec  = 600;   // seconds after alarm time to auto-stop (default 10 min)

// Active alarm / test state
bool     alarmActive          = false;  // true if either real alarm or test ramp running
bool     alarmIsTest          = false;  // true if this is ramp test, false if real alarm
bool     beepStarted          = false;  // for real alarms only
uint32_t alarmStartMs         = 0;      // used for test ramp
uint32_t alarmTestDurationMs  = 0;      // used for test ramp
uint32_t sunriseBeepEpochLocal  = 0;    // local epoch when alarm time is reached
uint32_t sunriseStartEpochLocal = 0;    // local epoch when ramp should be 0 -> full

// ---------------- Party / Music Sync ----------------
bool    partyEnabled      = false;
bool    musicSyncEnabled  = false;
uint8_t partyEffect       = 0;   // 0=Fade,1=Strobe,2=Pulse
uint8_t partySpeed        = 50;  // 0..100
uint8_t partyBrightness   = 80;  // 0..100
uint8_t partyColorMode    = 0;   // 0=RGB wheel,1=Random,2=Single color
uint8_t partySingleR      = 255;
uint8_t partySingleG      = 0;
uint8_t partySingleB      = 0;

uint32_t lastPartyStepMs  = 0;
uint16_t partyStep        = 0;
bool     lastSoundLevel   = false;

// ---------------- Forward declarations ----------------
void applyOutputs();
void applyStateOutputs();
void applyWebOutputs();
void checkAlarms();
void startSunrise(uint32_t alarmEpochLocal);
void updateRealAlarm();
void updateTestRamp();
void stopAlarm();
void runPartyMode();
uint32_t colorWheel(uint8_t pos);
uint8_t gamma8(uint8_t x);

void loadDefaultFromNVS();
void saveDefaultToNVS();
void loadAlarmsFromNVS();
void saveAlarmsToNVS();
void loadAlarmSettingsFromNVS();
void saveAlarmSettingsToNVS();

uint8_t daysMaskFromString(const String& s);
bool    isTodayEnabled(uint8_t mask, int wday);

// ---------------- Helpers ----------------
void IRAM_ATTR handleButtonISR() {
  static unsigned long lastInterruptTime = 0;
  unsigned long t = millis();
  if (t - lastInterruptTime > 200) {
    isrButtonPressed = true;
  }
  lastInterruptTime = t;
}

void hpWrite(uint8_t duty) {
  uint8_t hw = gamma8(duty);
  if (hw == 0) {
    analogWrite(pwmPin, 0);
    digitalWrite(boostPin, LOW);
  } else {
    digitalWrite(boostPin, HIGH);
    analogWrite(pwmPin, hw);
  }
}

// Simple gamma curve (~perceptual / "log") for 0..255 -> 0..255
uint8_t gamma8(uint8_t x) {
  uint16_t v = (uint16_t)x * (uint16_t)x; // gamma ~2.0
  v = (v + 254) / 255;                    // round
  if (v > 255) v = 255;
  return (uint8_t)v;
}

// SMTWTFS -> bitmask (0=Sun..6=Sat). Ambiguous letters mapped to both days.
uint8_t daysMaskFromString(const String& s) {
  uint8_t mask = 0;
  int countS = 0;
  for (size_t i = 0; i < s.length(); ++i) {
    if (s[i] == 'S') countS++;
  }
  if (countS == 1) mask |= (1 << 0);            // S -> Sun
  else if (countS >= 2) mask |= (1 << 0) | (1 << 6); // S -> Sun & Sat
  if (s.indexOf('M') != -1) mask |= (1 << 1);
  if (s.indexOf('T') != -1) mask |= (1 << 2) | (1 << 4); // T -> Tue & Thu
  if (s.indexOf('W') != -1) mask |= (1 << 3);
  if (s.indexOf('F') != -1) mask |= (1 << 5);
  return mask;
}

bool isTodayEnabled(uint8_t mask, int wday) { // 0=Sun..6=Sat
  if (mask == 0) return true;
  if (wday < 0) return false;
  if (wday > 6) wday %= 7;
  return (mask & (1 << wday)) != 0;
}

// ---------------- NVS: defaults ----------------
void loadDefaultFromNVS() {
  prefs.begin("lamp", true);
  defaultSaved    = prefs.getBool("hasDef", false);
  defaultStateNVS = prefs.getUChar("defState", 1);
  defaultR        = prefs.getUChar("defR",   255);
  defaultG        = prefs.getUChar("defG",   180);
  defaultB        = prefs.getUChar("defB",   100);
  defaultBri      = prefs.getUChar("defBri", 255);
  defaultHP       = prefs.getUChar("defHP",  0);
  prefs.end();
}

void saveDefaultToNVS() {
  prefs.begin("lamp", false);
  prefs.putBool("hasDef", true);
  prefs.putUChar("defState", (uint8_t)currentState);
  prefs.putUChar("defR",   webR);
  prefs.putUChar("defG",   webG);
  prefs.putUChar("defB",   webB);
  prefs.putUChar("defBri", webBri);
  prefs.putUChar("defHP",  webHighPower);
  prefs.end();

  defaultSaved    = true;
  defaultStateNVS = (uint8_t)currentState;
  defaultR        = webR;
  defaultG        = webG;
  defaultB        = webB;
  defaultBri      = webBri;
  defaultHP       = webHighPower;
}

// ---------------- NVS: alarms ----------------
void saveAlarmsToNVS() {
  prefs.begin("lamp", false);
  prefs.putUChar("alarmCount", (uint8_t)alarmCount);
  prefs.putBytes("alarms", alarms, sizeof(AlarmItem) * MAX_ALARMS);
  prefs.end();
}

void loadAlarmsFromNVS() {
  prefs.begin("lamp", true);
  uint8_t count = prefs.getUChar("alarmCount", 0);
  size_t storedSize = prefs.getBytesLength("alarms");
  if (storedSize == sizeof(AlarmItem) * (size_t)MAX_ALARMS) {
    prefs.getBytes("alarms", alarms, storedSize);
    alarmCount = min((int)count, MAX_ALARMS);
  } else {
    alarmCount = 0;
  }
  prefs.end();
  for (int i = 0; i < alarmCount; ++i) {
    alarms[i].lastFireMin = 0;
  }
}

// ---------------- NVS: alarm settings (ramp + type) ----------------
void loadAlarmSettingsFromNVS() {
  prefs.begin("lamp", true);
  alarmRampLeadSec = prefs.getULong("alarmLead", 600);
  alarmUseLED      = prefs.getBool("alarmLED", true);
  alarmUseBuzzer   = prefs.getBool("alarmBuzz", false);
  alarmTimeoutSec  = prefs.getULong("alarmTimeout", 600);
  prefs.end();

  if (alarmRampLeadSec < 10)   alarmRampLeadSec = 10;
  if (alarmRampLeadSec > 7200) alarmRampLeadSec = 7200; // cap at 2 hours
  if (alarmTimeoutSec < 60)    alarmTimeoutSec  = 60;   // 1 minute minimum
  if (alarmTimeoutSec > 7200)  alarmTimeoutSec  = 7200; // cap at 2 hours
}

void saveAlarmSettingsToNVS() {
  prefs.begin("lamp", false);
  prefs.putULong("alarmLead", alarmRampLeadSec);
  prefs.putBool("alarmLED", alarmUseLED);
  prefs.putBool("alarmBuzz", alarmUseBuzzer);
   prefs.putULong("alarmTimeout", alarmTimeoutSec);
  prefs.end();
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(boostPin, OUTPUT);
  digitalWrite(boostPin, LOW);

  pinMode(pwmPin, OUTPUT);
  analogWrite(pwmPin, 0);

  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonISR, FALLING);

  pinMode(soundPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pixels.begin();
  pixels.setBrightness(255);
  pixels.clear();
  pixels.show();

  loadDefaultFromNVS();
  loadAlarmsFromNVS();
  loadAlarmSettingsFromNVS();

  Serial.printf("Default saved: %s, state=%u\n", defaultSaved ? "yes" : "no", (unsigned)defaultStateNVS);
  Serial.printf("Alarms loaded: %d\n", alarmCount);
  Serial.printf("Alarm ramp lead: %lu s, timeout: %lu s, LED=%d, buzzer=%d\n",
                (unsigned long)alarmRampLeadSec,
                (unsigned long)alarmTimeoutSec,
                alarmUseLED, alarmUseBuzzer);

  // Soft AP
  const char* apName = "lumina_Lamp";
  const char* apPass = "luminalamp";
  WiFi.softAP(apName, apPass);
  Serial.print("AP SSID: "); Serial.print(apName);
  Serial.print("  password: "); Serial.println(apPass);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Pages
  server.on("/",            HTTP_GET, [](){ server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/index.html",  HTTP_GET, [](){ server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/alarms.html", HTTP_GET, [](){ server.send_P(200, "text/html", ALARMS_HTML); });
  server.on("/style.css",   HTTP_GET, [](){ server.send_P(200, "text/css",  STYLE_CSS); });
  server.on("/script.js",   HTTP_GET, [](){ server.send_P(200, "application/javascript", SCRIPT_JS); });

  // ---- RGB + HP control ----
  server.on("/setrgb", HTTP_GET, [](){
    uint16_t r   = server.hasArg("r")   ? server.arg("r").toInt()   : 0;
    uint16_t g   = server.hasArg("g")   ? server.arg("g").toInt()   : 0;
    uint16_t b   = server.hasArg("b")   ? server.arg("b").toInt()   : 0;
    uint16_t bri = server.hasArg("bri") ? server.arg("bri").toInt() : 255;

    webR   = constrain(r,   0, 255);
    webG   = constrain(g,   0, 255);
    webB   = constrain(b,   0, 255);
    webBri = constrain(bri, 0, 255);

    // Direct RGB control cancels party & test, uses web override
    partyEnabled     = false;
    musicSyncEnabled = false;

    if (!webOverride) savedState = currentState;
    webOverride = true;
    alarmActive = false;
    alarmIsTest = false;
    stopAlarm();

    applyOutputs();
    server.send(200, "text/plain", "OK");
  });

  server.on("/sethp", HTTP_GET, [](){
    uint16_t val = server.hasArg("val") ? server.arg("val").toInt() : 0;
    webHighPower = constrain(val, 0, 255);

    partyEnabled     = false;
    musicSyncEnabled = false;

    if (!webOverride) savedState = currentState;
    webOverride = true;
    alarmActive = false;
    alarmIsTest = false;
    stopAlarm();

    applyOutputs();
    server.send(200, "text/plain", "OK");
  });

  // ---- Time sync ----
  server.on("/settime", HTTP_GET, [](){
    if (!server.hasArg("epoch")) {
      server.send(400, "text/plain", "epoch required");
      return;
    }
    uint32_t e = (uint32_t) strtoul(server.arg("epoch").c_str(), nullptr, 10);
    int32_t tz = 0;
    if (server.hasArg("tz")) {
      tz = (int32_t) strtol(server.arg("tz").c_str(), nullptr, 10);
    }

    noInterrupts();
    baseEpoch   = e;
    baseMillis  = millis();
    tzOffsetMin = tz;
    interrupts();

    Serial.print("Time synced. UTC epoch = "); Serial.print(baseEpoch);
    Serial.print("  tz offset (min) = "); Serial.println(tzOffsetMin);

    server.send(200, "text/plain", "OK");
  });

  // ---- Alarms API ----
  server.on("/alarms/list", HTTP_GET, [](){
    String json = "{\"alarms\":[";
    for (int i = 0; i < alarmCount; i++) {
      if (i) json += ",";
      json += "{";
      json += "\"id\":" + String(alarms[i].id) + ",";
      char buf[6];
      snprintf(buf, sizeof(buf), "%02u:%02u", alarms[i].hour, alarms[i].minute);
      json += "\"time\":\"" + String(buf) + "\",";
      json += "\"daysMask\":" + String(alarms[i].daysMask) + ",";
      json += "\"enabled\":" + String(alarms[i].enabled ? "true" : "false");
      json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/alarms/add", HTTP_GET, [](){
    if (alarmCount >= MAX_ALARMS) {
      server.send(400, "text/plain", "full");
      return;
    }
    if (!server.hasArg("time")) {
      server.send(400, "text/plain", "time HH:MM");
      return;
    }
    String t = server.arg("time");
    int h, m;
    if (sscanf(t.c_str(), "%d:%d", &h, &m) != 2) {
      server.send(400, "text/plain", "bad time");
      return;
    }

    String days = server.hasArg("days") ? server.arg("days") : "";
    bool enabled = server.hasArg("enabled") ? (server.arg("enabled").toInt() != 0) : true;

    AlarmItem a = {};
    a.id       = millis() ^ random(0xFFFF);
    a.hour     = constrain(h, 0, 23);
    a.minute   = constrain(m, 0, 59);
    a.daysMask = daysMaskFromString(days);
    a.enabled  = enabled;
    a.lastFireMin = 0;

    alarms[alarmCount++] = a;
    saveAlarmsToNVS();

    server.send(200, "text/plain", String(a.id));
  });

  server.on("/alarms/toggle", HTTP_GET, [](){
    if (!server.hasArg("id") || !server.hasArg("enabled")) {
      server.send(400, "text/plain", "id & enabled");
      return;
    }
    uint32_t id = (uint32_t) strtoul(server.arg("id").c_str(), nullptr, 10);
    bool en = server.arg("enabled").toInt() != 0;
    for (int i = 0; i < alarmCount; i++) {
      if (alarms[i].id == id) {
        alarms[i].enabled = en;
        break;
      }
    }
    saveAlarmsToNVS();
    server.send(200, "text/plain", "OK");
  });

  server.on("/alarms/delete", HTTP_GET, [](){
    if (!server.hasArg("id")) {
      server.send(400, "text/plain", "id");
      return;
    }
    uint32_t id = (uint32_t) strtoul(server.arg("id").c_str(), nullptr, 10);
    for (int i = 0; i < alarmCount; i++) {
      if (alarms[i].id == id) {
        for (int j = i + 1; j < alarmCount; j++) {
          alarms[j - 1] = alarms[j];
        }
        alarmCount--;
        break;
      }
    }
    saveAlarmsToNVS();
    server.send(200, "text/plain", "OK");
  });

  // ---- Default state ----
  server.on("/default/save", HTTP_GET, [](){
    saveDefaultToNVS();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/default/apply", HTTP_GET, [](){
    if (!defaultSaved) {
      server.send(404, "application/json", "{\"ok\":false,\"reason\":\"no_default\"}");
      return;
    }
    loadDefaultFromNVS();

    webR         = defaultR;
    webG         = defaultG;
    webB         = defaultB;
    webBri       = defaultBri;
    webHighPower = defaultHP;

    partyEnabled     = false;
    musicSyncEnabled = false;

    if (!webOverride) savedState = currentState;
    webOverride = true;

    stopAlarm();
    applyOutputs();
    server.send(200, "application/json", "{\"ok\":true,\"applied\":true}");
  });

  // ---- Party / Music Sync ----
  // /party/set?on=0/1&music=0/1&effect=0..2&speed=0..100&bri=0..100&mode=rgb|random|single&r=&g=&b=
  server.on("/party/set", HTTP_GET, [](){
    if (server.hasArg("on")) {
      partyEnabled = server.arg("on").toInt() != 0;
      if (partyEnabled) {
        webOverride = false;
      }
    }
    if (server.hasArg("music")) {
      musicSyncEnabled = server.arg("music").toInt() != 0;
    }
    if (server.hasArg("effect")) {
      int v = server.arg("effect").toInt();
      partyEffect = constrain(v, 0, 2);
    }
    if (server.hasArg("speed")) {
      int v = server.arg("speed").toInt();
      partySpeed = constrain(v, 0, 100);
    }
    if (server.hasArg("bri")) {
      int v = server.arg("bri").toInt();
      partyBrightness = constrain(v, 0, 100);
    }
    if (server.hasArg("mode")) {
      String m = server.arg("mode");
      m.toLowerCase();
      if (m == "rgb")      partyColorMode = 0;
      else if (m == "random") partyColorMode = 1;
      else if (m == "single") partyColorMode = 2;
    }
    if (server.hasArg("r")) partySingleR = constrain(server.arg("r").toInt(), 0, 255);
    if (server.hasArg("g")) partySingleG = constrain(server.arg("g").toInt(), 0, 255);
    if (server.hasArg("b")) partySingleB = constrain(server.arg("b").toInt(), 0, 255);

    if (!partyEnabled) {
      applyOutputs();
    }

    server.send(200, "application/json", "{\"ok\":true}");
  });

  // ---- Alarm configuration (ramp + type + timeout) ----
  server.on("/alarmcfg/get", HTTP_GET, [](){
    String json = "{";
    json += "\"ok\":true,";
    json += "\"leadSec\":" + String(alarmRampLeadSec) + ",";
    json += "\"useLED\":" + String(alarmUseLED ? "true" : "false") + ",";
    json += "\"useBuzzer\":" + String(alarmUseBuzzer ? "true" : "false") + ",";
    json += "\"timeoutSec\":" + String(alarmTimeoutSec);
    json += "}";
    server.send(200, "application/json", json);
  });

  // /alarmcfg/set?lead=seconds&led=0/1&buzz=0/1&timeout=seconds
  server.on("/alarmcfg/set", HTTP_GET, [](){
    if (server.hasArg("lead")) {
      uint32_t v = (uint32_t)server.arg("lead").toInt();
      v = constrain(v, 10u, 7200u);
      alarmRampLeadSec = v;
    }
    if (server.hasArg("led")) {
      alarmUseLED = (server.arg("led").toInt() != 0);
    }
    if (server.hasArg("buzz")) {
      alarmUseBuzzer = (server.arg("buzz").toInt() != 0);
    }
    if (server.hasArg("timeout")) {
      uint32_t v = (uint32_t)server.arg("timeout").toInt();
      v = constrain(v, 60u, 7200u);
      alarmTimeoutSec = v;
    }
    saveAlarmSettingsToNVS();

    String json = "{";
    json += "\"ok\":true,";
    json += "\"leadSec\":" + String(alarmRampLeadSec) + ",";
    json += "\"useLED\":" + String(alarmUseLED ? "true" : "false") + ",";
    json += "\"useBuzzer\":" + String(alarmUseBuzzer ? "true" : "false") + ",";
    json += "\"timeoutSec\":" + String(alarmTimeoutSec);
    json += "}";
    server.send(200, "application/json", json);
  });

  // ---- Alarm ramp test ----
  // /alarmtest/start?duration=seconds  (if omitted, uses alarmRampLeadSec)
  server.on("/alarmtest/start", HTTP_GET, [](){
    uint32_t dur = alarmRampLeadSec;
    if (server.hasArg("duration")) {
      uint32_t d = (uint32_t)server.arg("duration").toInt();
      if (d > 0) dur = d;
    }
    dur = constrain(dur, 5u, 7200u);

    alarmActive         = true;
    alarmIsTest         = true;
    beepStarted         = false;
    sunriseBeepEpochLocal  = 0;
    sunriseStartEpochLocal = 0;
    alarmStartMs        = millis();
    alarmTestDurationMs = dur * 1000UL;

    digitalWrite(buzzerPin, LOW);
    hpWrite(0);

    String json = "{\"ok\":true,\"duration\":" + String(dur) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/alarmtest/stop", HTTP_GET, [](){
    stopAlarm();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  // ---- Alarm reset (stop any active alarm or test) ----
  server.on("/alarm/reset", HTTP_GET, [](){
    stopAlarm();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  // ---- Status ----
  server.on("/status", HTTP_GET, [](){
    uint32_t epochUtc = nowEpochUTC();
    String json = "{";
    json += "\"epoch\":" + String(epochUtc) + ",";
    json += "\"tz\":" + String(tzOffsetMin) + ",";
    json += "\"defaultSaved\":" + String(defaultSaved ? "true" : "false") + ",";
    json += "\"defaultState\":" + String(defaultStateNVS) + ",";
    json += "\"state\":" + String(currentState) + ",";
    json += "\"override\":" + String(webOverride ? "true" : "false") + ",";
    json += "\"savedState\":" + String(savedState) + ",";
    json += "\"alarmActive\":" + String(alarmActive ? "true" : "false") + ",";

    json += "\"rgb\":{";
    json += "\"r\":" + String(webR) + ",";
    json += "\"g\":" + String(webG) + ",";
    json += "\"b\":" + String(webB) + ",";
    json += "\"bri\":" + String(webBri);
    json += "},";

    json += "\"hp\":" + String(webHighPower) + ",";

    // Alarm config
    json += "\"alarmCfg\":{";
    json += "\"leadSec\":" + String(alarmRampLeadSec) + ",";
    json += "\"useLED\":" + String(alarmUseLED ? "true" : "false") + ",";
    json += "\"useBuzzer\":" + String(alarmUseBuzzer ? "true" : "false") + ",";
    json += "\"timeoutSec\":" + String(alarmTimeoutSec);
    json += "},";

    // Party status
    json += "\"party\":{";
    json += "\"enabled\":" + String(partyEnabled ? "true" : "false") + ",";
    json += "\"music\":"   + String(musicSyncEnabled ? "true" : "false") + ",";
    json += "\"effect\":"  + String(partyEffect) + ",";
    json += "\"speed\":"   + String(partySpeed) + ",";
    json += "\"bri\":"     + String(partyBrightness) + ",";
    json += "\"mode\":"    + String((int)partyColorMode) + ",";
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", partySingleR, partySingleG, partySingleB);
    json += "\"color\":\"" + String(buf) + "\"";
    json += "}";

    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("HTTP server started.");

  // Initial state: use default color in state 1 if available
  currentState = defaultSaved ? 1 : 0;
  webOverride  = false;
  partyEnabled = false;
  musicSyncEnabled = false;
  alarmActive  = false;
  alarmIsTest  = false;
  beepStarted  = false;

  applyOutputs();
}

// ---------------- Loop ----------------
void loop() {
  server.handleClient();

  // Button behavior:
  // 1) If alarm/test active -> stop alarm
  // 2) Else if party active  -> turn off party
  // 3) Else if web override  -> cancel override (return to saved state)
  // 4) Else cycle physical states
  if (isrButtonPressed) {
    noInterrupts();
    isrButtonPressed = false;
    interrupts();

    if (alarmActive) {
      stopAlarm();
      Serial.println("Button: alarm/test cancelled.");
    } else if (partyEnabled) {
      partyEnabled = false;
      Serial.println("Button: party mode off.");
      applyOutputs();
    } else if (webOverride) {
      webOverride = false;
      currentState = savedState;
      Serial.println("Button: cancel web override, restore state " + String(currentState));
      applyOutputs();
    } else {
      currentState = (currentState + 1) % 5;
      Serial.println("Button: state -> " + String(currentState));
      applyOutputs();
    }
  }

  // Alarm scheduler
  checkAlarms();

  // Active alarm / test engine
  if (alarmActive) {
    if (alarmIsTest) {
      updateTestRamp();
    } else {
      updateRealAlarm();
    }
  } else {
    runPartyMode();
  }
}

// ---------------- Outputs ----------------
void applyOutputs() {
  if (alarmActive) return; // alarm/test owns HP LED

  if (partyEnabled) {
    // party engine owns pixels; HP is usually off or small strobe
    return;
  }

  if (webOverride) {
    applyWebOutputs();
  } else {
    applyStateOutputs();
  }
}

void applyStateOutputs() {
  switch (currentState) {
    case 0: // All off
      pixels.clear();
      pixels.show();
      hpWrite(0);
      break;

    case 1: { // RGB default color, HP off
      uint8_t r   = defaultSaved ? defaultR   : 255;
      uint8_t g   = defaultSaved ? defaultG   : 180;
      uint8_t b   = defaultSaved ? defaultB   : 100;
      uint8_t bri = defaultSaved ? defaultBri : 255;

      pixels.setBrightness(gamma8(bri));
      for (int i = 0; i < numPixels; i++) {
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
      pixels.show();
      hpWrite(0);

      webR   = r;
      webG   = g;
      webB   = b;
      webBri = bri;
      break;
    }

    case 2: // HP @10%, RGB off
      pixels.clear();
      pixels.show();
      hpWrite(26);
      break;

    case 3: // HP @50%
      pixels.clear();
      pixels.show();
      hpWrite(128);
      break;

    case 4: // HP @100%
      pixels.clear();
      pixels.show();
      hpWrite(255);
      break;

    default:
      pixels.clear();
      pixels.show();
      hpWrite(0);
      break;
  }

  if (currentState != 1) {
    pixels.setBrightness(255);
  }
}

void applyWebOutputs() {
  pixels.setBrightness(gamma8(webBri));
  for (int i = 0; i < numPixels; i++) {
    pixels.setPixelColor(i, pixels.Color(webR, webG, webB));
  }
  pixels.show();
  hpWrite(webHighPower);
}

// ---------------- Party engine ----------------
uint32_t colorWheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return pixels.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return pixels.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return pixels.Color(pos * 3, 255 - pos * 3, 0);
}

void runPartyMode() {
  if (!partyEnabled || alarmActive) return;

  uint32_t now = millis();

  uint32_t baseInterval = map(partySpeed, 0, 100, 700, 60);
  if (baseInterval < 20) baseInterval = 20;

  bool beat = false;
  int soundVal = digitalRead(soundPin);
  bool level = (soundVal == HIGH);
  if (musicSyncEnabled && level && !lastSoundLevel) {
    beat = true;
  }
  lastSoundLevel = level;

  bool timeForStep = false;
  if (musicSyncEnabled) {
    if (beat) {
      timeForStep = true;
    } else if (now - lastPartyStepMs >= baseInterval * 4) {
      timeForStep = true; // slow fallback
    }
  } else {
    if (now - lastPartyStepMs >= baseInterval) {
      timeForStep = true;
    }
  }

  if (!timeForStep) return;

  lastPartyStepMs = now;
  partyStep++;

  // Base color
  uint8_t baseR = partySingleR;
  uint8_t baseG = partySingleG;
  uint8_t baseB = partySingleB;

  if (partyColorMode == 0) {
    uint32_t col = colorWheel((partyStep * 5) & 0xFF);
    baseR = (col >> 16) & 0xFF;
    baseG = (col >> 8)  & 0xFF;
    baseB = col & 0xFF;
  } else if (partyColorMode == 1) {
    if (!musicSyncEnabled || beat) {
      baseR = (uint8_t)random(0, 256);
      baseG = (uint8_t)random(0, 256);
      baseB = (uint8_t)random(0, 256);
      partySingleR = baseR;
      partySingleG = baseG;
      partySingleB = baseB;
    } else {
      baseR = partySingleR;
      baseG = partySingleG;
      baseB = partySingleB;
    }
  } else {
    baseR = partySingleR;
    baseG = partySingleG;
    baseB = partySingleB;
  }

  uint8_t maxBri = map(partyBrightness, 0, 100, 0, 255);

  switch (partyEffect) {
    case 0: { // Fade
      uint16_t wavePos = (partyStep * 8) & 0x1FF;
      uint8_t wave = (wavePos < 256) ? wavePos : (511 - wavePos);
      uint16_t bri = (uint16_t)maxBri * wave / 255;
      pixels.setBrightness(gamma8((uint8_t)bri));
      for (int i = 0; i < numPixels; i++) {
        pixels.setPixelColor(i, pixels.Color(baseR, baseG, baseB));
      }
      hpWrite(0);
      break;
    }
    case 1: { // Strobe
      bool on = (partyStep % 2) == 0;
      uint8_t bri = on ? maxBri : 0;
      pixels.setBrightness(gamma8(bri));
      for (int i = 0; i < numPixels; i++) {
        pixels.setPixelColor(i, pixels.Color(baseR, baseG, baseB));
      }
      hpWrite(on ? 60 : 0);
      break;
    }
    case 2: { // Pulse chase
      pixels.setBrightness(gamma8(maxBri));
      int head = partyStep % numPixels;
      for (int i = 0; i < numPixels; i++) {
        int dist = abs(i - head);
        uint8_t scale = (dist == 0) ? 255 : (dist == 1 ? 120 : 30);
        uint8_t r = (uint16_t)baseR * scale / 255;
        uint8_t g = (uint16_t)baseG * scale / 255;
        uint8_t b = (uint16_t)baseB * scale / 255;
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
      hpWrite(0);
      break;
    }
    default:
      pixels.clear();
      hpWrite(0);
      break;
  }

  pixels.show();
}

// ---------------- Alarm scheduler ----------------
void checkAlarms() {
  if (alarmActive) return;
  if (baseEpoch == 0) return;

  uint32_t epochLocal = nowEpochLocal();
  if (epochLocal == 0) return;

  time_t tt = (time_t)epochLocal;
  struct tm tmlocal;
  localtime_r(&tt, &tmlocal);
  int wday = tmlocal.tm_wday;
  int nowSecInDay = tmlocal.tm_hour * 3600 + tmlocal.tm_min * 60 + tmlocal.tm_sec;

  uint32_t window = alarmRampLeadSec > 0 ? alarmRampLeadSec : 1;
  int32_t bestDiff = INT32_MAX;
  uint32_t bestAlarmEpoch = 0;

  for (int i = 0; i < alarmCount; i++) {
    AlarmItem &a = alarms[i];
    if (!a.enabled) continue;
    if (!isTodayEnabled(a.daysMask, wday)) continue;

    int alarmSecInDay = a.hour * 3600 + a.minute * 60;
    int32_t diff = alarmSecInDay - nowSecInDay; // seconds until alarm time TODAY

    if (diff < 0) continue;                      // already passed today
    if (diff > (int32_t)window) continue;        // not yet in ramp window
    if (diff < bestDiff) {
      bestDiff = diff;
      bestAlarmEpoch = epochLocal + diff;
    }
  }

  if (bestAlarmEpoch != 0) {
    startSunrise(bestAlarmEpoch);
  }
}

void startSunrise(uint32_t alarmEpochLocal) {
  alarmActive = true;
  alarmIsTest = false;
  beepStarted = false;

  sunriseBeepEpochLocal = alarmEpochLocal;
  if (alarmRampLeadSec == 0) {
    sunriseStartEpochLocal = sunriseBeepEpochLocal;
  } else {
    sunriseStartEpochLocal = sunriseBeepEpochLocal - alarmRampLeadSec;
  }

  Serial.print("Alarm: scheduling ramp, beep at local epoch ");
  Serial.println(sunriseBeepEpochLocal);
}

// Buzzer pattern helper: 0.5s ON, 0.5s OFF while active
void updateBuzzerPattern(bool active) {
  static bool     wasActive = false;
  static bool     state     = false;
  static uint32_t lastMs    = 0;

  if (!active) {
    wasActive = false;
    state     = false;
    digitalWrite(buzzerPin, LOW);
    return;
  }

  uint32_t now = millis();

  // On rising edge: start with buzzer ON immediately
  if (!wasActive) {
    wasActive = true;
    state     = true;
    lastMs    = now;
    digitalWrite(buzzerPin, HIGH);
    return;
  }

  if (now - lastMs >= 500) {
    lastMs = now;
    state  = !state;
    digitalWrite(buzzerPin, state ? HIGH : LOW);
  }
}

// Real alarm engine (ramp + buzzer)
void updateRealAlarm() {
  uint32_t epochLocal = nowEpochLocal();
  if (epochLocal == 0) return;

  // HP LED ramp
  if (alarmUseLED) {
    uint8_t duty = 0;
    if (alarmRampLeadSec == 0) {
      duty = (epochLocal >= sunriseBeepEpochLocal) ? 255 : 0;
    } else {
      if (epochLocal <= sunriseStartEpochLocal) {
        duty = 0;
      } else if (epochLocal >= sunriseBeepEpochLocal) {
        duty = 255;
      } else {
        uint32_t elapsed = epochLocal - sunriseStartEpochLocal;
        if (elapsed > alarmRampLeadSec) elapsed = alarmRampLeadSec;
        duty = (uint8_t)((elapsed * 255UL) / alarmRampLeadSec);
      }
    }
    hpWrite(duty);
  } else {
    hpWrite(0);
  }

  // Buzzer only at/after alarm time
  bool buzzerShouldBeActive = alarmUseBuzzer && (epochLocal >= sunriseBeepEpochLocal);
  if (buzzerShouldBeActive && !beepStarted) {
    beepStarted = true;
    Serial.println("Alarm: buzzer ON.");
  }
  updateBuzzerPattern(buzzerShouldBeActive);

  // Auto timeout: stop alarm after configured duration past alarm time
  if (alarmTimeoutSec > 0 && sunriseBeepEpochLocal > 0) {
    if (epochLocal >= sunriseBeepEpochLocal + alarmTimeoutSec) {
      Serial.println("Alarm: auto timeout reached, stopping alarm.");
      stopAlarm();
    }
  }
}

// Test ramp engine (visual + optional buzzer)
void updateTestRamp() {
  uint32_t nowMs = millis();
  uint32_t elapsed = nowMs - alarmStartMs;

  if (elapsed >= alarmTestDurationMs + 5000UL) {
    // ramp done + 5s hold -> stop
    stopAlarm();
    return;
  }

  bool atFull = (elapsed >= alarmTestDurationMs);

  // Optional buzzer during test: beep when ramp reaches full brightness
  bool buzzerShouldBeActive = alarmUseBuzzer && atFull;
  updateBuzzerPattern(buzzerShouldBeActive);

  if (!alarmUseLED) {
    hpWrite(0);
    return;
  }

  if (atFull) {
    hpWrite(255); // hold full brightness for a few seconds
  } else {
    uint8_t duty = (uint8_t)((elapsed * 255UL) / alarmTestDurationMs);
    hpWrite(duty);
  }
}

void stopAlarm() {
  alarmActive          = false;
  alarmIsTest          = false;
  beepStarted          = false;
  sunriseBeepEpochLocal  = 0;
  sunriseStartEpochLocal = 0;
  alarmStartMs         = 0;
  alarmTestDurationMs  = 0;
  hpWrite(0);
  updateBuzzerPattern(false);
  Serial.println("Alarm/Test: stopped.");
}
