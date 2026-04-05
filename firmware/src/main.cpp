#include <Arduino.h>
#include <WiFi.h>
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
} showPlayback;

// Variables
uint32_t lastUpdate = 0;
uint32_t smokeEndTime = 0;
uint32_t lastDisplayUpdate = 0;

// CPU load measurement
volatile uint32_t loopCounter = 0;
uint32_t lastCpuCalcTime = 0;
uint32_t loopsPerSecond = 0;
const uint32_t MAX_LOOPS_PER_SEC = 100;  // calibrated baseline (with delay(10))
uint8_t cpuLoadPercent = 0;

// Forward declarations
void setupWiFi();
void setupServer();
void applyScene(const String& sceneId);
void applySceneValues(Scene* scene, float blend = 1.0f, Scene* fromScene = nullptr);
void updateShowPlayback();
void handleNotFound(AsyncWebServerRequest* request);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
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
  while (millis() - bootStart < OLED_BOOT_DURATION) {
    delay(10);
  }
  
  // Setup WiFi (display updated inside setupWiFi)
  setupWiFi();
  
  // Setup web server
  setupServer();
  
  server.begin();
  Serial.println("Web server started on port " + String(WEB_PORT));
  
  // Show ready screen
  bool apMode = (WiFi.getMode() == WIFI_AP);
  String ip = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  displayManager.showReady(ip, apMode);
  delay(2000); // Show ready screen briefly
}

void loop() {
  // Process DNS requests (captive portal in AP mode)
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }

  // Update DMX engine
  dmxEngine.update();
  
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
        else if (ch.name == "Dimmer" || ch.name == "Master") dmxEngine.setChannelValue(addr, brightness);
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
  
  // CPU load measurement: count loops per second
  loopCounter++;
  if (millis() - lastCpuCalcTime >= 1000) {
    loopsPerSecond = loopCounter;
    loopCounter = 0;
    lastCpuCalcTime = millis();
    // More loops = less busy. Baseline ~100 loops/s with delay(10)
    if (loopsPerSecond >= MAX_LOOPS_PER_SEC) {
      cpuLoadPercent = 0;
    } else {
      cpuLoadPercent = 100 - (loopsPerSecond * 100 / MAX_LOOPS_PER_SEC);
    }
  }
  
  // Update OLED display every 500ms
  if (millis() - lastDisplayUpdate >= 500) {
    lastDisplayUpdate = millis();
    
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
    dStatus.soundVolume = (uint8_t)(audioManager.getData().volume * 100);
    
    displayManager.showStatus(dStatus);
  }
  
  displayManager.update();
  delay(10);
}

