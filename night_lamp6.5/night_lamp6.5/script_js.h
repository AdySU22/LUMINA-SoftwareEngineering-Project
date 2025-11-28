#pragma once 
const char SCRIPT_JS[] PROGMEM = R"rawliteral(
// ==== Helpers ====
function debounce(fn, wait){ let t; return (...a)=>{ clearTimeout(t); t=setTimeout(()=>fn(...a), wait); }; }
function hexToRgb(hex){ const v=hex.replace('#',''); return { r:parseInt(v.substr(0,2),16), g:parseInt(v.substr(2,2),16), b:parseInt(v.substr(4,2),16) }; }
function rgbToHex(r,g,b){ const h=n=>n.toString(16).padStart(2,'0'); return `#${h(r)}${h(g)}${h(b)}`.toUpperCase(); }
function pad2(n){ return n.toString().padStart(2,'0'); }

// ==== Device time (no RTC) ====
let deviceEpoch = null;   // UTC epoch from /status
let lastSyncMs  = 0;

function updateTimeBox(){
  if (deviceEpoch == null) return;
  const d = new Date(deviceEpoch*1000);  // Date takes UTC and displays local
  const hh = pad2(d.getHours()), mm = pad2(d.getMinutes()), ss = pad2(d.getSeconds());
  const timeStr = `${hh}:${mm}:${ss}`;
  const dateStr = d.toLocaleDateString(undefined, { weekday:'short', year:'numeric', month:'short', day:'numeric' });

  const elTime = document.getElementById('rtcTime');
  const elDate = document.getElementById('rtcDate');
  const elStat = document.getElementById('rtcStatus');

  if (elTime) elTime.textContent = timeStr;
  if (elDate) elDate.textContent = dateStr;
  if (elStat) elStat.textContent = 'Source: internal';
}

// ==== DOM ====
const colorPicker      = document.getElementById('colorPicker');
const lampCircle       = document.getElementById('lampCircle');
const lampColorText    = document.getElementById('lampColor');
const brightnessSlider = document.getElementById('brightness'); // RGB only
const hpLEDSlider      = document.getElementById('hpLED');      // HP LED only
const brightnessText   = document.getElementById('lampBrightness');
const syncBtn          = document.getElementById('syncTimeBtn');

// --- Find the existing "Set as default" button and inject "Set back to default"
let setDefaultBtn = document.getElementById('setDefaultBtn');
if (!setDefaultBtn) {
  // robust fallback: search by visible text
  const candidates = Array.from(document.querySelectorAll('button'));
  setDefaultBtn = candidates.find(b => (b.textContent||'').trim().toLowerCase() === 'set as default');
}
let applyDefaultBtn = document.getElementById('applyDefaultBtn');
if (!applyDefaultBtn && setDefaultBtn && setDefaultBtn.parentElement) {
  applyDefaultBtn = document.createElement('button');
  applyDefaultBtn.id = 'applyDefaultBtn';
  applyDefaultBtn.className = setDefaultBtn.className || 'default-btn';
  applyDefaultBtn.style.marginTop = '8px';
  applyDefaultBtn.textContent = 'Set back to default';
  setDefaultBtn.insertAdjacentElement('afterend', applyDefaultBtn);
}

// ==== Default state actions ====
async function saveDefault(){
  try{
    await fetch('/default/save', { cache:'no-store' });
    if (setDefaultBtn){ setDefaultBtn.textContent = 'Saved!'; setTimeout(()=> setDefaultBtn.textContent='Set as default', 1200); }
    await fetchStatus();
  }catch(_){}
}

async function applyDefault(){
  try{
    const r = await fetch('/default/apply', { cache:'no-store' });
    if (r.ok){
      if (applyDefaultBtn){ applyDefaultBtn.textContent = 'Restored!'; setTimeout(()=> applyDefaultBtn.textContent='Set back to default', 1200); }
      await fetchStatus();
    }
  }catch(_){}
}

if (setDefaultBtn) setDefaultBtn.addEventListener('click', saveDefault);
if (applyDefaultBtn) applyDefaultBtn.addEventListener('click', applyDefault);

