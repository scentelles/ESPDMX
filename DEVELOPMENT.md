# Development Guide

## Project Architecture

### Frontend Architecture

```
src/
├── App.tsx          # Main app component with routing
├── main.tsx         # React DOM entry point
├── index.css        # Global styles
├── components/
│   ├── ui.tsx       # Base UI components (Button, Card, Input, etc.)
│   ├── modals.tsx   # Modal dialogs
│   └── index.tsx    # Feature components (SceneGrid, ShowGrid, etc.)
├── pages/
│   ├── UserPage.tsx  # User-facing lighting control interface
│   └── AdminPage.tsx # Admin configuration interface
├── services/
│   └── api.ts       # API client for backend communication
├── store/
│   └── index.ts     # Zustand state management
└── types/
    └── index.ts     # TypeScript type definitions
```

### Frontend Tech Stack

- **React 18**: UI framework with hooks
- **TypeScript**: Type-safe JavaScript
- **Zustand**: Lightweight state management
- **Axios**: HTTP client for API calls
- **TailwindCSS**: Utility-first CSS framework
- **Lucide React**: Icon library
- **Vite**: Build tool and dev server

### Backend Architecture

```
firmware/
├── src/
│   ├── main.cpp          # Entry point and main loop
│   ├── dmx_engine.cpp    # DMX protocol implementation
│   ├── config_manager.cpp # Configuration storage/retrieval
│   └── scene_manager.cpp  # Scene and show management
├── include/
│   ├── config.h          # Configuration constants
│   ├── dmx_engine.h      # DMX engine declarations
│   ├── config_manager.h  # Config manager declarations
│   └── scene_manager.h   # Scene manager declarations
├── platformio.ini        # PlatformIO project configuration
└── data/
    └── web/              # Static web files (compiled React app)
```

### Backend Tech Stack

- **Arduino Framework**: ESP32 development
- **PlatformIO**: Project management and build system
- **ESPAsyncWebServer**: Async HTTP web server
- **AsyncTCP**: Async TCP library
- **ArduinoJson**: JSON parsing and generation
- **SPIFFS**: File system for configuration storage

## Development Workflow

### Setting Up Development Environment

```bash
# Clone repository
git clone <repo-url>
cd dmxesp

# Frontend setup
cd frontend
npm install
npm run dev      # Starts dev server on localhost:3000

# Firmware setup (in separate terminal)
cd ../firmware
# Edit include/config.h with your settings
platformio run --target upload
```

### Frontend Development

**Key Files to Edit:**
- `src/pages/UserPage.tsx` - User interface features
- `src/pages/AdminPage.tsx` - Admin panel functionality
- `src/components/ui.tsx` - Reusable UI components
- `src/services/api.ts` - Backend API calls
- `src/store/index.ts` - Application state

**Development Commands:**
```bash
npm run dev      # Start dev server with hot reload
npm run build    # Build for production
npm run lint     # Check code quality
```

**Hot Module Replacement (HMR):**
- Changes to React components automatically reflect in browser
- Preserves component state during edits
- Configured in vite.config.ts

### Firmware Development

**Key Components:**

1. **DMX Engine** (`dmx_engine.*`)
   - Handles DMX512 protocol
   - Manages channel values
   - Implements strobe effect timing
   - Controls RS-485 direction pin

2. **Web Server** (`main.cpp`)
   - HTTP REST API endpoints
   - JSON request/response handling
   - CORS handling for frontend

3. **Configuration Manager** (`config_manager.*`)
   - Persistent storage to SPIFFS
   - WiFi credentials management
   - System configuration defaults

4. **Scene Manager** (`scene_manager.*`)
   - In-memory scene storage
   - Default scene initialization
   - JSON serialization

**Development Commands:**
```bash
# Build firmware
platformio run

# Upload to device
platformio run --target upload

# Monitor serial output
platformio device monitor --baud 115200

# Clean build
platformio run --target clean
```

**Debugging:**
```cpp
// Serial output for debugging
Serial.println("Debug message: " + String(value));
Serial.printf("Format string: %d, %s\n", intVal, strVal);

// Check heap memory
Serial.println("Free memory: " + String(ESP.getFreeHeap()));
```

## Code Style Guidelines

### Frontend (TypeScript/React)

```typescript
// Use functional components with hooks
export const MyComponent: React.FC = () => {
  const [state, setState] = useState<Type>(initialValue);
  
  return (
    <div>Component</div>
  );
};

// Use explicit type annotations
interface Props {
  value: string;
  onchange: (value: string) => void;
}

// Use const for functions
const handleEvent = (e: React.ChangeEvent<HTMLInputElement>) => {
  // Handle
};
```

### Backend (C++)

```cpp
// Use consistent naming
void setChannelValue(uint16_t channel, uint8_t value);

// Include guards
#ifndef CLASS_NAME_H
#define CLASS_NAME_H

class ClassName {
  // ...
};

#endif

// Use const for read-only references
void processData(const uint8_t* data, size_t len);

// Initialize members in constructor
ClassName::ClassName() : member1(0), member2(0) {}
```

