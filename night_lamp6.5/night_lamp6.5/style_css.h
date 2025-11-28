#pragma once
const char STYLE_CSS[] PROGMEM = R"rawliteral(
/* ===== General Layout ===== */
body {
  display: flex;
  justify-content: center;
  align-items: flex-start;
  background-color: #e9ebef;
  min-height: 100vh;
  margin: 0;
  font-family: 'Poppins', sans-serif;
  color: #222;
  line-height: 1.5;
}

.app {
  width: 390px;
  max-width: 390px;
  background: #f6f7f8;
  border-radius: 25px;
  overflow-y: auto;
  box-shadow: 0 0 20px rgba(0, 0, 0, 0.1);
  margin: 20px 0;
}

/* ===== Header ===== */
header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 18px 25px;
  background: white;
  border-radius: 25px 25px 0 0;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
}

.logo {
  font-weight: 700;
  font-size: 1.4rem;
  color: #222;
}

nav a {
  text-decoration: none;
  color: #666;
  margin-left: 18px;
  font-weight: 500;
  font-size: 0.95rem;
  padding: 6px 0;
  position: relative;
  transition: color 0.2s ease;
}

nav a.active {
  font-weight: 600;
  color: #007bff;
}

nav a.active::after {
  content: '';
  position: absolute;
  bottom: 0;
  left: 0;
  width: 100%;
  height: 2px;
  background-color: #007bff;
  border-radius: 1px;
}

/* ===== Cards ===== */
main {
  padding: 25px;
}

.card {
  background: white;
  border-radius: 16px;
  padding: 22px;
  margin-bottom: 20px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
  transition: all 0.3s ease;
}

.card:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
}

h2 {
  font-size: 1.3rem;
  margin-top: 0;
  margin-bottom: 0;
  font-weight: 600;
}

h3 {
  font-size: 1.1rem;
  margin-top: 0;
  margin-bottom: 16px;
  font-weight: 600;
}

label {
  display: block;
  font-weight: 500;
  margin-bottom: 8px;
  font-size: 0.95rem;
  color: #444;
}

/* ===== Lamp State ===== */
.lamp-state {
  display: flex;
  align-items: center;
  gap: 18px;
  min-height: 70px;
}

.lamp-circle {
  width: 70px;
  height: 70px;
  border-radius: 50%;
  background-color: #abcdef;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  border: 3px solid white;
  transition: all 0.3s ease;
  flex-shrink: 0;
  aspect-ratio: 1;
}

.lamp-circle:hover {
  transform: scale(1.05);
}

.lamp-info {
  flex: 1;
  min-width: 0;
}

.lamp-info h2 {
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  font-size: 1.2rem;
}

/* ===== Inputs ===== */
input[type="range"] {
  width: 100%;
  margin-bottom: 18px;
  height: 6px;
  border-radius: 3px;
  background: #e0e0e0;
  outline: none;
  -webkit-appearance: none;
}

input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: #007bff;
  cursor: pointer;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

input[type="color"] {
  width: 100%;
  height: 45px;
  border: none;
  border-radius: 10px;
  margin-bottom: 12px;
  cursor: pointer;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
}

/* ===== Switch Toggle ===== */
.mode {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 16px;
  padding: 12px 0;
  border-bottom: 1px solid #f0f0f0;
}

.mode:last-of-type {
  border-bottom: none;
}

.mode div p {
  margin: 0;
}

.mode div p span {
  font-size: 0.85rem;
  color: #666;
}

.switch {
  position: relative;
  display: inline-block;
  width: 48px;
  height: 26px;
}

.switch input { display: none; }

.slider {
  position: absolute;
  cursor: pointer;
  top: 0; left: 0; right: 0; bottom: 0;
  background-color: #ccc;
  border-radius: 26px;
  transition: 0.4s;
  box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.2);
}

.slider:before {
  position: absolute;
  content: "";
  height: 20px; width: 20px;
  left: 3px; bottom: 3px;
  background-color: white;
  border-radius: 50%;
  transition: 0.4s;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
}

input:checked + .slider { 
  background-color: #007bff; 
}

input:checked + .slider:before { 
  transform: translateX(22px); 
}