// ==== RGB / HP bindings ====
async function sendRGB(){
  if (!colorPicker) return;
  const {r,g,b}=hexToRgb(colorPicker.value);
  const bri = brightnessSlider ? Math.round((parseInt(brightnessSlider.value||'100')/100)*255) : 255;
  try{ await fetch(`/setrgb?r=${r}&g=${g}&b=${b}&bri=${bri}`); }catch(_){}
}
async function sendHP(){
  if (!hpLEDSlider) return;
  const hp = Math.round((parseInt(hpLEDSlider.value||'0')/100)*255);
  try{ await fetch(`/sethp?val=${hp}`); }catch(_){}
}
const sendRGBdeb = debounce(sendRGB,180);
const sendHPdeb  = debounce(sendHP,180);

if (colorPicker && lampCircle) {
  colorPicker.addEventListener('input', ()=>{
    lampCircle.style.backgroundColor = colorPicker.value;
    if (lampColorText) lampColorText.textContent = colorPicker.value.toUpperCase();
    sendRGBdeb();
  });
}
if (brightnessSlider && brightnessText) {
  brightnessSlider.addEventListener('input', ()=>{
    const pct = parseInt(brightnessSlider.value||'100');
    brightnessText.textContent = `${pct}%`;
    if (lampCircle) lampCircle.style.opacity = 0.3 + (pct/100)*0.7;
    sendRGBdeb(); // only RGB brightness
  });
}
if (hpLEDSlider) {
  hpLEDSlider.addEventListener('input', ()=>{ sendHPdeb(); });
}

// ==== Time/status sync ====
async function fetchStatus(){
  try{
    const resp = await fetch('/status', { cache:'no-store' });
    const js = await resp.json();

    // device UTC epoch
    if (typeof js.epoch === 'number') {
      deviceEpoch = js.epoch;
      lastSyncMs  = Date.now();
      updateTimeBox();
    }

    // enable/disable "Set back to default" based on saved flag
    if (applyDefaultBtn){
      applyDefaultBtn.disabled = !js.defaultSaved;
      applyDefaultBtn.title = js.defaultSaved ? '' : 'No default saved yet';
    }

    // Color preview
    const hex = rgbToHex(js.rgb.r, js.rgb.g, js.rgb.b);
    if (colorPicker)   colorPicker.value = hex;
    if (lampCircle)    lampCircle.style.backgroundColor = hex;
    if (lampColorText) lampColorText.textContent = hex;

    // RGB brightness
    const rgbPct = Math.round(js.rgb.bri/2.55);
    if (brightnessSlider){
      brightnessSlider.value = rgbPct;
      if (brightnessText) brightnessText.textContent = `${rgbPct}%`;
      if (lampCircle) lampCircle.style.opacity = 0.3 + (rgbPct/100)*0.7;
    }

    // HP LED
    const hpPct = Math.round(js.hp/2.55);
    if (hpLEDSlider) hpLEDSlider.value = hpPct;

  }catch(e){}
}
window.addEventListener('load', fetchStatus);
setInterval(fetchStatus, 5000); // occasional poll

// Tick displayed device time every second
setInterval(()=>{
  if (deviceEpoch != null){
    const now = Date.now();
    const delta = Math.floor((now - lastSyncMs)/1000);
    if (delta > 0){
      deviceEpoch += delta;
      lastSyncMs  += delta*1000;
      updateTimeBox();
    }
  }
}, 1000);

// Manual "Sync Time" from browser -> device (/settime)
if (syncBtn){
  syncBtn.addEventListener('click', async ()=>{
    const epoch = Math.floor(Date.now()/1000);
    const tz    = new Date().getTimezoneOffset(); // minutes; east of UTC => negative
    try {
      await fetch(`/settime?epoch=${epoch}&tz=${tz}`);
      await fetchStatus();
      syncBtn.textContent = 'Synced!';
      setTimeout(()=> syncBtn.textContent = 'Sync Time from Browser', 1500);
    } catch(_) {}
  });
}

