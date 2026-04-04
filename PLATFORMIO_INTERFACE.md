# PlatformIO VSCode Interface Guide

## The Main View You'll See

```
┌─────────────────────────────────────────────────────────────────┐
│ VSCode with PlatformIO Extension                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│ LEFT SIDEBAR (File Explorer):                                    │
│ 📁 firmware                                                       │
│   📁 include/                                                     │
│     📄 config.h ◄─── EDIT THIS (WiFi settings)                  │
│     📄 dmx_engine.h                                              │
│     📄 config_manager.h                                          │
│     📄 scene_manager.h                                           │
│   📁 src/                                                         │
│     📄 main.cpp (Arduino code)                                   │
│     📄 dmx_engine.cpp                                            │
│     📄 config_manager.cpp                                        │
│     📄 scene_manager.cpp                                         │
│   📄 platformio.ini (project config)                             │
│                                                                   │
│ BOTTOM TOOLBAR:                                                  │
│ ┌──────────────────────────────────────────────────────────────┐ │
│ │ PlatformIO: esp32 │ ▶ Build │ ↑ Upload │ ⊙ Serial Monitor  │ │
│ └──────────────────────────────────────────────────────────────┘ │
│ └─ Click HERE to upload!                                         │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Step-by-Step Visual

### 1️⃣ Open Folder
```
File Menu
  ↓
Open Folder (Ctrl+K Ctrl+O)
  ↓
Select: c:\Temp\DMXESP\firmware
  ↓
✓ Folder opened
```

### 2️⃣ Edit config.h
```
Left sidebar:
  firmware/
    include/
      config.h ◄─ CLICK THIS
        ↓
Edit these lines:
  Line 6:  #define WIFI_SSID "YOUR_SSID"
  Line 7:  #define WIFI_PASSWORD "YOUR_PASSWORD"
        ↓
Save (Ctrl+S)
```

### 3️⃣ Plug in USB
```
Your Computer          ESP32
┌──────────────────────────┐
│ USB Port ────────────── USB Port
└──────────────────────────┘
      (physical cable)
```

### 4️⃣ Click Upload
```
Bottom PlatformIO toolbar:
  
  ┌───────────────────────────────┐
  │ esp32 │ Build │ Upload │ Monitor │
  └───────────────┬────────────────┘
                  │
              🖱️ CLICK THIS
                  │
              ┌───v───┐
              │ Build │ (compile)
              │Upload │ (upload)
              └───────┘
                  │
              Compiling...    ████████░░ 45%
              Uploading...    ████████████ 100%
              Done! ✓
```

### 5️⃣ Monitor Output
```
Click ⊙ Monitor button
        ↓
Output window opens:
        ↓
=== DMXESP ===
✓ WiFi connected!
IP address: 192.168.1.50 ◄─ NOTE THIS NUMBER
✓ Web server started
=== System Ready ===
```

### 6️⃣ Open Browser
```
Address bar:
  http://192.168.1.50
        ↓
    Enter/Return
        ↓
   Web Interface Loads! 🎉
```

## Common Buttons Reference

```
┌─────────────────────────────────────────────────────────────┐
│                  PlatformIO Toolbar                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ▶ Build        - Compile only (no upload)                  │
│  ↑ Upload       - Compile + Upload to ESP32 ◄─ USE THIS     │
│  ⊙ Monitor      - View serial output                         │
│  ✓ Check        - Syntax check (very fast)                  │
│  🔧 Project     - Show project tasks                         │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## File Editing Reference

### config.h (Main Settings)
```cpp
// Lines to change:
6   #define WIFI_SSID "YOUR_SSID"        👈 Change this
7   #define WIFI_PASSWORD "YOUR_PASSWORD" 👈 And this
8   #define ADMIN_PIN "1234"               👈 Optional

// Leave these as-is (GPIO pins):
12  #define DMX_TX_PIN 17
13  #define DMX_RX_PIN 16
14  #define DMX_DE_PIN 15
```