/* ===== Buttons ===== */
.default-btn, .custom-btn {
  border: none;
  border-radius: 10px;
  padding: 10px 16px;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.2s ease;
  font-size: 0.95rem;
}

.default-btn {
  background: #f0f0f0;
  width: 100%;
  margin-top: 12px;
  color: #444;
}

.default-btn:hover {
  background: #e5e5e5;
}

.custom-btn {
  background: #f0f0f0;
  color: #444;
}

.custom-btn:hover {
  background: #e5e5e5;
}

/* ===== Party & Music Settings ===== */
.party-settings, .music-settings { 
  display: none; 
}

.palette-row, .effects {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin: 12px 0;
}

.effect {
  border: none;
  background: #f0f0f0;
  border-radius: 10px;
  padding: 8px 14px;
  cursor: pointer;
  font-size: 0.9rem;
  transition: all 0.2s ease;
  color: #444;
}

.effect:hover {
  background: #e5e5e5;
}

.effect.active {
  background: #007bff;
  color: white;
}

/* ===== Color Palette Animation ===== */
.custom-palette {
  display: none;
  margin-top: 12px;
  padding: 14px;
  background: #f9f9f9;
  border-radius: 12px;
  opacity: 0;
  transform: translateY(-5px);
  transition: opacity 0.3s ease, transform 0.3s ease;
}

.custom-palette.show {
  display: block;
  opacity: 1;
  transform: translateY(0);
}

.color-pickers {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 15px;
}

.color-picker-item {
  display: flex;
  align-items: center;
  gap: 12px;
}

.color-picker-item label {
  width: 70px;
  margin: 0;
  font-size: 0.9rem;
  color: #666;
}

.color-picker-item input[type="color"] {
  flex: 1;
  margin: 0;
  height: 40px;
}

.add-color-btn {
  width: 100%;
  background: #f0f0f0;
  border: none;
  border-radius: 8px;
  padding: 10px;
  cursor: pointer;
  font-weight: 500;
  color: #666;
  transition: all 0.2s ease;
}

.add-color-btn:hover {
  background: #e5e5e5;
}

.remove-color-btn {
  background: none;
  border: none;
  cursor: pointer;
  padding: 6px;
  border-radius: 6px;
  color: #999;
  transition: all 0.2s ease;
  display: flex;
  align-items: center;
  justify-content: center;
}

.remove-color-btn:hover {
  background: #ffebee;
  color: #f44336;
}

/* ===== Alarm Page ===== */
.next-alarm {
  text-align: center;
  padding: 25px;
}

.next-alarm h3 {
  margin-bottom: 10px;
}

#nextTime {
  font-size: 2.8rem;
  font-weight: 700;
  margin: 10px 0;
  color: #007bff;
}

.subtext {
  color: #666;
  font-size: 0.9rem;
  margin: 0;
}

.wheel-container {
  display: flex;
  justify-content: center;
  align-items: center;
  background: #eaf0ff;
  border-radius: 14px;
  padding: 15px;
  font-size: 2.2rem;
  font-weight: 600;
  margin-bottom: 20px;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.05);
}

.wheel-container input {
  width: 75px;
  font-size: 2.2rem;
  text-align: center;
  border: none;
  background: transparent;
  outline: none;
  -moz-appearance: textfield;
  font-weight: 600;
  color: #222;
}

.wheel-container input::-webkit-outer-spin-button,
.wheel-container input::-webkit-inner-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

.days {
  display: flex;
  justify-content: space-between;
  margin-bottom: 20px;
}

.day {
  width: 42px;
  height: 42px;
  border-radius: 50%;
  border: none;
  background: #f0f0f0;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.2s ease;
  color: #444;
}

.day:hover {
  background: #e5e5e5;
}

.day.active {
  background: #007bff;
  color: white;
  transform: scale(1.05);
}

.add-btn {
  display: block;
  width: 100%;
  background: #007bff;
  color: white;
  border: none;
  border-radius: 12px;
  padding: 14px;
  font-weight: 600;
  cursor: pointer;
  font-size: 1rem;
  transition: all 0.2s ease;
  box-shadow: 0 2px 6px rgba(0, 123, 255, 0.3);
}

.add-btn:hover {
  background: #0069d9;
  transform: translateY(-2px);
  box-shadow: 0 4px 8px rgba(0, 123, 255, 0.4);
}

