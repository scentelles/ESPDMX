# PlatformIO Build & Flash Guide

## Quick Start (3 Steps)

### Step 1: Open Firmware Folder in VSCode

1. **File → Open Folder**
2. Navigate to: `c:\Temp\DMXESP\firmware`
3. Click **Select Folder**
4. Wait for PlatformIO to initialize (you'll see "PlatformIO: Initializing" at bottom)

### Step 2: Configure WiFi

Edit `include/config.h`:

```cpp
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define ADMIN_PIN "1234"
```

Change lines 6-8 to your WiFi credentials.

**Optional:** Verify GPIO pins match your hardware:
- `DMX_TX_PIN 17` - TX pin
- `DMX_RX_PIN 16` - RX pin  
- `DMX_DE_PIN 15` - DE (direction enable) pin

### Step 3: Upload to ESP32

**Using PlatformIO GUI (Easiest):**

Look at the bottom toolbar in VSCode. You should see:
```
PlatformIO: esp32 | Build | Upload | Serial Monitor
```

1. **Plug ESP32 into USB**
2. Click **Upload** button (looks like arrow →)
3. PlatformIO will:
   - Compile the code (~30-60 seconds)
   - Upload to ESP32 (~10-20 seconds)
   - Show "Done uploading" when complete

**Alternative - Using CLI:**
```bash
cd c:\Temp\DMXESP\firmware
platformio run --target upload
```

## Verify Upload Success

After upload completes:

1. **Click Serial Monitor** button in toolbar (or press `Ctrl+Alt+A`)
2. You should see boot messages at 115200 baud:
   ```
   === DMXESP - Professional DMX Lighting Controller ===
   Initializing...
   ✓ SPIFFS initialized
   Connecting to WiFi: YOUR_SSID
   ✓ WiFi connected!
   IP address: 192.168.1.100
   ✓ DMX engine initialized
   ✓ Web server started on port 80
   === System Ready ===
   ```

3. Note your IP address (e.g., `192.168.1.100`)

## Access Web Interface

1. Open browser: `http://192.168.1.100` (use your actual IP)
2. You should see the DMXESP user interface
3. Test:
   - Brightness slider works? ✅
   - Color scenes appear? ✅
   - Strobe/smoke buttons visible? ✅

## Next: Admin Panel

1. Click the **⚙️** gear icon (bottom right)
2. Enter PIN: `1234`
3. You're now in admin panel to:
   - Add DMX fixtures
   - Create custom scenes
   - Configure shows

## Troubleshooting

### Upload Fails

**"Port not found"**
- Plug ESP32 in again
- Try different USB port
- Install CH340 driver: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

**"Timeout during upload"**
- Click **Ctrl+C** to stop
- Close Serial Monitor if open
- Try again with fresh upload

**"Firmware too large"**
- Reduce web app size
- Check build output for warnings

### Serial Monitor Shows Garbage

- **Check baud rate**: Must be 115200
- Look at bottom right of Serial Monitor for baud selector
- Click and change to 115200 if needed

### WiFi Won't Connect

- Check SSID/password spelling (case-sensitive)
- Verify WiFi is 2.4GHz (not 5GHz only)
- Look for signal strength in router
- If fails, ESP32 starts AP mode at 192.168.4.1

### Can't Access Web Interface

- Ping the ESP32: `ping 192.168.1.100` (replace with your IP)
- Try from different browser
- Check firewall isn't blocking port 80
- Reboot ESP32 (press EN button)

## Common Tasks

### Monitor Serial Output
```
Ctrl+Alt+A (PlatformIO shortcut)
or click Serial Monitor in toolbar
```

### Rebuild from Scratch
```
Ctrl+Shift+P → PlatformIO: Clean
Then: Ctrl+Shift+P → PlatformIO: Build
```

### Full Build + Upload
```bash
# Command line
cd firmware
platformio run --target upload

# Or use GUI: Click Upload button
```

### Just Compile (No Upload)
```
Ctrl+Shift+P → PlatformIO: Build
or click Build button in toolbar
```

## First Time Setup Checklist

- [ ] Opened `firmware` folder in VSCode
- [ ] Installed ESP32 board (PlatformIO does this automatically)
- [ ] Configured WiFi in `include/config.h`
- [ ] Plugged in ESP32 via USB
- [ ] Clicked Upload button
- [ ] Serial Monitor shows success messages
- [ ] Can access `http://<IP>` in browser
- [ ] Web interface loads and shows scenes
- [ ] Can move brightness slider
- [ ] Can login to admin panel (PIN: 1234)

## Important Notes

⚠️ **First Upload Can Be Slow**
- PlatformIO installs tools on first run
- May take 2-3 minutes first time
- Subsequent uploads are faster (30-60 seconds)

⚠️ **WiFi Credentials**
- Change from default "YOUR_SSID" to actual network
- Password is case-sensitive
- 2.4GHz networks only (ESP32 doesn't support 5GHz in STA mode)

⚠️ **Admin PIN**
- Default: "1234"
- Change in admin settings immediately after first login
- Used to access admin panel

## Next Steps

1. ✅ Build & upload complete
2. 🎨 Add your DMX fixtures in admin panel
3. 🎨 Create custom color scenes
4. 🎆 Build lighting shows
5. 🎉 Deploy with actual lighting equipment

## GUI Elements

**Bottom Toolbar (PlatformIO):**
```
[Platform] esp32 | ▶ Build | ↑ Upload | ⊙ Serial Monitor | ✓ Check
```

- **Build**: Compile only
- **Upload**: Compile + Flash to ESP32  
- **Serial Monitor**: View ESP32 output
- **Check**: Syntax check only

**Explorer (Left Side):**
```
firmware/
├── include/
│   ├── config.h ← 📝 Edit WiFi here
│   ├── dmx_engine.h
│   ├── config_manager.h
│   └── scene_manager.h
├── src/
│   ├── main.cpp
│   ├── dmx_engine.cpp
│   ├── config_manager.cpp
│   └── scene_manager.cpp
└── platformio.ini
```

## Command Palette Shortcuts

Open with **Ctrl+Shift+P** then type:

- `PlatformIO: Build` - Compile
- `PlatformIO: Upload and Monitor` - Upload + watch serial
- `PlatformIO: Serial Monitor` - View output
- `PlatformIO: Clean` - Delete build files

---

**You're ready to go!** Just click Upload and watch your DMX system come to life! 🚀