// ── Apply a scene to DMX output ──────────────────────────────────────
void applyScene(const String& sceneId) {
  Scene* scene = sceneManager.getScene(sceneId);
  if (!scene) return;
  
  for (const auto& fv : scene->fixtureValues) {
    FixtureDef* fixture = sceneManager.getFixture(fv.fixtureId);
    if (!fixture || !fixture->enabled) continue;
    
    for (const auto& ch : fixture->channels) {
      auto it = fv.values.find(ch.name);
      uint8_t value = (it != fv.values.end()) ? it->second : ch.defaultValue;
      uint16_t dmxAddr = fixture->dmxAddress + ch.offset;
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
      // Blend between previous and current scene
      float blend = (float)elapsed / (float)step.transitionTime;
      // For simplicity, just apply current scene (smooth transitions would need
      // interpolation between two scenes' DMX values - implement later)
      applyScene(step.sceneId);
      return;
    } else {
      // Transition complete, apply fully and start hold
      showPlayback.inTransition = false;
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
    showPlayback.stepStartTime = now;
    showPlayback.inTransition = true;
    // Apply next step's scene immediately if no transition
    ShowStep& nextStep = show->steps[showPlayback.currentStep];
    if (nextStep.transitionTime == 0) {
      showPlayback.inTransition = false;
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
  server.on("/api/fixtures", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, data, len)) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      FixtureDef f;
      f.id = doc["id"].as<String>();
      f.name = doc["name"].as<String>();
      f.type = doc["type"].as<String>();
      f.dmxAddress = doc["dmxAddress"] | 1;
      f.channelCount = doc["channelCount"] | 1;
      f.enabled = doc["enabled"] | true;
      
      JsonArray chArr = doc["channels"];
      for (JsonObject chObj : chArr) {
        ChannelDef ch;
        ch.name = chObj["name"].as<String>();
        ch.offset = chObj["offset"] | 0;
        ch.defaultValue = chObj["defaultValue"] | 0;
        ch.type = chObj["type"].as<String>();
        f.channels.push_back(ch);
      }
      
      sceneManager.saveFixture(f);
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
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
  server.on("/api/scenes", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(8192);
      if (deserializeJson(doc, data, len)) {
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
  server.on("/api/shows", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(4096);
      if (deserializeJson(doc, data, len)) {
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
        s.steps.push_back(step);
      }
      
      sceneManager.saveShow(s);
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
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
  server.on("/api/config", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(512);
      if (deserializeJson(doc, data, len)) {
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
    });
  
  // ── Control: Activate Scene ──
  server.on("/api/control/scene", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("sceneId")) {
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
    });
  
  // ── Control: Start Show ──
  server.on("/api/control/show", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("showId")) {
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
  server.on("/api/control/color", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("color") && doc.containsKey("brightness")) {
        JsonArray color = doc["color"];
        uint8_t r = color[0], g = color[1], b = color[2];
        uint8_t brightness = doc["brightness"];
        
        // Apply to first RGB-capable fixture
        for (const auto& f : sceneManager.getFixtures()) {
          if (!f.enabled) continue;
          for (const auto& ch : f.channels) {
            uint16_t addr = f.dmxAddress + ch.offset;
            if (ch.name == "Red") dmxEngine.setChannelValue(addr, r);
            else if (ch.name == "Green") dmxEngine.setChannelValue(addr, g);
            else if (ch.name == "Blue") dmxEngine.setChannelValue(addr, b);
          }
        }
        dmxEngine.setMasterBrightness(brightness);
        systemState.masterBrightness = brightness;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Strobe ──
  server.on("/api/control/strobe", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("speed")) {
        dmxEngine.setStrobeSpeed(doc["speed"]);
        dmxEngine.setStrobeActive(true);
        systemState.strobeActive = true;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Strobe Stop ──
  server.on("/api/control/strobe-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      dmxEngine.setStrobeActive(false);
      systemState.strobeActive = false;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Smoke ──
  server.on("/api/control/smoke", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("duration")) {
        uint32_t duration = doc["duration"];
        systemState.smokeActive = true;
        smokeEndTime = millis() + duration;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Smoke Stop ──
  server.on("/api/control/smoke-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      systemState.smokeActive = false;
      smokeEndTime = 0;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Brightness ──
  server.on("/api/control/brightness", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("brightness")) {
        uint8_t brightness = doc["brightness"];
        dmxEngine.setMasterBrightness(brightness);
        systemState.masterBrightness = brightness;
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  // ── Control: Sound Mode ──
  server.on("/api/control/sound", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len)) {
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
    });
  
  // ── Control: Sound Stop ──
  server.on("/api/control/sound-stop", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      audioManager.setMode(SOUND_OFF);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  // ── Control: Direct DMX channel (test sliders) ──
  server.on("/api/control/dmx", HTTP_POST, handleNotFound, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, data, len);
      if (doc.containsKey("channel") && doc.containsKey("value")) {
        int channel = doc["channel"];
        int value = doc["value"];
        if (channel >= 1 && channel <= 512 && value >= 0 && value <= 255) {
          dmxEngine.setChannelValue(channel, value);
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
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

void handleNotFound(AsyncWebServerRequest* request) {
  // In AP mode, redirect non-API requests to main page (captive portal)
  if (WiFi.getMode() == WIFI_AP && !request->url().startsWith("/api/")) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    return;
  }
  request->send(404, "application/json", "{\"error\":\"Not found\"}");
}
