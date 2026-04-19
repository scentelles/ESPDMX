#include <Arduino.h>
#include <WiFi.h>
#include <esp_freertos_hooks.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "dmx_engine.h"
#include "config_manager.h"
#include "scene_manager.h"
#include "display_manager.h"
#include "audio_manager.h"

// Global objects
AsyncWebServer server(WEB_PORT);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
DMXEngine dmxEngine;
ConfigManager configManager;
SceneManager sceneManager;
DisplayManager displayManager;
AudioManager audioManager;

// System state
struct SystemState {
  String activeScene;
  String activeAmbiance;
  String activeShow;
  bool strobeActive;
  bool smokeActive;
  uint8_t masterBrightness;
  uint32_t uptime;
  uint32_t freeMemory;
} systemState;

// Show playback state
struct ShowPlayback {
  bool running;
  String showId;
  int currentStep;
  uint32_t stepStartTime;
  bool inTransition;
  uint32_t transitionStartTime;
  // Snapshot of DMX values at start of smooth transition (channels 1-512)
  uint8_t prevDmx[513];
  uint8_t targetDmx[513];
  bool smoothActive;  // true when currently doing smooth interpolation
} showPlayback;

// Variables
uint32_t lastUpdate = 0;
uint32_t smokeEndTime = 0;
uint32_t lastDisplayUpdate = 0;

// CPU load measurement (FreeRTOS idle-task based, both cores)
// Self-calibrating: the max idle delta observed becomes the 100% idle reference
volatile uint32_t idleCount0 = 0;
volatile uint32_t idleCount1 = 0;
uint32_t lastCpuCalcTime = 0;
uint32_t prevIdle0 = 0;
uint32_t prevIdle1 = 0;
uint32_t maxIdle0 = 0;
uint32_t maxIdle1 = 0;
uint8_t cpuLoadPercent = 0;

bool idleHook0(void) {
  idleCount0++;
  return false;
}
bool idleHook1(void) {
  idleCount1++;
  return false;
}

// Body buffer for chunked POST requests (AsyncWebServer delivers body in chunks)
String bodyBuffer;

// Forward declarations
void setupWiFi();
void setupServer();
void applyScene(const String& sceneId);
void computeSceneDmx(const String& sceneId, uint8_t* buf);
void updateShowPlayback();
void handleNotFound(AsyncWebServerRequest* request);
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
              AwsEventType type, void* arg, uint8_t* data, size_t len);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Register idle hooks for CPU load measurement (both cores)
  esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
  esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
  
  Serial.println("\n\nDMXESP - Professional DMX Lighting Controller");
  Serial.println("Initializing...");
  
  // Initialize display and show boot logo
  displayManager.begin();
  displayManager.showBootLogo();
  delay(500);
  
  // Initialize SPIFFS - ONLY call this once!
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    displayManager.update();
    delay(3000);
    ESP.restart();  // Restart if SPIFFS fails
    return;
  }
  
  Serial.println("SPIFFS initialized successfully");
  
  // Initialize managers (configManager and sceneManager do NOT call SPIFFS.begin())
  configManager.begin();
  sceneManager.begin();
  
  // Initialize DMX engine - register fixtures
  dmxEngine.begin();
  int fixtureCount = 0;
  for (const auto& f : sceneManager.getFixtures()) {
    if (f.enabled) {
      dmxEngine.addFixture(0, f.dmxAddress, f.channelCount);
      fixtureCount++;
      Serial.printf("  Fixture '%s' [%s] addr=%d ch=%d\n", 
        f.name.c_str(), f.id.c_str(), f.dmxAddress, f.channelCount);
      for (const auto& ch : f.channels) {
        Serial.printf("    ch '%s' offset=%d type=%s\n", 
          ch.name.c_str(), ch.offset, ch.type.c_str());
      }
    }
  }
  
  Serial.println("DMX Engine initialized with " + String(fixtureCount) + " fixtures");
  
  // Initialize system state
  systemState.masterBrightness = 100;
  systemState.strobeActive = false;
  systemState.smokeActive = false;
  
  // Initialize audio manager (I2S mic)
  audioManager.begin();
  Serial.println("Setup complete!");
  
  // Initialize show playback
  showPlayback.running = false;
  showPlayback.currentStep = -1;
  
  // Ensure boot logo is visible for a minimum time
  uint32_t bootStart = millis();
  while (millis() - bootStart < TFT_BOOT_DURATION) {
    delay(10);
  }
  
  // Setup WiFi (display updated inside setupWiFi)
  setupWiFi();
  
  // Setup web server
  setupServer();
  
  // WebSocket for real-time DMX control
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.begin();
  Serial.println("Web server started on port " + String(WEB_PORT));
  
  // Show ready screen
  bool apMode = (WiFi.getMode() == WIFI_AP);
  String ip = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  displayManager.showReady(ip, apMode);
  delay(2000); // Show ready screen briefly
  
  // Reset CPU measurement counters so first delta in loop() is clean
  prevIdle0 = idleCount0;
  prevIdle1 = idleCount1;
  lastCpuCalcTime = millis();
}

