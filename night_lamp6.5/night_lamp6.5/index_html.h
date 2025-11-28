#pragma once
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<link rel="stylesheet" href="/style.css">
<script src="/script.js" defer></script>

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Lumina – Smart Light Control</title>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;500;600;700&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="style.css" />
</head>
<body>
  <div class="app">
    <header>
      <h1 class="logo">Lumina</h1>
      <nav>
        <a href="#" class="active">Settings</a>
        <a href="alarms.html">Alarms</a>
      </nav>
    </header>

    <main>
      <!-- Lamp State -->
      <section class="card lamp-state">
        <div class="lamp-circle" id="lampCircle"></div>
        <div class="lamp-info">
          <h2>Current Lamp State</h2>
          <p>
            Color: <span id="lampColor">#ABCDEF</span>,
            Brightness: <span id="lampBrightness">80%</span>
          </p>
        </div>
      </section>

      <!-- Brightness + Color Picker -->
      <section class="card">
        <h3>Current Lamp State</h3>
        <label>Main Brightness</label>
        <input type="range" id="brightness" min="0" max="100" value="80" />

        <label>Color Picker</label>
        <input type="color" id="colorPicker" value="#abcdef" />
        <button class="default-btn">Set as Default</button>
      </section>

            <!-- Modes & Party -->
      <section class="card">
        <!-- (no "Modes" heading text as requested) -->

        <!-- Party Mode first -->
        <div class="mode">
          <div>
            <p><strong>Party Mode</strong><br><span>Dynamic light patterns</span></p>
          </div>
          <label class="switch">
            <input type="checkbox" id="partyMode" />
            <span class="slider"></span>
          </label>
        </div>

        <!-- Music Sync second -->
        <div class="mode">
          <div>
            <p><strong>Music Sync</strong><br><span>Reacts to nearby sounds</span></p>
          </div>
          <label class="switch">
            <input type="checkbox" id="musicMode" />
            <span class="slider"></span>
          </label>
        </div>

        <!-- Party settings: always visible and in the same box -->
        <div class="party-settings" id="partySettings">

          <!-- Effects -->
          <label>Effects</label>
          <div class="effects">
            <button class="effect active" data-effect="0">Fade</button>
            <button class="effect" data-effect="1">Strobe</button>
            <button class="effect" data-effect="2">Pulse</button>
          </div>

          <!-- Speed -->
          <label>Speed</label>
          <input type="range" id="partySpeed" min="0" max="100" value="50">

          <!-- Brightness -->
          <label>Brightness</label>
          <input type="range" id="partyBrightness" min="0" max="100" value="80">

          <!-- Color mode -->
          <label>Color Mode</label>
          <div class="palette-row">
            <label><input type="radio" name="partyColorMode" value="rgb" checked> RGB</label>
            <label><input type="radio" name="partyColorMode" value="random"> Random</label>
            <label><input type="radio" name="partyColorMode" value="single"> Single Color</label>
          </div>

          <!-- Single-color picker (used when "single" is selected) -->
          <div class="custom-palette" id="partySingleColorBox">
            <input type="color" id="partyColorPicker" value="#ff0000">
          </div>
        </div>
      </section>


      <!-- Advanced Settings -->
      <section class="card">
        <h3>Advanced Settings</h3>

        <!-- Existing white LED manual control -->
        <label>White LED Intensity</label>
        <input type="range" id="hpLED" min="0" max="100" value="60" />

        <!-- Ramp lead time -->
        <div class="field-row">
          <label for="rampSeconds">Wake-up ramp lead time (seconds)</label>
          <input type="number" id="rampSeconds" min="10" max="7200" step="10" value="600" />
        </div>

        <!-- Alarm timeout -->
        <div class="field-row">
          <label for="alarmTimeoutMinutes">Alarm timeout (minutes)</label>
          <input type="number" id="alarmTimeoutMinutes" min="1" max="120" step="1" value="10" />
        </div>

        <!-- Alarm type -->
        <label>Alarm Type</label>
        <div class="effects" id="alarmTypeGroup">
          <button type="button" class="effect alarm-type active" data-type="led">LED</button>
          <button type="button" class="effect alarm-type" data-type="buzzer">Beeper</button>
        </div>

        <!-- Ramp test -->
        <div class="test-row">
          <button type="button" class="default-btn test-btn" id="testRampBtn">Test Ramp</button>
          <button type="button" class="default-btn test-btn" id="stopTestBtn" disabled>Stop Test</button>
        </div>

        <div class="progress-line" id="testProgress" aria-hidden="true">
          <div class="progress-fill"></div>
        </div>

        <!-- Reset active alarm -->
        <div class="test-row">
          <button type="button" class="default-btn" id="resetAlarmBtn">Reset Alarm</button>
        </div>
      </section>
    </main>
  </div>

  <script src="script.js"></script>
</body>
</html>
)rawliteral";
