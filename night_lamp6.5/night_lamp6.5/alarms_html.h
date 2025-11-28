#pragma once
const char ALARMS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Lumina – Alarms</title>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;500;600;700&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="style.css" />
</head>
<body>
  <div class="app">
    <header>
      <h1 class="logo">Lumina</h1>
      <nav>
        <a href="index.html">Settings</a>
        <a href="#" class="active">Alarms</a>
      </nav>
    </header>

    <main>
      <!-- Current Time (Device) -->
      <section class="card rtc-card">
        <h3>Current Time (Device)</h3>
        <div class="rtc-row">
          <div class="rtc-time" id="rtcTime">--:--:--</div>
          <div class="rtc-date" id="rtcDate">--</div>
          <div class="rtc-state" id="rtcStatus">Source: internal</div>
        </div>
        <div style="margin-top:12px;">
          <button class="default-btn" id="syncTimeBtn">Sync Time from Browser</button>
        </div>
      </section>

      <!-- Next Alarm -->
      <section class="card next-alarm">
        <h3>Next Alarm</h3>
        <p id="nextTime">--:--</p>
        <p class="subtext" id="timeRemaining">No upcoming alarms</p>
      </section>

      <!-- Set Alarm -->
      <section class="card set-alarm">
        <h3>Set Alarm</h3>

        <div class="wheel-container">
          <input type="number" id="hour" min="0" max="23" value="0">
          <span>:</span>
          <input type="number" id="minute" min="0" max="59" value="0">
        </div>

        <div class="days">
          <button class="day">S</button>
          <button class="day">M</button>
          <button class="day">T</button>
          <button class="day">W</button>
          <button class="day">T</button>
          <button class="day">F</button>
          <button class="day">S</button>
        </div>

        <button id="addAlarm" class="add-btn">+ Add Alarm</button>
      </section>

      <!-- Alarm List -->
      <section class="alarm-list card" id="alarmList">
        <p class="empty">No alarms have been set yet.</p>
      </section>
    </main>
  </div>

  <script src="script.js"></script>
</body>
</html>
)rawliteral";