// ==== PARTY / UI niceties (unchanged) ====
const partyToggle = document.getElementById('partyMode');
const partySettings = document.getElementById('partySettings');
if (partyToggle && partySettings) {
  partyToggle.addEventListener('change', ()=>{
    if (partyToggle.checked) {
      partySettings.style.display='block';
      setTimeout(()=>{ partySettings.style.opacity='1'; partySettings.style.transform='translateY(0)'; },10);
    } else {
      partySettings.style.opacity='0'; partySettings.style.transform='translateY(-10px)';
      setTimeout(()=>{ partySettings.style.display='none'; },300);
    }
  });
}
const customizeBtn = document.getElementById('customizeBtn');
const customPalette = document.getElementById('customPalette');
const paletteStatus = document.getElementById('paletteStatus');
const partyColorPicker = document.getElementById('partyColorPicker');
if (customizeBtn) {
  customizeBtn.addEventListener('click', ()=>{
    const isCustom = paletteStatus.textContent==='Custom';
    if (isCustom){ paletteStatus.textContent='Default'; customPalette.classList.remove('show'); customizeBtn.textContent='Customize';
      if (lampCircle) lampCircle.style.backgroundColor='#abcdef';
      if (colorPicker) colorPicker.value='#abcdef';
      if (lampColorText) lampColorText.textContent='#ABCDEF';
      sendRGBdeb();
    } else { paletteStatus.textContent='Custom'; customPalette.classList.add('show'); customizeBtn.textContent='Default'; }
  });
  if (partyColorPicker) {
    partyColorPicker.addEventListener('input', ()=>{
      if (lampCircle) lampCircle.style.backgroundColor=partyColorPicker.value;
      if (lampColorText) lampColorText.textContent=partyColorPicker.value.toUpperCase();
      if (colorPicker) colorPicker.value=partyColorPicker.value;
      sendRGBdeb();
    });
  }
}

// Only apply the single-selection behaviour to the Party effects,
// not to the alarm-type buttons in Advanced Settings.
document.querySelectorAll('#partySettings .effect').forEach(effect=>{
  effect.addEventListener('click', ()=>{
    document.querySelectorAll('#partySettings .effect').forEach(e=>e.classList.remove('active'));
    effect.classList.add('active');
    effect.style.transform='scale(0.95)';
    setTimeout(()=>{ effect.style.transform='scale(1)'; },150);
  });
});

// ==== Alarms UI (unchanged) ====
const hourInput      = document.getElementById('hour');
const minuteInput    = document.getElementById('minute');
const addAlarmBtn    = document.getElementById('addAlarm');
const alarmList      = document.getElementById('alarmList');
const nextTime       = document.getElementById('nextTime');
const timeRemaining  = document.getElementById('timeRemaining');

function formatTimeInput(input, min, max) {
  input.addEventListener('input', ()=>{
    let v = parseInt(input.value)||0; if (v>max) v=max; if (v<min) v=min; input.value=v.toString().padStart(2,'0');
  });
  input.addEventListener('wheel', e=>{
    e.preventDefault(); let v=parseInt(input.value)||0; v = e.deltaY<0 ? v+1 : v-1;
    if (v>max) v=min; if (v<min) v=max; input.value=v.toString().padStart(2,'0');
  });
  input.addEventListener('keydown', e=>{
    let v=parseInt(input.value)||0;
    if (e.key==='ArrowUp'){ e.preventDefault(); v=v+1>max?min:v+1; input.value=v.toString().padStart(2,'0'); }
    if (e.key==='ArrowDown'){ e.preventDefault(); v=v-1<min?max:v-1; input.value=v.toString().padStart(2,'0'); }
  });
}

