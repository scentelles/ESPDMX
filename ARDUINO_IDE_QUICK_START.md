# Arduino IDE: 5-Minute Setup

Start building with Arduino IDE today!

## System Requirements

- **Arduino IDE**: 1.8.19+ or 2.0+
- **USB Cable**: For ESP32 programming
- **Computer**: Windows, Mac, or Linux

## Step-by-Step

### 1. Download & Install Arduino IDE (2 min)
[Download Arduino IDE →](https://www.arduino.cc/en/software)

### 2. Add ESP32 Support (1 min)

**Arduino IDE 1.x:**
1. **File → Preferences**
2. Find "Additional Boards Manager URLs"
3. Paste: `https://dl.espressif.com/dl/package_esp32_index.json`
4. Click OK

**Arduino IDE 2.0:**
1. **File → Preferences**
2. Find "Arduino CLI configuration file"
3. Click the edit icon
4. Add same URL to boardManagerSketchbookPath section

### 3. Install Board Support (1 min)
1. **Tools → Board → Boards Manager**
2. Search: `esp32`
3. Click **Install** (Espressif Systems)
4. Wait for installation

### 4. Install Libraries (1 min)

**Sketch → Include Library → Manage Libraries**

Install these 3 libraries:
1. **ESP Async WebServer** by me-no-dev (v1.2.3+)
2. **AsyncTCP** by me-no-dev (v1.1.1+)
3. **ArduinoJson** by Benoit Blanchon (v6.21.2+)

Search for each and click Install.

## Using with DMXESP

### 1. Open the Sketch
- **File → Open**
- Navigate to: `firmware/DMXESP/DMXESP.ino`
- File opens in Arduino IDE

### 2. Edit WiFi Settings

At the top of DMXESP.ino (lines 6-7):
```cpp
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"
```

Change to your WiFi credentials.

### 3. Configure Board

**Tools menu - Set these:**
- **Board**: ESP32 Dev Module  
- **Upload Speed**: 921600
- **CPU Frequency**: 240MHz
- **Flash Frequency**: 80MHz
- **Flash Mode**: DIO
- **Flash Size**: 4MB 
- **Partition Scheme**: Default 4MB with spiffs
- **PSRAM**: Enabled
- **Port**: COM3 (or your ESP32's COM port)

### 4. Upload Sketch

1. Plug ESP32 into computer via USB
2. Click **Upload** button (right arrow icon)
3. Wait for compilation (takes 30-60 seconds)
4. Should see: **Done uploading**

### 5. Verify

1. **Tools → Serial Monitor**
2. Set baud rate: **115200** (important!)
3. You should see boot messages:
   ```
   === DMXESP - Professional DMX Lighting Controller ===
   ✓ WiFi connected!
   IP address: 192.168.x.x
   ✓ Web server started on port 80
   === System Ready ===
   ```

## Access Web Interface

1. Note the IP address from Serial Monitor
2. Open browser: `http://192.168.x.x`
3. **That's it!** The web interface loads 🎉

## Test It

In the web interface:
- ✅ Move brightness slider (0-100%)
- ✅ Click color scenes (7 presets)
- ✅ Press STROBE button (red button)
- ✅ Press SMOKE button (red button)

## Admin Panel

1. Click ⚙️ button (bottom right)
2. Enter PIN: `1234`
3. You can now:
   - Add DMX fixtures
   - Create custom scenes
   - Configure shows
   - Change admin PIN
   - Adjust system settings

## Troubleshooting

### "Port" doesn't appear in Tools menu
- Check USB cable is working
- Try different USB cable
- Install CH340 driver (for clone boards): [Download →](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- Restart Arduino IDE

### Upload fails with timeout
- Close any other serial monitor windows
- Try different USB port
- Hold BOOT button on ESP32 while uploading
- Try lower upload speed (512000 or 115200)

### Can't find COM port
- Windows: Check Device Manager
- Mac: System Information → USB
- Linux: Run `ls /dev/tty*`

### WiFi won't connect
- Check SSID/password spelling (case-sensitive)
- Verify WiFi is 2.4GHz (not 5GHz only)
- Check signal strength (must see WiFi from that location)
- ESP32 falls back to AP mode at `192.168.4.1`

### Serial Monitor shows garbage
- Wrong baud rate set (should be 115200)
- Bad USB cable
- Try different USB port

## Tips & Tricks

**Keyboard Shortcuts:**
- `Ctrl+U` - Upload
- `Ctrl+Shift+M` - Open Serial Monitor
- `Ctrl+T` - New Tab
- `Ctrl+,'` - Open Preferences

**Before Each Upload:**
1. Close Serial Monitor
2. Make sure ESP32 is powered
3. Give it 2 seconds after plugging in

**Reset ESP32:**
- Press EN (Enable) button on board
- Resets immediately

**Save Your Configuration:**
- Note your WiFi SSID and password
- Note your ESP32 IP address
- Save these for later reference

## Next: Add Fixtures

Once running, add your DMX fixtures:

1. In admin panel → Fixtures tab
2. Click "+ Add Fixture"
3. Enter:
   - Name (e.g., "Main RGB Bar")
   - Type (RGB, RGBW, Dimmer, etc.)
   - DMX Address (1-512, unique for each)
   - Number of channels
4. Click Save

Then test by selecting color scenes!

## Production Deployment

Ready to embed the web app?

1. Build it: `cd frontend && npm run build`
2. Copy files: `cp -r dist/* ../firmware/DMXESP/data/web/`
3. Re-upload firmware
4. Access at ESP32 IP (no more localhost)

## Getting Help

**Documentation:**
- Full setup guide: Read `INSTALLATION.md`
- User guide: Read `USER_GUIDE.md`
- Technical reference: Read `.github/copilot-instructions.md`

**Common Issues:**
- Check the troubleshooting section above
- Search Arduino IDE forums
- Check ESP32 community forums

## You're Ready! 🚀

You now have a professional DMX lighting control system running on your ESP32. Start with the demo scenes and build from there.

**Time to first run: ~10 minutes** ⏱️

**Questions?** Check the full `ARDUINO_IDE_SETUP.md` for more details.

---

**Built with ❤️ for event professionals**