void loop() {
  // Process DNS requests (captive portal in AP mode)
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }

  // DMX output runs in its own FreeRTOS task — no update() call needed here
  
  // Clean up disconnected WebSocket clients periodically
  ws.cleanupClients();
  
  // Update audio manager and apply sound-reactive effects
  audioManager.update();
  if (audioManager.getMode() != SOUND_OFF) {
    uint8_t r, g, b;
    audioManager.getSoundColor(r, g, b);
    uint8_t brightness = audioManager.getSoundBrightness();
    
    for (const auto& f : sceneManager.getFixtures()) {
      if (!f.enabled) continue;
      for (const auto& ch : f.channels) {
        uint16_t addr = f.dmxAddress + ch.offset;
        if (addr < 1 || addr > 512) continue;
        if (ch.name == "Red") dmxEngine.setChannelValue(addr, (r * brightness) / 255);
        else if (ch.name == "Green") dmxEngine.setChannelValue(addr, (g * brightness) / 255);
        else if (ch.name == "Blue") dmxEngine.setChannelValue(addr, (b * brightness) / 255);
        else if (ch.name == "White") dmxEngine.setChannelValue(addr, (min(r, min(g, b)) * brightness) / 255);
        else if (ch.type == "dimmer") dmxEngine.setChannelValue(addr, brightness);
      }
    }
  }
  
  // Update show playback
  updateShowPlayback();
  
  // Handle smoke timeout
  if (systemState.smokeActive && millis() > smokeEndTime) {
    systemState.smokeActive = false;
  }
  
  // Update system state
  systemState.uptime = millis() / 1000;
  systemState.freeMemory = ESP.getFreeHeap();
  
  // CPU load measurement (idle-task based, self-calibrating)
  if (millis() - lastCpuCalcTime >= 1000) {
    uint32_t curIdle0 = idleCount0;
    uint32_t curIdle1 = idleCount1;
    uint32_t delta0 = curIdle0 - prevIdle0;
    uint32_t delta1 = curIdle1 - prevIdle1;
    prevIdle0 = curIdle0;
    prevIdle1 = curIdle1;
    lastCpuCalcTime = millis();

    // Self-calibrate: highest idle count seen = 100% idle (0% CPU)
    if (delta0 > maxIdle0) maxIdle0 = delta0;
    if (delta1 > maxIdle1) maxIdle1 = delta1;

    uint8_t load0 = (maxIdle0 > 0) ? 100 - min(100UL, delta0 * 100UL / maxIdle0) : 0;
    uint8_t load1 = (maxIdle1 > 0) ? 100 - min(100UL, delta1 * 100UL / maxIdle1) : 0;
    cpuLoadPercent = (load0 + load1) / 2;
  }
  
  // Update OLED display (50ms for smooth audio visualizer, 500ms otherwise)
  uint32_t displayInterval = (audioManager.getMode() != SOUND_OFF) ? 50 : 500;
  if (millis() - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = millis();
    
    // Debug: print DMX channels 1-10 occasionally (every ~2 seconds)
    static uint32_t lastDmxLog = 0;
    if (millis() - lastDmxLog >= 2000) {
      lastDmxLog = millis();
      Serial.printf("DMX[1-10]: %d %d %d %d %d %d %d %d %d %d\n",
        dmxEngine.getChannelValue(1), dmxEngine.getChannelValue(2),
        dmxEngine.getChannelValue(3), dmxEngine.getChannelValue(4),
        dmxEngine.getChannelValue(5), dmxEngine.getChannelValue(6),
        dmxEngine.getChannelValue(7), dmxEngine.getChannelValue(8),
        dmxEngine.getChannelValue(9), dmxEngine.getChannelValue(10));
    }
    
    DisplayStatus dStatus;
    dStatus.wifiConnected = (WiFi.status() == WL_CONNECTED);
    dStatus.apMode = (WiFi.getMode() == WIFI_AP);
    dStatus.ipAddress = dStatus.apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    dStatus.fixtureCount = sceneManager.getFixtures().size();
    dStatus.enabledFixtures = 0;
    for (const auto& f : sceneManager.getFixtures()) {
      if (f.enabled) dStatus.enabledFixtures++;
    }
    
    // Resolve scene/show names
    Scene* activeScene = systemState.activeScene.length() > 0 ? sceneManager.getScene(systemState.activeScene) : nullptr;
    Show* activeShow = systemState.activeShow.length() > 0 ? sceneManager.getShow(systemState.activeShow) : nullptr;
    dStatus.activeScene = activeScene ? activeScene->name : "";
    dStatus.activeShow = activeShow ? activeShow->name : "";
    dStatus.showRunning = showPlayback.running;
    
    dStatus.masterBrightness = systemState.masterBrightness;
    dStatus.strobeActive = systemState.strobeActive;
    dStatus.smokeActive = systemState.smokeActive;
    dStatus.dmxActive = true;
    dStatus.uptime = systemState.uptime;
    dStatus.freeMemory = systemState.freeMemory;
    dStatus.cpuLoad = cpuLoadPercent;
    dStatus.soundMode = (uint8_t)audioManager.getMode();
    AudioData ad = audioManager.getData();
    dStatus.soundVolume = (uint8_t)(ad.volume * 100);
    dStatus.soundBass = (uint8_t)(ad.bass * 100);
    dStatus.soundMid = (uint8_t)(ad.mid * 100);
    dStatus.soundHigh = (uint8_t)(ad.high * 100);
    dStatus.soundPeak = (uint8_t)(ad.peak * 100);
    displayManager.showStatus(dStatus);
  }
  
  displayManager.update();
  delay(10);
}

