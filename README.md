# 🚗 ECUMaster Black + ESP32 Bluetooth Display

[![ESP32](https://img.shields.io/badge/board-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![ECUMaster](https://img.shields.io/badge/device-ECUMaster-black.svg)](https://www.ecumaster.com)
[![Bluetooth](https://img.shields.io/badge/communication-Bluetooth-brightgreen.svg)](https://en.wikipedia.org/wiki/Bluetooth)
[![LVGL](https://img.shields.io/badge/UI-LVGL-purple.svg)](https://lvgl.io/)
[![Arduino](https://img.shields.io/badge/framework-Arduino-blue.svg)](https://www.arduino.cc)
[![License: GPL v3](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Language: C++](https://img.shields.io/badge/language-C++-orange.svg)](https://isocpp.org)
[![Vehicle Telemetry](https://img.shields.io/badge/type-Telemetry-lightgrey.svg)]()

---

### 📦 Description

This project interfaces the **ECUMaster Black ECU** with an **ESP32** to create a Bluetooth-enabled dashboard display, featuring:

- Automatic Bluetooth reconnection
- Real-time monitoring of engine vitals
- **Peak value tracking (Peak Boost & Peak Coolant Temperature)**
- Custom alerts (CEL, coolant temp, RPM, AFR, voltage, boost, etc.) via display & buzzer
- TFT screen GUI (mono-space font) with **LVGL**

> ⚠️ **Note:** If `SerialBT.setPin(pin)` doesn't work, downgrade to **ESP32 Arduino Core 2.0.17** in the board manager.

---

### 🆕 New Functionality - Peak Value Display

The display now tracks and shows:

- **Peak Boost Pressure**
- **Peak Coolant Temperature (CLT)**

These values represent the **highest recorded readings since power-on**, allowing quick review after spirited driving or testing without logging software.

> 💡 Useful for tuning, safety checks, and track sessions.

---

### ✅ Tested On

- ESP32 JC2432W328
- Active 3.3V buzzer
- Arduino Core v2.0.17
- ECUMaster Black with Bluetooth Adapter
- LVGL v8.3

---

### 📥 Installation

1. Clone this repo
2. Install dependencies (LVGL, eSPI, BluetoothSerial, etc.)
3. Edit LVGL and eSPI config if needed.
4. Edit the code - Use your MAC address or use Bluetooth name and Pin.
5. Copy the mono-space fonts (ui_font_JBM_18.c, ui_font_JBM_15.c, ui_font_JBM_10.c) to your project directory
6. Upload to ESP32
7. Optional - Connect an active 3.0-3.3V buzzer to GPIO Pin 22 (for warnings/CEL)
8. Pair with ECUMaster Black Bluetooth adapter
9. Enjoy real-time data on-screen!

---

### 📦 Dependencies

- [BluetoothSerial](https://github.com/espressif/arduino-esp32/tree/master/libraries/BluetoothSerial)
- [LVGL](https://lvgl.io)
- [eSPI](https://github.com/Bodmer/TFT_eSPI)

--- 

## 🔍 ECU Log Analyzer (Web)

Analyze ESP32 / EMU Black CSV logs directly in your browser.

👉 **Launch Log Analyzer**  
https://danuecumaster.github.io/ECUMaster-Black-ESP32-Bluetooth-Display/log_analyzer/

---

### ❓ FAQ

**Q:** SerialBT.setPin() doesn't work?  
**A:** Use ESP32 Arduino Core v2.0.17. Later versions removed this feature.

**Q:** Display not initializing?  
**A:** Check eSPI & LVGL configuration or contact the manufacturer of your display for configuration guidelines.

**Q:** Blurry text?  
**A:** Check lv_conf.h - LV_COLOR_16_SWAP

**Q:** Font size not working?  
**A:** Check lv_conf.h and copy the font file to the main directory.

**Q:** 3D Printing material?  
**A:** ABS or ASA recommended if cabin heat is extremely high. 

---

### 🧰 Hardware Used

- **ESP32 JC2432W328**  
  [🛒 AliExpress link](https://www.aliexpress.com/item/1005006729707613.html)
- **3D Printed Case**  
  [📦 Thingiverse STL](https://www.thingiverse.com/thing:6705691)
- **ECU**  
  ECUMaster Black + Bluetooth Adapter
- **Active 3.3V buzzer**    
 [🛒 AliExpress link](https://www.aliexpress.com/item/1005008682347898.html)
  
---

## 📺 Demo

![Display](Docs/display.jpg)

[![Watch on YouTube](https://img.youtube.com/vi/EzVEeiy3vmI/0.jpg)](https://youtu.be/EzVEeiy3vmI)

---

### 📜 License

This project is licensed under **GPL v3**.  
See the [LICENSE](./LICENSE) file for more info.

---

### ❤️ Credits & Contributions

Made with ❤️ for petrolheads.  
Contributions, forks, and feature requests are welcome!