async function fetchAlarms() {
  try{
    const resp = await fetch('/alarms/list');
    const js = await resp.json();
    return js.alarms || [];
  }catch(e){ return []; }
}
function dayLettersFromRow(){
  const act = Array.from(document.querySelectorAll('.day.active')).map(d=>d.textContent).join('');
  return act;
}
async function renderAlarms() {
  const alarms = await fetchAlarms();
  alarmList.innerHTML = '';
  if (!alarms.length) { alarmList.innerHTML = '<p class="empty">No alarms have been set yet.</p>'; updateNextAlarm(alarms); return; }

  alarms.sort((a,b)=>a.time.localeCompare(b.time));
  alarms.forEach(alarm=>{
    const div = document.createElement('div');
    div.className = `alarm-item ${alarm.enabled?'':'disabled'}`;
    div.innerHTML = `
      <div class="alarm-info">
        <div class="time">${alarm.time}</div>
        <div class="desc">Alarm, every ${alarm.days && alarm.days.length ? alarm.days.split('').join(',') : 'day'}</div>
      </div>
      <div class="alarm-controls">
        <label class="switch">
          <input type="checkbox" ${alarm.enabled?'checked':''} data-id="${alarm.id}">
          <span class="slider"></span>
        </label>
        <button class="delete-btn" data-id="${alarm.id}" title="Delete alarm">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M3 6h18M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
          </svg>
        </button>
      </div>
    `;
    alarmList.appendChild(div);
  });

  document.querySelectorAll('.alarm-item .switch input').forEach(sw=>{
    sw.addEventListener('change', async (e)=>{
      const id = e.target.dataset.id; const en = e.target.checked ? 1 : 0;
      try{ await fetch(`/alarms/toggle?id=${id}&enabled=${en}`);}catch(_){}
      e.target.closest('.alarm-item').classList.toggle('disabled', !e.target.checked);
      updateNextAlarm(await fetchAlarms());
    });
  });
  document.querySelectorAll('.delete-btn').forEach(btn=>{
    btn.addEventListener('click', async (e)=>{
      const id = e.currentTarget.dataset.id;
      try{ await fetch(`/alarms/delete?id=${id}`);}catch(_){}
      await renderAlarms();
    });
  });

  updateNextAlarm(alarms);
}

function computeNextAlarm(alarms){
  // preview uses browser time
  const now = new Date();
  const currentDay = now.getDay(); // 0..6
  const dayNames = ['S','M','T','W','T','F','S'];
  const currentTime = now.getHours()*60 + now.getMinutes();
  let nextAlarm = null; let minDiff = Infinity;

  alarms.forEach(alarm=>{
    if (!alarm.enabled) return;
    const [ah, am] = alarm.time.split(':').map(Number);
    const alarmTime = ah*60 + am;
    const days = alarm.days || '';
    function dayEnabled(d){ if (!days || days.length===0) return true; return days.indexOf(dayNames[d]) !== -1; }
    if (dayEnabled(currentDay)) {
      let diff = alarmTime - currentTime;
      if (diff>0 && diff<minDiff){ minDiff=diff; nextAlarm=alarm; }
    }
    for (let i=1;i<=7;i++){
      const nd = (currentDay + i) % 7;
      if (dayEnabled(nd)) {
        let diff = alarmTime + 24*60*i - currentTime;
        if (diff<minDiff){ minDiff=diff; nextAlarm=alarm; }
        break;
      }
    }
  });
  return { nextAlarm, minDiff };
}

function updateNextAlarm(alarms) {
  const { nextAlarm, minDiff } = computeNextAlarm(alarms);
  if (nextAlarm) {
    nextTime.textContent = nextAlarm.time;
    const h = Math.floor(minDiff/60), m = minDiff%60;
    timeRemaining.textContent = h>0 ? `In ${h}h ${m}m` : `In ${m}m`;
    nextTime.style.color = '#007bff'; nextTime.style.fontWeight='700';
  } else {
    nextTime.textContent='--:--'; timeRemaining.textContent='No upcoming alarms';
    nextTime.style.color='#999'; nextTime.style.fontWeight='400';
  }
}