// ── Compute scene DMX values into a buffer (without applying) ────────
void computeSceneDmx(const String& sceneId, uint8_t* buf) {
  Scene* scene = sceneManager.getScene(sceneId);
  if (!scene) return;
  
  for (const auto& fv : scene->fixtureValues) {
    FixtureDef* fixture = sceneManager.getFixture(fv.fixtureId);
    if (!fixture || !fixture->enabled) continue;
    
    for (const auto& ch : fixture->channels) {
      auto it = fv.values.find(ch.name);
      uint8_t value;
      if (it != fv.values.end()) {
        value = it->second;
      } else if (ch.type == "dimmer") {
        value = 255;
      } else {
        value = ch.defaultValue;
      }
      uint16_t dmxAddr = fixture->dmxAddress + ch.offset;
      if (dmxAddr >= 1 && dmxAddr <= 512) {
        buf[dmxAddr] = value;
      }
    }
  }
}

// ── Apply a scene to DMX output ──────────────────────────────────────
void applyScene(const String& sceneId) {
  Scene* scene = sceneManager.getScene(sceneId);
  if (!scene) {
    Serial.println("applyScene: scene '" + sceneId + "' not found!");
    return;
  }
  
  Serial.println("applyScene: '" + scene->name + "' (" + sceneId + ")");
  
  for (const auto& fv : scene->fixtureValues) {
    FixtureDef* fixture = sceneManager.getFixture(fv.fixtureId);
    if (!fixture || !fixture->enabled) {
      Serial.println("  fixture '" + fv.fixtureId + "' not found or disabled");
      continue;
    }
    
    Serial.printf("  fixture '%s' addr=%d channels=%d\n", 
      fixture->name.c_str(), fixture->dmxAddress, fixture->channels.size());
    
    for (const auto& ch : fixture->channels) {
      auto it = fv.values.find(ch.name);
      uint8_t value;
      if (it != fv.values.end()) {
        value = it->second;
      } else if (ch.type == "dimmer") {
        value = 255;
      } else {
        value = ch.defaultValue;
      }
      uint16_t dmxAddr = fixture->dmxAddress + ch.offset;
      Serial.printf("    ch '%s' offset=%d -> DMX[%d] = %d\n",
        ch.name.c_str(), ch.offset, dmxAddr, value);
      if (dmxAddr >= 1 && dmxAddr <= 512) {
        dmxEngine.setChannelValue(dmxAddr, value);
      }
    }
  }
  
  systemState.activeScene = sceneId;
}

