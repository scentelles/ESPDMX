# DMXESP Quick Start Guide

Get your professional DMX lighting control system up and running in 15 minutes!

## What You'll Need

- ✅ ESP32 development board
- ✅ RS-485 module (MAX485 or similar)
- ✅ USB cable & computer with Node.js installed
- ✅ WiFi network access

## 5-Minute Setup

### Step 1: Prepare Hardware (2 min)
```
RS-485 Module      ESP32 GPIO
    A (TX)  -----> GPIO17
    B (RX)  -----> GPIO16
    DE      -----> GPIO15
    GND     -----> GND
    VCC     -----> 5V (⚠️ via voltage regulator)
```

### Step 2: Configure Firmware (1 min)
Edit `firmware/include/config.h`:
```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
#define ADMIN_PIN "1234"
```

### Step 3: Upload to ESP32 (2 min)
```bash
cd firmware
platformio run --target upload
```

Done! 🎉

## First Test (3 min)

1. **Find ESP32 IP Address**
   - Open your router's device list
   - Or check serial monitor output (115200 baud)
   - It should show: `IP address: 192.168.x.x`

2. **Open Web Interface**
   - Open browser: `http://192.168.x.x`
   - You should see the user interface

3. **Try the Controls**
   - Move brightness slider
   - Click color scenes
   - Try strobe button 👍

## Login to Admin Panel

1. Click the ⚙️ button (bottom right)
2. Enter PIN: `1234`
3. Now you can:
   - Add DMX fixtures
   - Create custom scenes
   - Configure shows
   - Change admin PIN

## My First Fixture

1. **Go to Admin → Fixtures**
2. **Click "+ Add Fixture"**
3. **Fill in:**
   - Name: `Main Lights`
   - Type: `RGB`
   - DMX Address: `1`
   - Channels: `3`
4. **Click Save**
5. **In User Page:**
   - Select a color scene
   - Watch your fixture light up! ✨

## Next Steps

1. **Configure all your fixtures**
   - Each fixture needs unique DMX address
   - Set correct channel count

2. **Create custom scenes**
   - Your brand colors
   - Event-specific looks
   - Different room ambiances

3. **Setup shows**
   - Transitions between colors
   - Timed sequences
   - Music-synchronized patterns

4. **Change security**
   - Admin → Settings
   - Change admin PIN from default
   - Set WiFi password

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Can't reach web interface | Check WiFi connection, verify IP address from serial monitor |
| Fixtures don't respond | Verify RS-485 wiring, check DMX address configuration |
| Can't login to admin | Default PIN is `1234` (if you changed it, use your new PIN) |
| ESP32 won't upload | Try different USB cable, install CH340 drivers, hold BOOT while uploading |

## Performance Tips

- **📊 Max Fixtures**: 64 simultaneous
- **🚀 Update Rate**: 44Hz (smooth animations)
- **💾 Max Scenes**: Unlimited (stored in memory)
- **⚡ Max DMX Channels**: Full 512-channel universe

## Important Notes

⚠️ **Safety First!**
- Test all functions before events
- Warn attendees when using strobe effects
- Provide adequate ventilation for smoke machine
- Keep power supply well-rated (5A recommended)

🔐 **Security:**
- Change admin PIN immediately
- Don't expose on internet (local WiFi only)
- Keep backups of configuration

## Need Help?

- 📖 **Full Guide**: See `USER_GUIDE.md`
- 🛠️ **Installation**: See `INSTALLATION.md`
- 👨‍💻 **Development**: See `DEVELOPMENT.md`
- 📚 **Technical**: See `.github/copilot-instructions.md`

## What Can You Control?

| Feature | User | Admin |
|---------|------|-------|
| Color Scenes | ✅ | ✅ Create/Edit |
| Brightness | ✅ | ✅ Configure |
| Dynamic Shows | ✅ | ✅ Create/Edit |
| Strobe Effects | ✅ | ✅ Configure |
| Smoke Machine | ✅ | ✅ Configure |
| Fixture Config | ❌ | ✅ Full Control |
| System Settings | ❌ | ✅ Full Control |

---

**Congratulations!** Your professional DMX lighting system is ready to go. 🎆

For detailed instructions, see the full documentation in this project.