if (hourInput && minuteInput) {
  formatTimeInput(hourInput,0,23);
  formatTimeInput(minuteInput,0,59);
  hourInput.value='00'; minuteInput.value='00';

  document.querySelectorAll('.day').forEach(day=>{
    day.addEventListener('click', ()=>{
      day.classList.toggle('active');
      day.style.transform='scale(0.9)'; setTimeout(()=>{ day.style.transform='scale(1)'; },150);
    });
  });

  addAlarmBtn.addEventListener('click', async ()=>{
    const h = hourInput.value.padStart(2,'0');
    const m = minuteInput.value.padStart(2,'0');
    const days = dayLettersFromRow();
    try{
      await fetch(`/alarms/add?time=${h}:${m}&days=${encodeURIComponent(days)}&enabled=1`);
      addAlarmBtn.style.transform='scale(0.95)'; setTimeout(()=>{ addAlarmBtn.style.transform='scale(1)'; },150);
      await renderAlarms();
    }catch(_){}
  });

  setInterval(async ()=>{ updateNextAlarm(await fetchAlarms()); }, 60000);
  (async ()=>{ await renderAlarms(); })();
}

// ==== Party Mode + Music Sync (ESP32-side /party/set integration) ====
document.addEventListener('DOMContentLoaded', ()=>{
  const partySettings    = document.getElementById('partySettings');
  if (!partySettings) return;

  // Always show party settings (ignore any old show/hide animation)
  partySettings.style.display   = 'block';
  partySettings.style.opacity   = '1';
  partySettings.style.transform = 'none';

  const partyModeOrig   = document.getElementById('partyMode');
  const musicModeOrig   = document.getElementById('musicMode');
  const effectButtons   = Array.from(document.querySelectorAll('#partySettings .effect'));
  const speedSlider     = document.getElementById('partySpeed');
  const brightSlider    = document.getElementById('partyBrightness');
  const colorModeInputs = Array.from(document.querySelectorAll('input[name="partyColorMode"]'));
  const singleColorBox  = document.getElementById('partySingleColorBox');
  const singleColorPicker = document.getElementById('partyColorPicker');

  // Clone switches to clear any previous listeners that might hide the settings
  let partyMode = partyModeOrig;
  if (partyModeOrig) {
    const clone = partyModeOrig.cloneNode(true);
    partyModeOrig.parentNode.replaceChild(clone, partyModeOrig);
    partyMode = clone;
  }
  let musicMode = musicModeOrig;
  if (musicModeOrig) {
    const clone = musicModeOrig.cloneNode(true);
    musicModeOrig.parentNode.replaceChild(clone, musicModeOrig);
    musicMode = clone;
  }

  // Local state (kept simple)
  const partyState = {
    on:     partyMode ? partyMode.checked : false,
    music:  musicMode ? musicMode.checked : false,
    effect: 0,
    speed:  speedSlider  ? parseInt(speedSlider.value  || '50', 10) : 50,
    bri:    brightSlider ? parseInt(brightSlider.value || '80', 10) : 80,
    mode:   'rgb',
    color:  singleColorPicker ? (singleColorPicker.value || '#ff0000') : '#ff0000'
  };

  function updateSingleColorVisibility(){
    if (!singleColorBox) return;
    singleColorBox.style.display = (partyState.mode === 'single') ? 'block' : 'none';
  }

  async function sendPartyConfig(){
    const rgb = typeof hexToRgb === 'function' ? hexToRgb(partyState.color) : {r:255,g:0,b:0};
    const params = new URLSearchParams({
      on:     partyState.on    ? '1' : '0',
      music:  partyState.music ? '1' : '0',
      effect: String(partyState.effect),
      speed:  String(partyState.speed),
      bri:    String(partyState.bri),
      mode:   partyState.mode,
      r:      String(rgb.r),
      g:      String(rgb.g),
      b:      String(rgb.b)
    });
    try {
      await fetch('/party/set?' + params.toString());
    } catch(e) {
      console.warn('Failed to send party config', e);
    }
  }

  // Effect buttons
  effectButtons.forEach((btn, idx)=>{
    btn.addEventListener('click', ()=>{
      effectButtons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      partyState.effect = idx;
      sendPartyConfig();
    });
  });

  // Speed / brightness
  if (speedSlider) {
    speedSlider.addEventListener('input', ()=>{
      partyState.speed = parseInt(speedSlider.value || '50', 10);
      sendPartyConfig();
    });
  }
  if (brightSlider) {
    brightSlider.addEventListener('input', ()=>{
      partyState.bri = parseInt(brightSlider.value || '80', 10);
      sendPartyConfig();
    });
  }

  // Color mode radios
  colorModeInputs.forEach(input=>{
    input.addEventListener('change', ()=>{
      if (!input.checked) return;
      partyState.mode = input.value;
      updateSingleColorVisibility();
      sendPartyConfig();
    });
  });

  // Single-color picker
  if (singleColorPicker) {
    singleColorPicker.addEventListener('input', ()=>{
      partyState.color = singleColorPicker.value || '#ff0000';
      sendPartyConfig();
    });
  }

  // Switches
  if (partyMode) {
    partyMode.addEventListener('change', ()=>{
      partyState.on = partyMode.checked;
      sendPartyConfig();
    });
  }
  if (musicMode) {
    musicMode.addEventListener('change', ()=>{
      partyState.music = musicMode.checked;
      sendPartyConfig();
    });
  }

  // Initialise from /status so the UI matches device state
  async function initFromStatus(){
    try {
      const res = await fetch('/status');
      const js  = await res.json();
      if (!js.party) return;
      const p = js.party;

      partyState.on    = !!p.enabled;
      partyState.music = !!p.music;
      partyState.effect= p.effect ?? 0;
      partyState.speed = p.speed  ?? partyState.speed;
      partyState.bri   = p.bri    ?? partyState.bri;
      partyState.mode  = p.modeName || p.mode || partyState.mode;
      partyState.color = p.color  || partyState.color;

      if (partyMode) partyMode.checked = partyState.on;
      if (musicMode) musicMode.checked = partyState.music;
      if (speedSlider)  speedSlider.value  = partyState.speed;
      if (brightSlider) brightSlider.value = partyState.bri;
      if (singleColorPicker) singleColorPicker.value = partyState.color;

      effectButtons.forEach((btn, idx)=>{
        if (idx === partyState.effect) btn.classList.add('active');
        else                           btn.classList.remove('active');
      });

      colorModeInputs.forEach(input=>{
        input.checked = (input.value === partyState.mode);
      });

      updateSingleColorVisibility();
    } catch(e) {
      console.warn('Failed to init party state', e);
    }
  }

  updateSingleColorVisibility();
  initFromStatus();
});