// ── Show playback ────────────────────────────────────────────────────
void updateShowPlayback() {
  if (!showPlayback.running) return;
  
  Show* show = sceneManager.getShow(showPlayback.showId);
  if (!show || show->steps.empty()) {
    showPlayback.running = false;
    return;
  }
  
  uint32_t now = millis();
  ShowStep& step = show->steps[showPlayback.currentStep];
  uint32_t elapsed = now - showPlayback.stepStartTime;
  
  // During transition phase
  if (showPlayback.inTransition && step.transitionTime > 0) {
    if (elapsed < step.transitionTime) {
      // Smooth crossfade: interpolate DMX values between prev and target
      if (showPlayback.smoothActive) {
        float t = (float)elapsed / (float)step.transitionTime;
        for (int i = 1; i <= 512; i++) {
          uint8_t v = (uint8_t)((1.0f - t) * showPlayback.prevDmx[i] + t * showPlayback.targetDmx[i]);
          dmxEngine.setChannelValue(i, v);
        }
      }
      // If not smooth, scene was already applied at transition start
      return;
    } else {
      // Transition complete, apply target fully and start hold
      showPlayback.inTransition = false;
      showPlayback.smoothActive = false;
      showPlayback.stepStartTime = now;
      applyScene(step.sceneId);
      return;
    }
  }
  
  // During hold phase
  if (elapsed >= step.duration) {
    // Move to next step
    showPlayback.currentStep++;
    if (showPlayback.currentStep >= (int)show->steps.size()) {
      if (show->loop) {
        showPlayback.currentStep = 0;
      } else {
        showPlayback.running = false;
        show->isRunning = false;
        systemState.activeShow = "";
        return;
      }
    }
    
    ShowStep& nextStep = show->steps[showPlayback.currentStep];
    showPlayback.stepStartTime = now;
    showPlayback.inTransition = true;
    showPlayback.smoothActive = false;
    
    if (nextStep.transitionTime == 0) {
      // No transition — apply immediately
      showPlayback.inTransition = false;
      applyScene(nextStep.sceneId);
    } else if (nextStep.smoothTransition) {
      // Smooth transition: snapshot current DMX state, compute target
      showPlayback.smoothActive = true;
      for (int i = 1; i <= 512; i++) {
        showPlayback.prevDmx[i] = dmxEngine.getChannelValue(i);
      }
      memset(showPlayback.targetDmx, 0, sizeof(showPlayback.targetDmx));
      computeSceneDmx(nextStep.sceneId, showPlayback.targetDmx);
    } else {
      // Hard cut at start of transition (original behavior)
      applyScene(nextStep.sceneId);
    }
  }
}