### main.cpp (Arduino Code)
```cpp
void setup()  - Runs once when ESP32 starts
void loop()   - Runs repeatedly (like in Arduino)
// Most of the code is here - Don't edit unless you know what you're doing
```

## Build Output Explained

### Successful Upload
```
Compiling firmware for esp32...
Compiling .pio\build\esp32\src\main.cpp.o
Compiling .pio\build\esp32\src\dmx_engine.cpp.o
Compiling .pio\build\esp32\src\config_manager.cpp.o
Compiling .pio\build\esp32\src\scene_manager.cpp.o
Linking .pio\build\esp32\firmware.elf
Building .pio\build\esp32\firmware.bin
Image successfully validated
==================ota digest and crc metadata ==================
Uploading .pio\build\esp32\firmware.bin
Erasing flash...
Writing at 0x00000400... (5 %)
Writing at 0x00001c00... (10%)
Writing at 0x00003400... (15%)
...
Writing at 0x000c0000... (100%)
Hard resetting via RTS pin...
Done uploading ✓
```

### If Upload Fails
```
error: Failed to open COM3
↓
Solution: Close Serial Monitor and try again
or unplug/replug USB cable
```

## Serial Monitor Output Explained

```
Line 1: ┐
Line 2: │
Line 3: │ Boot messages (just info)
Line 4: │
Line 5: ┘

Line 6: === DMXESP - Professional DMX Lighting Controller ===
Line 7: Initializing...

Line 8: ✓ SPIFFS initialized          ← File system OK
Line 9: Connecting to WiFi: MyWiFi    ← Your WiFi name
Line 10: .................✓ WiFi connected!  ← Connected!
Line 11: IP address: 192.168.1.50    ← YOUR IP ADDRESS ◄◄
Line 12: ✓ DMX engine initialized     ← DMX ready
Line 13: ✓ Web server started on port 80 ← Web server running
Line 14: === System Ready ===         ← All good! 🎉
```

## Keyboard Shortcuts

```
Ctrl+Shift+P    - Open command palette (search commands)
Ctrl+S          - Save file
Ctrl+Alt+U      - Upload (same as Upload button)
Ctrl+Alt+A      - Open Serial Monitor
Ctrl+Alt+C      - Build
Ctrl+,          - Open settings
Ctrl+`          - Open terminal
```

## Typical First Build Timeline

```
Action                          Time
─────────────────────────────────────────
You click Upload                 0s ◄─ Now
PlatformIO installs tools        5-15s (only first time!)
Compiling firmware               20-45s
Uploading to ESP32               10-20s
Reboot and serial output         5s
                                 ─────
Total                            40-80s (first time)
                                 20-50s (subsequent)
```

## Success Checklist

After uploading, you should see in Serial Monitor:

- [ ] `=== DMXESP ===` message
- [ ] `✓ WiFi connected!` (not just "Connecting...")
- [ ] `IP address: 192.168.x.x` (different number for you)
- [ ] `✓ DMX engine initialized`
- [ ] `✓ Web server started`
- [ ] `=== System Ready ===`

If all ✓, then try: `http://<your IP>` in browser

## Troubleshooting Flowchart

```
Upload fails?
    │
    ├─ Serial Monitor showing errors?
    │  └─ Read the error message carefully
    │     └─ Most common: Port not found → Unplug/replug USB
    │
    ├─ Can't find ESP32?
    │  └─ Close Serial Monitor first
    │     └─ Try different USB port/cable
    │
    ├─ Still stuck?
    │  └─ Right-click folder → Clean (PlatformIO)
    │     └─ Try upload again
    │
    └─ Nothing works?
       └─ Restart VSCode
          └─ Check physical connections
```

## What Happens When You Click Upload

```
1. Saves all files
2. Compiles C++ code to machine code
3. Links all code together
4. Creates binary file (.bin)
5. Connects to ESP32 via USB/COM port
6. Erases ESP32 flash memory
7. Writes new firmware
8. Resets ESP32
9. ESP32 boots with new code
10. Serial Monitor shows output
```

---

**Ready to click Upload?** The full guide is in `PLATFORMIO_BUILD_FLASH.md` if you need more details!