.add-btn:active {
  transform: translateY(0);
}

/* ===== Alarm List ===== */
.alarm-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin: 12px 0;
  padding: 14px 16px;
  background: #f7f8fa;
  border-radius: 14px;
  transition: all 0.2s ease;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
}

.alarm-item:hover {
  background: #f0f2f5;
  transform: translateY(-2px);
  box-shadow: 0 3px 6px rgba(0, 0, 0, 0.08);
}

.alarm-item .time {
  font-size: 1.6rem;
  font-weight: 600;
  margin-bottom: 4px;
}

.alarm-item .desc {
  font-size: 0.85rem;
  color: #666;
}

.empty {
  text-align: center;
  color: #999;
  font-style: italic;
  margin: 20px 0;
}

/* ===== Alarm Controls ===== */
.alarm-controls {
  display: flex;
  align-items: center;
  gap: 12px;
}

.delete-btn {
  background: none;
  border: none;
  cursor: pointer;
  padding: 6px;
  border-radius: 6px;
  color: #999;
  transition: all 0.2s ease;
  display: flex;
  align-items: center;
  justify-content: center;
}

.delete-btn:hover {
  background: #ffebee;
  color: #f44336;
  transform: scale(1.1);
}

.alarm-info {
  flex: 1;
}

/* ===== Disabled Alarm State ===== */
.alarm-item.disabled .time {
  color: #999;
  text-decoration: line-through;
}

.alarm-item.disabled .desc {
  color: #bbb;
}

/* ===== RTC card ===== */
.rtc-card { }
.rtc-row { display:flex; gap:12px; align-items:baseline; flex-wrap:wrap; }
.rtc-time { font-size:2rem; font-weight:700; letter-spacing:0.02em; }
.rtc-date { opacity:.75; }
.rtc-state { margin-left:auto; font-weight:600; }

/* ===== Responsive Design ===== */
@media (max-width: 420px) {
  .app {
    width: 95%;
    margin: 10px auto;
    border-radius: 20px;
  }
  
  main {
    padding: 20px 15px;
  }
  
  .card {
    padding: 18px;
    border-radius: 14px;
  }
  
  .lamp-state {
    gap: 15px;
  }
  
  .lamp-circle {
    width: 60px;
    height: 60px;
  }
  
  .lamp-info h2 {
    font-size: 1.1rem;
  }
  
  .wheel-container {
    font-size: 1.8rem;
    padding: 12px;
  }
  
  .wheel-container input {
    font-size: 1.8rem;
    width: 60px;
  }
  
  .day {
    width: 38px;
    height: 38px;
  }
  
  #nextTime {
    font-size: 2.4rem;
  }
}

/* ===== Animation Keyframes ===== */
@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.alarm-item {
  animation: fadeIn 0.3s ease;
}

/* ===== Scrollbar Styling ===== */
.app::-webkit-scrollbar {
  width: 6px;
}

.app::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 3px;
}

.app::-webkit-scrollbar-thumb {
  background: #c1c1c1;
  border-radius: 3px;
}

.app::-webkit-scrollbar-thumb:hover {
  background: #a8a8a8;
}

/* ===== Alarm ramp & test ===== */
.field-row {
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-top: 10px;
}

.field-row input[type="number"] {
  width: 100%;
  padding: 6px 10px;
  border-radius: 10px;
  border: 1px solid #d5d7dd;
  font-family: inherit;
  font-size: 0.95rem;
  box-sizing: border-box;
}

.test-row {
  display: flex;
  gap: 8px;
  margin-top: 12px;
}

.test-btn {
  flex: 1;
}

.progress-line {
  position: relative;
  width: 100%;
  height: 6px;
  border-radius: 999px;
  background: #e3e5ea;
  margin-top: 8px;
  overflow: hidden;
  opacity: 0;
  transform: translateY(4px);
  transition: opacity 0.2s ease, transform 0.2s ease;
}

.progress-line.active {
  opacity: 1;
  transform: translateY(0);
}

.progress-fill {
  position: absolute;
  inset: 0;
  width: 0%;
  height: 100%;
  background: linear-gradient(90deg, #4facfe, #00f2fe);
  transition: width 0.15s linear;
}

)rawliteral";