// ── WiFi ─────────────────────────────────────────────────────────────
void setupWiFi() {
  SystemConfig config = configManager.getConfig();
  
  Serial.println("Connecting to WiFi: " + String(config.wifiSSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    displayManager.showWiFiConnecting(String(config.wifiSSID), attempts, 20);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect to WiFi. Using AP mode.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(config.wifiSSID, config.wifiPassword);
    Serial.println("AP SSID: " + String(config.wifiSSID));
    Serial.println("AP IP: " + WiFi.softAPIP().toString());
    
    // Start DNS server for captive portal (redirect all domains to ESP32)
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("Captive portal DNS started");
  }
}

// ── Web Server Routes ────────────────────────────────────────────────
void setupServer() {
  // ── GET Endpoints ──
  
  server.on("/api/fixtures", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getFixturesJSON());
  });
  
  server.on("/api/scenes", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getScenesJSON());
  });
  
  server.on("/api/shows", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getShowsJSON());
  });
  
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(1024);
    doc["activeScene"] = systemState.activeScene;
    doc["activeAmbiance"] = systemState.activeAmbiance;
    doc["activeShow"] = systemState.activeShow;
    doc["strobeActive"] = systemState.strobeActive;
    doc["smokeActive"] = systemState.smokeActive;
    doc["masterBrightness"] = systemState.masterBrightness;
    doc["soundMode"] = (int)audioManager.getMode();
    doc["soundSensitivity"] = audioManager.getSensitivity();
    
    // Audio data
    AudioData ad = audioManager.getData();
    JsonObject audio = doc.createNestedObject("audio");
    audio["volume"] = (int)(ad.volume * 100);
    audio["bass"] = (int)(ad.bass * 100);
    audio["mid"] = (int)(ad.mid * 100);
    audio["high"] = (int)(ad.high * 100);
    audio["beat"] = ad.beatDetected;
    
    JsonArray dmxOutput = doc.createNestedArray("dmxOutput");
    for (int i = 0; i < 16; i++) {
      dmxOutput.add(dmxEngine.getChannelValue(i + 1));
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(256);
    doc["dmxBaud"] = configManager.getConfig().dmxBaud;
    doc["maxFixtures"] = configManager.getConfig().maxFixtures;
    doc["updateInterval"] = configManager.getConfig().updateInterval;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/api/system/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(256);
    doc["uptime"] = systemState.uptime;
    doc["freeMemory"] = systemState.freeMemory;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // ── POST CRUD: Fixtures ──
  server.on("/api/fixtures", HTTP_POST,
    // Request handler — called when body is fully received
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, bodyBuffer)) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      FixtureDef f;
      f.id = doc["id"].as<String>();
      f.name = doc["name"].as<String>();
      f.type = doc["type"].as<String>();
      f.dmxAddress = doc["dmxAddress"] | 1;
      f.enabled = doc["enabled"] | true;
      
      JsonArray chArr = doc["channels"];
      uint8_t maxOffset = 0;
      for (JsonObject chObj : chArr) {
        ChannelDef ch;
        ch.name = chObj["name"].as<String>();
        ch.offset = chObj["offset"] | 0;
        ch.defaultValue = chObj["defaultValue"] | 0;
        ch.type = chObj["type"].as<String>();
        if (ch.offset > maxOffset) maxOffset = ch.offset;
        f.channels.push_back(ch);
      }
      // channelCount = DMX footprint (highest offset + 1)
      f.channelCount = f.channels.empty() ? 1 : (maxOffset + 1);
      
      // Parse strobe channel config
      if (doc.containsKey("strobeChannel")) {
        JsonObject sc = doc["strobeChannel"];
        f.strobeChannel.enabled = sc["enabled"] | false;
        f.strobeChannel.offset = sc["offset"] | 0;
        f.strobeChannel.value = sc["value"] | 255;
      } else {
        f.strobeChannel = {false, 0, 255};
      }
      
      sceneManager.saveFixture(f);
      
      // Log saved fixture channel mapping for debugging
      Serial.printf("Fixture saved: '%s' addr=%d channels=%d\n",
        f.name.c_str(), f.dmxAddress, f.channels.size());
      for (const auto& ch : f.channels) {
        Serial.printf("  ch '%s' offset=%d type=%s\n",
          ch.name.c_str(), ch.offset, ch.type.c_str());
      }
      
      // Re-register fixtures on DMX engine
      dmxEngine.clearFixtures();
      int fixtureCount = 0;
      for (const auto& fx : sceneManager.getFixtures()) {
        if (fx.enabled) {
          dmxEngine.addFixture(0, fx.dmxAddress, fx.channelCount);
          fixtureCount++;
        }
      }
      Serial.println("Fixtures updated: " + String(fixtureCount) + " active");
      
      // Re-apply the active scene so DMX output reflects the new channel mapping
      if (systemState.activeScene.length() > 0) {
        Serial.println("Re-applying active scene after fixture update");
        applyScene(systemState.activeScene);
      }
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    }, NULL,
    // Body handler — accumulate chunks
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── DELETE: Fixtures ──
  server.on("/api/fixtures/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    String id = url.substring(lastSlash + 1);
    
    if (sceneManager.deleteFixture(id)) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(404, "application/json", "{\"error\":\"Fixture not found\"}");
    }
  });
  
  // ── POST CRUD: Scenes ──
  server.on("/api/scenes", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(8192);
      if (deserializeJson(doc, bodyBuffer)) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      Scene s;
      s.id = doc["id"].as<String>();
      s.name = doc["name"].as<String>();
      s.description = doc["description"].as<String>();
      s.icon = doc["icon"].as<String>();
      
      JsonArray fvArr = doc["fixtureValues"];
      for (JsonObject fvObj : fvArr) {
        FixtureChannelValues fv;
        fv.fixtureId = fvObj["fixtureId"].as<String>();
        JsonObject vals = fvObj["values"];
        for (JsonPair kv : vals) {
          fv.values[String(kv.key().c_str())] = kv.value().as<uint8_t>();
        }
        s.fixtureValues.push_back(fv);
      }
      
      sceneManager.saveScene(s);
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── DELETE: Scenes ──
  server.on("/api/scenes/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    String id = url.substring(lastSlash + 1);
    
    if (sceneManager.deleteScene(id)) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
    }
  });
  
  // ── POST CRUD: Shows ──
  server.on("/api/shows", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(4096);
      if (deserializeJson(doc, bodyBuffer)) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      Show s;
      s.id = doc["id"].as<String>();
      s.name = doc["name"].as<String>();
      s.description = doc["description"].as<String>();
      s.icon = doc["icon"].as<String>();
      s.loop = doc["loop"] | true;
      s.isRunning = false;
      
      JsonArray stepsArr = doc["steps"];
      for (JsonObject stepObj : stepsArr) {
        ShowStep step;
        step.sceneId = stepObj["sceneId"].as<String>();
        step.duration = stepObj["duration"] | 5000;
        step.transitionTime = stepObj["transitionTime"] | 1000;
        step.smoothTransition = stepObj["smoothTransition"] | false;
        s.steps.push_back(step);
      }
      
      sceneManager.saveShow(s);
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── DELETE: Shows ──
  server.on("/api/shows/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    String id = url.substring(lastSlash + 1);
    
    if (sceneManager.deleteShow(id)) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(404, "application/json", "{\"error\":\"Show not found\"}");
    }
  });
  
  // ── POST: Config ──
  server.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(512);
      if (deserializeJson(doc, bodyBuffer)) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      SystemConfig cfg = configManager.getConfig();
      if (doc.containsKey("wifiSSID")) strlcpy(cfg.wifiSSID, doc["wifiSSID"] | "", sizeof(cfg.wifiSSID));
      if (doc.containsKey("wifiPassword")) strlcpy(cfg.wifiPassword, doc["wifiPassword"] | "", sizeof(cfg.wifiPassword));
      if (doc.containsKey("adminPin")) strlcpy(cfg.adminPin, doc["adminPin"] | "", sizeof(cfg.adminPin));
      if (doc.containsKey("dmxBaud")) cfg.dmxBaud = doc["dmxBaud"];
      if (doc.containsKey("updateInterval")) cfg.updateInterval = doc["updateInterval"];
      configManager.setConfig(cfg);
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Activate Scene ──
  server.on("/api/control/scene", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer) && doc.containsKey("sceneId")) {
        String sceneId = doc["sceneId"].as<String>();
        // Stop any running show
        if (showPlayback.running) {
          Show* oldShow = sceneManager.getShow(showPlayback.showId);
          if (oldShow) oldShow->isRunning = false;
          showPlayback.running = false;
          systemState.activeShow = "";
        }
        applyScene(sceneId);
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Start Show ──
  server.on("/api/control/show", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer) && doc.containsKey("showId")) {
        String showId = doc["showId"].as<String>();
        Show* show = sceneManager.getShow(showId);
        if (show && !show->steps.empty()) {
          // Stop any current show
          if (showPlayback.running) {
            Show* oldShow = sceneManager.getShow(showPlayback.showId);
            if (oldShow) oldShow->isRunning = false;
          }
          
          show->isRunning = true;
          showPlayback.running = true;
          showPlayback.showId = showId;
          showPlayback.currentStep = 0;
          showPlayback.stepStartTime = millis();
          showPlayback.inTransition = (show->steps[0].transitionTime > 0);
          
          systemState.activeShow = showId;
          applyScene(show->steps[0].sceneId);
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Stop Show ──
  server.on("/api/control/show-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      if (showPlayback.running) {
        Show* show = sceneManager.getShow(showPlayback.showId);
        if (show) show->isRunning = false;
        showPlayback.running = false;
        systemState.activeShow = "";
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Color ──
  server.on("/api/control/color", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer) && doc.containsKey("color") && doc.containsKey("brightness")) {
        JsonArray color = doc["color"];
        uint8_t r = color[0], g = color[1], b = color[2];
        uint8_t brightness = doc["brightness"];
        uint8_t w = min(r, min(g, b)); // derive white from common component
        
        // Apply to all enabled fixtures, respecting channel layout
        for (const auto& f : sceneManager.getFixtures()) {
          if (!f.enabled) continue;
          for (const auto& ch : f.channels) {
            uint16_t addr = f.dmxAddress + ch.offset;
            if (addr < 1 || addr > 512) continue;
            if (ch.name == "Red")        dmxEngine.setChannelValue(addr, (r * brightness) / 255);
            else if (ch.name == "Green") dmxEngine.setChannelValue(addr, (g * brightness) / 255);
            else if (ch.name == "Blue")  dmxEngine.setChannelValue(addr, (b * brightness) / 255);
            else if (ch.name == "White") dmxEngine.setChannelValue(addr, (w * brightness) / 255);
            else if (ch.type == "dimmer") dmxEngine.setChannelValue(addr, brightness);
          }
        }
        dmxEngine.setMasterBrightness(brightness);
        systemState.masterBrightness = brightness;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Strobe ──
  server.on("/api/control/strobe", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer) && doc.containsKey("speed")) {
        dmxEngine.setStrobeSpeed(doc["speed"]);
        dmxEngine.setStrobeActive(true);
        systemState.strobeActive = true;
        // Set strobe channel DMX value for fixtures that have it
        for (const auto& f : sceneManager.getFixtures()) {
          if (f.enabled && f.strobeChannel.enabled) {
            uint16_t addr = f.dmxAddress + f.strobeChannel.offset;
            if (addr >= 1 && addr <= 512) {
              dmxEngine.setChannelValue(addr, f.strobeChannel.value);
            }
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Strobe Stop ──
  server.on("/api/control/strobe-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      dmxEngine.setStrobeActive(false);
      systemState.strobeActive = false;
      // Reset strobe channel DMX value to 0 for fixtures that have it
      for (const auto& f : sceneManager.getFixtures()) {
        if (f.enabled && f.strobeChannel.enabled) {
          uint16_t addr = f.dmxAddress + f.strobeChannel.offset;
          if (addr >= 1 && addr <= 512) {
            dmxEngine.setChannelValue(addr, 0);
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Smoke ──
  server.on("/api/control/smoke", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer) && doc.containsKey("duration")) {
        uint32_t duration = doc["duration"];
        systemState.smokeActive = true;
        smokeEndTime = millis() + duration;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Smoke Stop ──
  server.on("/api/control/smoke-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      systemState.smokeActive = false;
      smokeEndTime = 0;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Brightness ──
  server.on("/api/control/brightness", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer) && doc.containsKey("brightness")) {
        uint8_t brightness = doc["brightness"];
        dmxEngine.setMasterBrightness(brightness);
        systemState.masterBrightness = brightness;
        
        // Update all Dimmer channels so brightness takes effect immediately
        for (const auto& f : sceneManager.getFixtures()) {
          if (!f.enabled) continue;
          for (const auto& ch : f.channels) {
            if (ch.type == "dimmer") {
              uint16_t addr = f.dmxAddress + ch.offset;
              if (addr >= 1 && addr <= 512) {
                dmxEngine.setChannelValue(addr, brightness);
              }
            }
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Sound Mode ──
  server.on("/api/control/sound", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, bodyBuffer)) {
        if (doc.containsKey("mode")) {
          int mode = doc["mode"];
          if (mode >= 0 && mode <= 4) {
            audioManager.setMode((SoundMode)mode);
            // Stop show when entering sound mode
            if (mode != 0 && showPlayback.running) {
              Show* show = sceneManager.getShow(showPlayback.showId);
              if (show) show->isRunning = false;
              showPlayback.running = false;
              systemState.activeShow = "";
            }
          }
        }
        if (doc.containsKey("sensitivity")) {
          audioManager.setSensitivity(doc["sensitivity"]);
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // ── Control: Sound Stop ──
  server.on("/api/control/sound-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      audioManager.setMode(SOUND_OFF);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Audio: Real-time levels (GET) ──
  server.on("/api/audio", HTTP_GET, [](AsyncWebServerRequest* request) {
    AudioData ad = audioManager.getData();
    DynamicJsonDocument doc(512);
    doc["volume"] = (int)(ad.volume * 100);
    doc["bass"] = (int)(ad.bass * 100);
    doc["mid"] = (int)(ad.mid * 100);
    doc["high"] = (int)(ad.high * 100);
    doc["peak"] = (int)(ad.peak * 100);
    doc["beat"] = ad.beatDetected;
    doc["mode"] = (int)audioManager.getMode();
    doc["sensitivity"] = ad.sensitivity;
    
    // Raw diagnostic
    int32_t rawMin, rawMax;
    float diagRms;
    audioManager.readDiagnostic(rawMin, rawMax, diagRms);
    JsonObject diag = doc.createNestedObject("diag");
    diag["rawMin"] = rawMin;
    diag["rawMax"] = rawMax;
    diag["rawRms"] = (int)(diagRms * 10000);  // scaled x10000
    diag["rawSpan"] = rawMax - rawMin;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ── Control: Direct DMX channel (test sliders) ──
  server.on("/api/control/dmx", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, bodyBuffer);
      if (doc.containsKey("channel") && doc.containsKey("value")) {
        int channel = doc["channel"];
        int value = doc["value"];
        if (channel >= 1 && channel <= 512 && value >= 0 && value <= 255) {
          dmxEngine.setChannelValue(channel, value);
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
    });
  
  // Captive portal detection handlers (must be before serveStatic)
  // Apple
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  // Android
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  // Windows
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  // Firefox
  server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  // Microsoft redirect
  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });

  // Serve web interface
  server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html");
  
  // 404 handler - redirect non-API requests to main page (captive portal)
  server.onNotFound(handleNotFound);
}

// ── WebSocket event handler for real-time DMX control ──────────────
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      Serial.printf("WS recv: %.*s\n", (int)len, (char*)data);
      // Parse JSON: {"dmx":{channel:value,...}} or {"ch":channel,"val":value}
      StaticJsonDocument<512> doc;
      if (deserializeJson(doc, data, len)) return;
      
      if (doc.containsKey("dmx")) {
        JsonObject dmx = doc["dmx"];
        for (JsonPair kv : dmx) {
          int channel = atoi(kv.key().c_str());
          int value = kv.value().as<int>();
          if (channel >= 1 && channel <= 512 && value >= 0 && value <= 255) {
            Serial.printf("WS DMX: ch%d=%d\n", channel, value);
            dmxEngine.setChannelValue(channel, value);
          }
        }
      } else if (doc.containsKey("ch") && doc.containsKey("val")) {
        int channel = doc["ch"];
        int value = doc["val"];
        if (channel >= 1 && channel <= 512 && value >= 0 && value <= 255) {
          Serial.printf("WS DMX: ch%d=%d\n", channel, value);
          dmxEngine.setChannelValue(channel, value);
        }
      }
    }
  }
}

void handleNotFound(AsyncWebServerRequest* request) {
  // In AP mode, redirect non-API requests to main page (captive portal)
  if (WiFi.getMode() == WIFI_AP && !request->url().startsWith("/api/")) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    return;
  }
  request->send(404, "application/json", "{\"error\":\"Not found\"}");
}