// ==== Advanced Alarm Settings: ramp + type + timeout + test ====
document.addEventListener('DOMContentLoaded', ()=>{
  const rampInput      = document.getElementById('rampSeconds');
  const timeoutInput   = document.getElementById('alarmTimeoutMinutes');
  const alarmTypeGroup = document.getElementById('alarmTypeGroup');
  const ledBtn         = alarmTypeGroup ? alarmTypeGroup.querySelector('[data-type="led"]')    : null;
  const buzzBtn        = alarmTypeGroup ? alarmTypeGroup.querySelector('[data-type="buzzer"]') : null;
  const testBtn        = document.getElementById('testRampBtn');
  const stopBtn        = document.getElementById('stopTestBtn');
  const progressLine   = document.getElementById('testProgress');
  const progressFill   = progressLine ? progressLine.querySelector('.progress-fill') : null;
  const resetAlarmBtn  = document.getElementById('resetAlarmBtn');

  if (!rampInput || !timeoutInput || !alarmTypeGroup || !ledBtn || !buzzBtn || !testBtn || !stopBtn) return;

  let testTimer   = null;
  let testDurMs   = 0;
  let testStartMs = 0;

  function setProgressActive(active){
    if (!progressLine || !progressFill) return;
    if (active) {
      progressLine.classList.add('active');
      progressFill.style.width = '0%';
    } else {
      progressLine.classList.remove('active');
      progressFill.style.width = '0%';
    }
  }

  function startProgress(seconds){
    if (!progressLine || !progressFill) return;
    clearInterval(testTimer);
    testDurMs   = seconds * 1000;
    testStartMs = Date.now();
    setProgressActive(true);

    testTimer = setInterval(()=>{
      const elapsed = Date.now() - testStartMs;
      const frac = Math.min(elapsed / testDurMs, 1);
      progressFill.style.width = (frac * 100).toFixed(1) + '%';
      if (frac >= 1) {
        clearInterval(testTimer);
        setTimeout(()=>{ stopTest(true); }, 300); // auto-stop after ramp ends
      }
    }, 100);
  }

  async function loadAlarmCfg(){
    try{
      const resp = await fetch('/alarmcfg/get', { cache:'no-store' });
      if (!resp.ok) return;
      const js = await resp.json();
      if (typeof js.leadSec === 'number') {
        rampInput.value = js.leadSec;
      }
      if (typeof js.timeoutSec === 'number') {
        const mins = Math.round(js.timeoutSec / 60);
        timeoutInput.value = mins;
      }
      if (typeof js.useLED === 'boolean') {
        if (js.useLED) ledBtn.classList.add('active');
        else           ledBtn.classList.remove('active');
      }
      if (typeof js.useBuzzer === 'boolean') {
        if (js.useBuzzer) buzzBtn.classList.add('active');
        else              buzzBtn.classList.remove('active');
      }
    }catch(e){}
  }

  async function saveAlarmCfg(){
    const leadSec = parseInt(rampInput.value || '0', 10) || 0;
    let timeoutMin = parseInt(timeoutInput.value || '0', 10) || 0;
    if (timeoutMin < 1) timeoutMin = 1;
    if (timeoutMin > 120) timeoutMin = 120;
    const useLED  = ledBtn.classList.contains('active');
    const useBuzz = buzzBtn.classList.contains('active');

    const params = new URLSearchParams({
      lead:    String(leadSec),
      led:     useLED ? '1' : '0',
      buzz:    useBuzz ? '1' : '0',
      timeout: String(timeoutMin * 60)
    });
    try{
      await fetch('/alarmcfg/set?' + params.toString());
    }catch(e){}
  }
  const saveAlarmCfgDeb = typeof debounce === 'function'
    ? debounce(saveAlarmCfg, 300)
    : saveAlarmCfg;

  rampInput.addEventListener('change', saveAlarmCfgDeb);
  rampInput.addEventListener('blur', saveAlarmCfgDeb);
  timeoutInput.addEventListener('change', saveAlarmCfgDeb);
  timeoutInput.addEventListener('blur', saveAlarmCfgDeb);

  // Alarm type buttons act as independent toggles (LED, Buzzer, or both)
  ledBtn.addEventListener('click', ()=>{
    ledBtn.classList.toggle('active');
    saveAlarmCfgDeb();
  });

  buzzBtn.addEventListener('click', ()=>{
    buzzBtn.classList.toggle('active');
    saveAlarmCfgDeb();
  });

  async function startTest(){
    const leadSec = parseInt(rampInput.value || '0', 10) || 0;
    if (leadSec <= 0) return;

    const params = new URLSearchParams({ duration: String(leadSec) });
    try{
      testBtn.disabled = true;
      stopBtn.disabled = false;
      await fetch('/alarmtest/start?' + params.toString());
      startProgress(leadSec);
    }catch(e){
      testBtn.disabled = false;
      stopBtn.disabled = true;
      setProgressActive(false);
    }
  }

  async function stopTest(fromAuto=false){
    clearInterval(testTimer);
    try{
      await fetch('/alarmtest/stop');
    }catch(e){}
    if (!fromAuto){
      setProgressActive(false);
    }
    testBtn.disabled = false;
    stopBtn.disabled = true;
  }

  testBtn.addEventListener('click', startTest);
  stopBtn.addEventListener('click', ()=> stopTest(false));

  if (resetAlarmBtn) {
    resetAlarmBtn.addEventListener('click', async ()=>{
      try{
        await fetch('/alarm/reset', { cache:'no-store' });
      }catch(e){}
    });
  }

  loadAlarmCfg();
});

)rawliteral";
