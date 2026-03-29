# 🛠️ ESP32 HW Diagnostics — Hardware Suite v1.0

A polished, full-featured hardware diagnostic tool for the **ESP32-S3** platform. Tests the ST7789 display, ES8311 audio codec/speaker, and INMP441 microphone in sequence with animated boot screen, progress indicators, badge-style results, and a live mic scope on the summary screen.

---

## ✨ Features

- Animated starfield + bracket boot sequence
- 3-stage test suite: Display → Codec/Speaker → INMP441 Mic
- Colour-wash and gradient display test
- ES8311 chime + frequency sweep (200 Hz → 4 kHz)
- INMP441 microphone recording with live oscilloscope
- Badge-style PASS / FAIL indicators per component
- Scrolling log panel with fading message history
- Full summary screen with live mic scope running post-test
- Serial monitor output throughout

---

## 🛒 Hardware Required

| Component | Details |
|-----------|---------|
| ESP32-S3 Dev Module | Any standard ESP32-S3 board |
| ES8311 Codec Module | I2C address 0x18 |
| INMP441 Microphone | I2S digital MEMS mic |
| ST7789 TFT Display | 240 × 320 px |

---

## 🔌 Wiring

### ES8311 Codec → ESP32-S3

| Signal | GPIO |
|--------|------|
| PA_EN | 48 |
| DOUT | 45 |
| DIN | 12 |
| WS | 13 |
| BCLK | 14 |
| MCLK | 38 |
| SCL | 2 |
| SDA | 1 |

### INMP441 Microphone → ESP32-S3

| Signal | GPIO |
|--------|------|
| VDD | 3.3V |
| GND | GND |
| L/R | GND (left channel) |
| WS | 4 |
| SCK | 5 |
| SD | 6 |

### ST7789 TFT → ESP32-S3

| Signal | GPIO |
|--------|------|
| DC | 39 |
| CS | 47 |
| CLK | 41 |
| SDA (MOSI) | 40 |
| BLK | 42 |

---

## 📚 Libraries

| Library | Source |
|---------|--------|
| `arduino-audio-tools` | [pschatzmann/arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools) |
| `arduino-audio-driver` | [pschatzmann/arduino-audio-driver](https://github.com/pschatzmann/arduino-audio-driver) |
| `Adafruit ST7789` | Arduino Library Manager |
| `Adafruit GFX Library` | Arduino Library Manager |

---

## ⚙️ Board Settings (Arduino IDE)

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC on Boot | Enabled |

---

## 🧪 Test Sequence

### Test 1 — Display
- Solid colour fills (Red, Green, Blue, Cyan, Yellow, White)
- Horizontal RGB gradient
- Pixel dot pattern

### Test 2 — Codec + Speaker
- ES8311 initialisation over I2C
- 4-note chime (C5 → C6)
- Frequency sweep 200 Hz → 4 kHz with on-screen bar visualiser

### Test 3 — INMP441 Microphone
- I2S port 1 initialisation
- 3-second recording with countdown and live oscilloscope
- Pass threshold: peak amplitude > 300

---

## 📊 Summary Screen

After all tests complete, a colour-coded summary screen displays:
- Per-component PASS / FAIL with detail text
- Live microphone oscilloscope (updates continuously in `loop()`)
- Final footer with author and version info

---

## 🚀 Getting Started

1. Wire hardware per the tables above.
2. Install all libraries.
3. Select **ESP32S3 Dev Module**, enable **USB CDC on Boot**.
4. Upload and open Serial Monitor at **115200 baud**.
5. Watch the animated boot sequence and test progression on the TFT.
6. Speak into the INMP441 during the 3-second mic countdown.

---

## 📄 License

MIT License — see [LICENSE](LICENSE_HWDiagnostics) for details.  
© 2026 Patrick Perez
