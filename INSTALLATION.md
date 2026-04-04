# Installation Guide

## Prerequisites

### Hardware Required
- **ESP32 Development Board** (e.g., ESP32-DevKitC)
  - Minimum: 4MB Flash, 1.25MB PSRAM
  - Recommended: 8MB Flash, 4MB PSRAM
- **RS-485 Module** (for DMX output)
  - MAX485 or similar
  - 3.3V logic level versions recommended
- **USB Cable** (for programming)
- **Power Supply**
  - 5V for ESP32 and RS-485 module
  - Optional: 24V for RS-485 differential pair

### Software Requirements
- **Node.js** 16.0.0 or higher
- **npm** or **yarn**
- **PlatformIO CLI** or **VSCode** with PlatformIO extension
- **Python** 3.7+ (for PlatformIO)

## Setup Instructions

### Step 1: Clone or Download the Project

```bash
git clone https://github.com/yourusername/dmxesp.git
cd dmxesp
```

### Step 2: Configure Firmware

Edit `firmware/include/config.h`:

```cpp
// WiFi Configuration
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Admin PIN (change this!)
#define ADMIN_PIN "1234"

// GPIO Pins (verify against your hardware)
#define DMX_TX_PIN 17
#define DMX_RX_PIN 16
#define DMX_DE_PIN 15
```

### Step 3: Install Frontend Dependencies

```bash
cd frontend
npm install
```

### Step 4: Build Frontend (Development)

For development with hot reload:

```bash
npm run dev
```

The web interface will be available at `http://localhost:3000`

### Step 5: Upload Firmware to ESP32

Using PlatformIO extension in VSCode:
1. Install the PlatformIO extension
2. Open the `firmware` folder
3. Click the upload icon or use `Ctrl+Alt+U`

Using PlatformIO CLI:

```bash
cd firmware
platformio run --target upload
# To monitor output:
platformio device monitor
```

### Step 6: Access the Web Interface

Once firmware is uploaded:

1. Connect to the same WiFi network as the ESP32
2. Open `http://<ESP32_IP>:80` in your browser
3. Find the admin button (⚙️) in the bottom right
4. Enter the admin PIN you configured

To find the ESP32 IP address:
- Check your WiFi router's device list
- Open Serial Monitor at 115200 baud to see boot messages
- Use network discovery tools

## Wiring Diagram

### RS-485 Module to ESP32

```
RS-485 Module     ESP32
    A (TX+)    → GPIO17 (UART2_TX / DMX_TX_PIN)
    B (TX-)    → GPIO16 (UART2_RX / DMX_RX_PIN)
    DE (Enable)→ GPIO15 (DMX_DE_PIN)
    GND        → GND
    VCC        → 5V (through voltage regulator if needed)
```

### Optional: Relay Outputs

For strobe and smoke machine triggers:

```
ESP32 GPIO          Relay Input
GPIO25      → Strobe relay
GPIO26      → Smoke relay
GND         → Relay common
```

## First Run Checklist

- [ ] ESP32 connected via USB
- [ ] RS-485 module properly wired
- [ ] WiFi SSID and password configured
- [ ] Firmware uploaded successfully
- [ ] Web browser can reach ESP32
- [ ] Can log into admin panel with configured PIN
- [ ] Can see demo scenes in user interface
- [ ] Master brightness slider works
- [ ] DMX fixtures respond to color changes

## Troubleshooting

### WiFi Connection Issues

**Problem**: ESP32 continuously fails to connect to WiFi
- Check SSID and password spelling (case-sensitive)
- Verify WiFi network is 2.4GHz (not 5GHz)
- Re-upload firmware with correct credentials

**Problem**: Device appears in AP mode instead of connecting
- Check serial monitor output for error message
- WiFi will default to AP mode if connection fails
- Access via `http://192.168.4.1` in AP mode

### DMX Output Not Working

**Problem**: Fixtures don't respond to commands
- Check RS-485 module wiring with multimeter
- Verify correct GPIO pins in config.h
- Check DMX fixtures are set to correct address
- Use DMX monitor to check signal output

**Problem**: ESP32 resets when DMX transmits
- Check power supply capacity (5A recommended for peaks)
- Add decoupling capacitor (100µF) near RS-485 module
- Verify proper grounding

### Web Interface Not Loading

**Problem**: Cannot reach `http://<ESP32_IP>:80`
- Check ESP32 is actually connected to WiFi
- Try accessing from another device
- Check firewall settings
- Verify web server is running in serial monitor

**Problem**: Admin panel won't open
- Verify PIN is correct (default: "1234")
- Check browser console for errors
- Try clearing browser cache

### Firmware Upload Fails

**Problem**: Upload timeout or permission denied
- Try different USB cable
- Install CH340 drivers if using generic ESP32 boards
- Close serial monitor before uploading
- Hold BOOT button during upload if needed

**Problem**: Python not found
- Install Python 3.7+
- Add Python to system PATH
- Restart VSCode after Python installation

## Next Steps

1. **Add DMX Fixtures**
   - Go to Admin Panel → Fixtures
   - Add fixture name, type, DMX address
   - Test with color scenes

2. **Create Custom Scenes**
   - Admin Panel → Scenes
   - Set RGB colors and brightness
   - Add descriptive name and emoji icon

3. **Configure Shows**
   - Admin Panel → Shows
   - Create dynamic lighting sequences
   - Set animation duration

4. **Set Security**
   - Admin Panel → Settings
   - Change admin PIN from default
   - Configure WiFi password

5. **Deploy to Production**
   - Build React app: `npm run build`
   - Copy `frontend/dist` to `firmware/data/web`
   - Re-upload firmware
   - Access at ESP32 IP without development server

## Support

For issues or questions:
- Check troubleshooting section above
- Review `.github/copilot-instructions.md`
- Check serial monitor output (`115200 baud`)
- Create GitHub issue with:
  - Error message
  - Serial monitor output
  - Your configuration
  - Steps to reproduce
