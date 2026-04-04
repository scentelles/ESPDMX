# GitHub Copilot Instructions
# DMXESP - Professional DMX Lighting Control System

This project implements a full-stack DMX lighting control system for ESP32.

## Project Structure

### Frontend (`/frontend`)
- **Technology**: React 18 + TypeScript + TailwindCSS + Vite
- **Two main pages**: User page with controls, Admin panel with configuration
- **Real-time control**: Brightness, color scenes, ambiances, dynamic shows, effects
- **Build output**: Static files ready for SPIFFS deployment

### Firmware (`/firmware`)
- **Technology**: Arduino (PlatformIO) on ESP32
- **DMX Engine**: UART-based DMX512 protocol implementation
- **Web Server**: Async HTTP server with REST API and WebSocket support
- **Configuration**: Persistent storage on SPIFFS file system
- **Managers**: ConfigManager, SceneManager, DMXEngine

## Getting Started

### Frontend Setup
```bash
cd frontend
npm install
npm run dev      # Development with hot reload
npm run build    # Production build
```

### Firmware Setup
```bash
cd firmware
# Edit include/config.h to set WiFi credentials and pin configuration
# Using VSCode with PlatformIO extension:
# - Open command palette
# - Run "PlatformIO: Upload"
# Or using CLI:
platformio run --target upload
```

## Key Features Implemented

✅ Beautiful, responsive React UI with TailwindCSS
✅ DMX512 protocol over RS-485
✅ WiFi web server for remote control
✅ User page: Color scenes, ambiances, dynamic shows, effects
✅ Admin panel: Fixture management, scene builder, show configuration
✅ Strobe and smoke machine triggers
✅ Master brightness control
✅ Persistent configuration storage
✅ Real-time state updates
✅ Default demo scenes and shows

## API Endpoints

All endpoints use `/api` prefix:

### Read Operations
- `GET /api/scenes` - Get all color scenes
- `GET /api/shows` - Get all dynamic shows
- `GET /api/fixtures` - Get configured fixtures
- `GET /api/state` - Get current lighting state
- `GET /api/config` - Get system configuration
- `GET /api/system/status` - Get system uptime and memory

### Control Operations
- `POST /api/control/scene` - Activate a color scene
- `POST /api/control/color` - Set RGB color and brightness
- `POST /api/control/show` - Start a dynamic show
- `POST /api/control/strobe` - Trigger stroboscope effect
- `POST /api/control/strobe-stop` - Stop strobe
- `POST /api/control/smoke` - Trigger smoke machine
- `POST /api/control/brightness` - Set master brightness

## Hardware Connections

### RS-485 Module to ESP32
- RS-485 TX (A) → GPIO17 (DMX_TX_PIN)
- RS-485 RX (B) → GPIO16 (DMX_RX_PIN)
- RS-485 DE (Enable) → GPIO15 (DMX_DE_PIN)
- GND → GND
- 5V → VCC (via voltage regulator)

### Optional I/O
- Strobe trigger relay → GPIO pin (configure in code)
- Smoke machine relay → GPIO pin (configure in code)

## Configuration

Edit `firmware/include/config.h`:
- `WIFI_SSID` / `WIFI_PASSWORD` - WiFi credentials
- `ADMIN_PIN` - Admin panel PIN (change this!)
- `DMX_*_PIN` - DMX interface GPIO pins
- `DMX_BAUD_RATE` - DMX baudrate (standard: 250000)

## Building for Production

### Frontend
```bash
npm run build
# Copy dist folder contents to firmware/data/web/
```

### Firmware
```bash
platformio run --target upload
```

## Deployment Checklist

- [ ] Change default admin PIN in config.h
- [ ] Set correct WiFi credentials
- [ ] Verify RS-485 wiring
- [ ] Configure DMX fixture addresses
- [ ] Test all scenes and shows
- [ ] Verify strobe and smoke trigger outputs
- [ ] Check master brightness range
- [ ] Test on target fixtures

## Performance Notes

- ESP32 with 4MB PSRAM recommended
- React app is ~150KB gzipped
- DMX output: Full 512 channels at 44Hz
- Web interface updates every 500ms by default

## Troubleshooting

**No WiFi connection**: Check SSID/password in config.h, check WiFi signal
**DMX not working**: Verify RS-485 wiring, check UART2 pin configuration
**Admin panel not loading**: Check admin PIN, verify web server is running
**Fixtures not responding**: Check DMX addresses, verify fixture configuration

## Future Enhancements

- Save custom scenes to SPIFFS
- Sequencer for complex lighting programs
- MIDI input support
- DMX input monitoring/debugging
- OTA (Over-The-Air) firmware updates
- WebSocket live feedback
- Advanced color correction
- Fixture library expansion
