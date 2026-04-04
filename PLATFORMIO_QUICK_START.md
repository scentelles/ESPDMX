# PlatformIO: Build & Flash in 5 Steps

## Your checklist:

### ✅ Step 1: Open Folder in VSCode

1. In VSCode: **File → Open Folder** (or `Ctrl+K Ctrl+O`)
2. Navigate to: **`c:\Temp\DMXESP\firmware`**
3. Click **Select Folder**
4. Wait 10-15 seconds for PlatformIO to initialize

You should see at the bottom:
```
[PlatformIO] - Status: esp32 board detected
```

### ✅ Step 2: Edit WiFi Settings

1. Open file: **`include/config.h`**
2. Find lines 6-7:
   ```cpp
   #define WIFI_SSID "YOUR_SSID"
   #define WIFI_PASSWORD "YOUR_PASSWORD"
   ```
3. Replace with **your actual WiFi network name and password**
4. **Save file** (`Ctrl+S`)

**Example:**
```cpp
#define WIFI_SSID "MyWiFi"
#define WIFI_PASSWORD "MyPassword123"
```

### ✅ Step 3: Plug in ESP32

Connect ESP32 to your computer via **USB cable**

You should see notification in VSCode or hear Windows sound.

### ✅ Step 4: Click Upload

In VSCode bottom toolbar, you'll see icons like:

```
PlatformIO: esp32  |  ▶ Build  |  ↑ Upload  |  ⊙ Monitor
```

**Click the UP ARROW** (Upload button)

This will:
1. Compile your code (~45 seconds)
2. Flash to ESP32 (~15 seconds)  
3. Show **"Done uploading"** when finished

### ✅ Step 5: Monitor Output

Click the **⊙ Monitor** button (or `Ctrl+Alt+A`)

You should see:
```
=== DMXESP - Professional DMX Lighting Controller ===
Initializing...
✓ SPIFFS initialized
Connecting to WiFi: MyWiFi
.....✓ WiFi connected!
IP address: 192.168.1.50
✓ DMX engine initialized
✓ Web server started on port 80
=== System Ready ===
```

**Save your IP address!** (e.g., `192.168.1.50`)

---

## 🎉 Done! Open Browser

Type in your browser: **`http://192.168.1.50`** (use your IP from step 5)

You should see the DMXESP web interface!

---

## Testing

In the web interface:
- ✅ Slide brightness up/down
- ✅ Click color scenes (should change lights)
- ✅ Click STROBE button (red)
- ✅ Click SMOKE button (red)
- ✅ Click ⚙️ button (should prompt for PIN: `1234`)

**All working?** Perfect! 🎆

---

## Troubleshooting

### Upload button is grayed out
- Make sure USB is plugged in
- Close Serial Monitor first
- Restart VSCode

### "Port not found"
- Unplug and replug USB cable
- Open Device Manager on Windows to check COM port
- May need to install CH340 driver

### Serial Monitor shows nothing
- Click ⊙ Monitor button again
- Check baud rate is 115200 (bottom right of output)
- Try pressing EN button on ESP32

### Can't access `http://192.168.1.50`
- Check you're on same WiFi network as ESP32
- Ping it: open command prompt, type: `ping 192.168.1.50`
- Try `192.168.4.1` if WiFi failed (AP mode)

---

## Next: Add Fixtures

1. Click ⚙️ button → Enter PIN `1234`
2. Go to **Fixtures** tab
3. Click **+ Add Fixture**
4. Enter:
   - Name: "Main Lights"
   - Type: RGB
   - DMX Address: 1
   - Channels: 3
5. Click Save

Then test with real DMX fixtures!

---

**Questions?** See full guide: `PLATFORMIO_BUILD_FLASH.md`
