# Mock Server - Local Development Guide

## Purpose
The mock server simulates the ESP32 API endpoints locally, allowing you to develop and test the frontend without needing to flash the firmware every time you make changes.

## Quick Start

### Option 1: Windows
Double-click `start-mock-server.bat` or run in PowerShell:
```powershell
.\start-mock-server.bat
```

### Option 2: Linux/Mac
Run the shell script:
```bash
chmod +x start-mock-server.sh
./start-mock-server.sh
```

### Option 3: Manual
```bash
# Build the frontend
cd frontend
npm run build
cd ..

# Start the mock server
node mock-server.js
```

## What it does
1. **Builds** the latest React frontend
2. **Starts** a local server on `http://localhost:3000`
3. **Simulates** all ESP32 API endpoints with mock data
4. **Logs** all API calls to the console

## Features
✅ Full API simulation with mock data
✅ Scene activation and management
✅ Show triggering
✅ Color and brightness control
✅ Strobe and smoke effects
✅ System state tracking
✅ No hardware needed

## Mock Endpoints

### Read Operations
- `GET /api/scenes` - Returns 4 mock color scenes
- `GET /api/shows` - Returns 3 mock shows
- `GET /api/fixtures` - Returns 3 mock fixtures
- `GET /api/state` - Returns current system state
- `GET /api/config` - Returns system configuration
- `GET /api/system/status` - Returns uptime and memory

### Control Operations
- `POST /api/control/scene` - Activate a scene
- `POST /api/control/color` - Set RGB color and brightness
- `POST /api/control/show` - Start a show
- `POST /api/control/strobe` - Trigger strobe effect
- `POST /api/control/strobe-stop` - Stop strobe
- `POST /api/control/smoke` - Trigger smoke machine
- `POST /api/control/brightness` - Set brightness

## Example Usage

### Activating a Scene
```curl
POST http://localhost:3000/api/control/scene
Content-Type: application/json

{
  "sceneId": "scene-1"
}
```

### Setting Color
```curl
POST http://localhost:3000/api/control/color
Content-Type: application/json

{
  "color": [255, 100, 50],
  "brightness": 80
}
```

### Triggering Strobe
```curl
POST http://localhost:3000/api/control/strobe
Content-Type: application/json

{
  "speed": 5
}
```

## Testing the UI

Once the server is running:

1. Open http://localhost:3000 in your browser
2. The frontend will load with mock data
3. Test all controls:
   - ✓ Click color scenes to activate them
   - ✓ Adjust brightness slider
   - ✓ Start shows
   - ✓ Trigger strobe and smoke effects
4. Open DevTools (F12) → Console to see API responses logged
5. Check the terminal where mock-server is running to see API calls

## Console Output

The mock server logs all API calls:
```
✓ Scene activated: Red Room
✓ Color set: RGB(255, 100, 50) Brightness: 80
✓ Strobe activated: Speed 5
```

## When to Use Hardware vs Mock Server

### Use Mock Server When:
- Developing UI features
- Testing API responses
- Making CSS/layout changes
- No ESP32 hardware available
- Quick iteration needed

### Flash Hardware When:
- Testing actual DMX output
- Hardware/pins configuration
- Testing WiFi connectivity
- Final validation before deployment

## Switching Between Mock and Hardware

The frontend automatically detects the backend:
- **http://localhost:3000** → Uses mock server
- **http://esp32.local or IP** → Uses real hardware

No code changes needed!

## Modifying Mock Data

Edit `mock-server.js` to change:
- Scene definitions (colors, brightness, icons)
- Show configurations
- Fixture setup
- Mock API responses

Then restart the server with `start-mock-server.bat`

## Troubleshooting

### Port already in use
If port 3000 is busy, edit `mock-server.js`:
```javascript
const PORT = 3001; // Change to any available port
```

### Frontend not building
```bash
cd frontend
npm install  # Reinstall dependencies
npm run build
```

### CORS errors
The mock server includes CORS headers for all origins.

## Performance Notes
- Mock server is much faster than reflashing (no serial upload delay)
- Browser hot-reload possible with `npm run dev` instead of build
- Great for rapid prototyping

## Next Steps
1. Run `start-mock-server.bat`
2. Open http://localhost:3000
3. Start developing! 🚀