## API Documentation

### Endpoints

Base URL: `http://<ESP32_IP>/api`

#### GET /scenes
```json
{
  "scenes": [
    {
      "id": "warm_white",
      "name": "Warm White",
      "description": "Classic warm lighting",
      "color": [255, 180, 100],
      "brightness": 80,
      "icon": "🟡"
    }
  ]
}
```

#### POST /control/scene
```json
{
  "sceneId": "warm_white"
}
```

#### POST /control/color
```json
{
  "fixtureId": "fixture-1",
  "color": [255, 0, 0],
  "brightness": 100
}
```

#### POST /control/strobe
```json
{
  "speed": 5
}
```

#### GET /state
```json
{
  "activeScene": "warm_white",
  "activeAmbiance": null,
  "activeShow": null,
  "strobeActive": false,
  "smokeActive": false,
  "masterBrightness": 100,
  "dmxOutput": [255, 180, 100, ...]
}
```

## Adding New Features

### Adding a New Scene

1. Add to default scenes in `scene_manager.cpp`:
```cpp
scenes.push_back({"new_id", "New Scene", "Description", 255, 0, 0, 100, "🎨"});
```

2. Update user interface to show new scene

### Adding a New API Endpoint

1. Create handler function in `main.cpp`:
```cpp
void handleNewEndpoint(AsyncWebServerRequest* request, ...) {
  // Process request
  request->send(200, "application/json", "{\"status\":\"ok\"}");
}
```

2. Register in `setupServer()`:
```cpp
server.on("/api/new-endpoint", HTTP_GET, handleNewEndpoint);
```

3. Add client method in `api.ts`:
```typescript
async newEndpoint(): Promise<any> {
  const response = await this.api.get('/new-endpoint');
  return response.data;
}
```

### Adding Admin Controls

1. Create new tab in `AdminPage.tsx`
2. Add component for managing resource
3. Integrate with API service
4. Add to admin route navigation

## Testing

### Frontend Testing

```bash
# Manual testing
npm run dev  # Start dev server
# Navigate to http://localhost:3000 in browser

# Test user interface
# - Try all scenes
# - Test brightness slider
# - Test strobe/smoke buttons

# Test admin panel
# - Add fixture
# - Edit scene
# - Change settings
```

### Firmware Testing

1. **Serial Monitor Testing:**
   - Watch boot messages
   - Monitor WiFi connection
   - Check memory usage
   - Observe API call handling

2. **API Testing with curl:**
   ```bash
   # Get scenes
   curl http://<ESP32_IP>/api/scenes
   
   # Activate scene
   curl -X POST http://<ESP32_IP>/api/control/scene \
     -H "Content-Type: application/json" \
     -d '{"sceneId":"warm_white"}'
   
   # Set brightness
   curl -X POST http://<ESP32_IP>/api/control/brightness \
     -H "Content-Type: application/json" \
     -d '{"brightness":50}'
   ```

3. **DMX Output Monitoring:**
   - Use DMX analyzer/meter on output
   - Verify correct channel values
   - Check timing between frames

## Performance Optimization

### Frontend
- Use code splitting for large components
- Lazy load routes with React.lazy()
- Minimize re-renders with useMemo
- Optimize images and assets

### Firmware
- Monitor heap memory in serial output
- Avoid string allocations in loops
- Use DynamicJsonDocument sizing carefully
- Cache frequently accessed config

## Deployment

### Frontend Production Build
```bash
npm run build
# Output: dist/ folder with optimized files
```

### Deploying to ESP32
1. Build frontend: `npm run build`
2. Copy `frontend/dist/*` to `firmware/data/web/`
3. Upload firmware with PlatformIO
4. Access at ESP32 IP address

### OTA Updates (Future)
- Firmware update mechanism not yet implemented
- Would require secure update mechanism
- Planned enhancement for future releases

## Troubleshooting Development Issues

### Frontend issues
- **HMR not working**: Check vite.config.ts proxy settings
- **API calls fail**: Verify ESP32 IP and port
- **Build fails**: Clear node_modules and reinstall

### Firmware issues
- **Upload fails**: Check USB driver, try different port
- **WiFi won't connect**: Verify SSID/password in config.h
- **DMX not outputting**: Check pin configuration matches hardware
- **Memory issues**: Monitor with Serial output, reduce buffers

## Resources

- **ESP32 Documentation**: https://docs.espressif.com/projects/esp-idf/en/latest/
- **DMX512 Standard**: http://www.usitt.org/standards/dmx512
- **React Documentation**: https://react.dev
- **TailwindCSS**: https://tailwindcss.com/docs
- **Arduino Reference**: https://www.arduino.cc/reference/en/

---

**Questions?** Check the main README.md or INSTALLATION.md files.
