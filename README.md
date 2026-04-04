# DMXESP - Professional DMX Lighting Control System

A beautiful, responsive WebApp for controlling DMX lighting fixtures from an ESP32 microcontroller. Perfect for rental businesses, events, weddings, and parties.

## Features

### User Interface
- **Color Scenes**: Pre-configured color palettes for quick mood selection
- **Ambiances**: Curated lighting presets for different event types
- **Dynamic Shows**: Pre-programmed lighting sequences and animations
- **Stroboscope Control**: Red button for strobe effects
- **Smoke Trigger**: Red button for synchronized fog machine control
- **Responsive Design**: Works on desktop, tablet, and mobile devices

### Admin Panel
- **Fixture Configuration**: Define DMX addresses and properties for each light
- **Fixture Definitions**: Manage lighting fixture types and capabilities
- **Channel Mapping**: Assign and configure DMX channels
- **Scene Management**: Create and edit custom color scenes
- **Show Management**: Build custom dynamic light shows
- **System Settings**: Configure network, DMX output, and performance

## Architecture

```
DMXESP/
├── frontend/              # React TypeScript web application
│   ├── src/
│   │   ├── components/   # Reusable UI components
│   │   ├── pages/        # User and Admin pages
│   │   ├── services/     # API communication
│   │   ├── types/        # TypeScript definitions
│   │   └── styles/       # TailwindCSS styling
│   ├── package.json
│   └── tailwind.config.js
└── firmware/              # ESP32 Arduino code
    ├── src/
    │   ├── dmx_engine.cpp      # DMX protocol & rendering
    │   ├── web_server.cpp      # HTTP & WebSocket server
    │   ├── config_manager.cpp  # Configuration storage
    │   └── main.cpp            # Entry point
    ├── include/
    └── platformio.ini
```

## Technology Stack

- **Frontend**: React 18 + TypeScript + TailwindCSS + Vite
- **Backend**: Arduino (PlatformIO) on ESP32
- **Communication**: REST API + WebSocket for real-time control
- **Storage**: SPIFFS (ESP32 file system) for configurations
- **DMX Protocol**: UART/RS-485 module connected to GPIO pins

## Quick Start

### Prerequisites
- ESP32 development board
- RS-485 module for DMX output
- Node.js 16+ and npm
- PlatformIO CLI or VSCode PlatformIO extension

### Frontend Setup
```bash
cd frontend
npm install
npm run dev
```
Access the web interface at `http://<ESP32_IP>:80`

### Firmware Setup
```bash
cd firmware
# Configure your WiFi and DMX settings in src/config.h
platformio run --target upload
```

## Configuration

### DMX Setup
Edit `firmware/src/config.h`:
- `DMX_TXD_PIN`: TX pin for RS-485 module (default: GPIO17)
- `DMX_RXD_PIN`: RX pin for RS-485 module (default: GPIO16)
- `DMX_DE_PIN`: DE pin for RS-485 module (default: GPIO15)

### WiFi Setup
Configure in the admin panel or edit firmware config files.

## API Endpoints

- `GET /api/scenes` - Get all color scenes
- `GET /api/ambiances` - Get all ambiances
- `GET /api/shows` - Get all dynamic shows
- `GET /api/fixtures` - Get fixture list
- `POST /api/control/color` - Set light color
- `POST /api/control/scene` - Activate a scene
- `POST /api/control/strobe` - Trigger stroboscope
- `POST /api/control/smoke` - Trigger smoke machine
- `POST /api/control/show` - Start a dynamic show
- `WS /api/live` - WebSocket for real-time updates

## Admin Panel Features

- Authentication (PIN/Password)
- Fixture management and addressing
- Scene builder with real-time preview
- Show sequence builder
- DMX channel monitor
- Network and system settings
- Backup and restore configurations

## Security

- PIN-based authentication for admin panel
- No internet connectivity required (local network only)
- Secure SPIFFS storage for configurations

## Hardware Requirements

- ESP32 (minimum 4MB PSRAM recommended)
- RS-485 module for DMX output
- Power supply rated for ESP32 + DMX interface
- Ethernet or WiFi for web interface access

## Licensing

MIT License - Free for personal and commercial use

## Support

For issues and feature requests, create an issue in the repository.

---

Built with ❤️ for event professionals
