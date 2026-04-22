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
  uint8_t prevDmx[513];
  uint8_t targetDmx[513];
  bool smoothActive;
} showPlayback;

// Variables
uint32_t lastUpdate = 0;
uint32_t smokeEndTime = 0;
uint32_t lastDisplayUpdate = 0;

volatile uint32_t idleCount0 = 0;
volatile uint32_t idleCount1 = 0;
uint32_t lastCpuCalcTime = 0;
uint32_t prevIdle0 = 0;
uint32_t prevIdle1 = 0;
uint32_t maxIdle0 = 0;
uint32_t maxIdle1 = 0;
uint8_t cpuLoadPercent = 0;

bool idleHook0(void) { idleCount0++; return false; }
bool idleHook1(void) { idleCount1++; return false; }

String bodyBuffer;

void setupWiFi();
void setupServer();
void applyScene(const String& sceneId);
void computeSceneDmx(const String& sceneId, uint8_t* buf);
void updateShowPlayback();
void handleNotFound(AsyncWebServerRequest* request);
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
  esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
  
  Serial.println("\n\nDMXESP - Professional DMX Lighting Controller");
  Serial.println("Initializing...");
  
  displayManager.begin();
  displayManager.showBootLogo();
  delay(500);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    displayManager.update();
    delay(3000);
    ESP.restart();
    return;
  }
  
  Serial.println("SPIFFS initialized successfully");
  
  configManager.begin();
  dmxEngine.begin();
  sceneManager.begin(&dmxEngine);
  
  SetupDef* setupDef = sceneManager.getActiveSetup();
  int fixtureCount = setupDef ? setupDef->instances.size() : 0;
  Serial.println("DMX Engine initialized. Active instances: " + String(fixtureCount));
  
  systemState.masterBrightness = 100;
  systemState.strobeActive = false;
  systemState.smokeActive = false;
  
  audioManager.begin();
  Serial.println("Setup complete!");
  
  showPlayback.running = false;
  showPlayback.currentStep = -1;
  
  uint32_t bootStart = millis();
  while (millis() - bootStart < TFT_BOOT_DURATION) delay(10);
  
  setupWiFi();
  setupServer();
  
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.begin();
  Serial.println("Web server started on port " + String(WEB_PORT));
  
  bool apMode = (WiFi.getMode() == WIFI_AP);
  String ip = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  displayManager.showReady(ip, apMode);
  delay(2000);
  
  prevIdle0 = idleCount0;
  prevIdle1 = idleCount1;
  lastCpuCalcTime = millis();
}

void loop() {
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }

  ws.cleanupClients();
  audioManager.update();

  if (audioManager.getMode() != SOUND_OFF) {
    uint8_t r, g, b;
    audioManager.getSoundColor(r, g, b);
    uint8_t brightness = audioManager.getSoundBrightness();
    
    SetupDef* setup = sceneManager.getActiveSetup();
    if (setup) {
      for (const auto& f : setup->instances) {
        if (!f.enabled) continue;
        FixtureProfile* profile = sceneManager.getProfile(f.profileId);
        if (!profile) continue;
        for (const auto& ch : profile->channels) {
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
  }
  
  updateShowPlayback();
  
  if (systemState.smokeActive && millis() > smokeEndTime) {
    systemState.smokeActive = false;
  }
  
  systemState.uptime = millis() / 1000;
  systemState.freeMemory = ESP.getFreeHeap();
  
  if (millis() - lastCpuCalcTime >= 1000) {
    uint32_t curIdle0 = idleCount0;
    uint32_t curIdle1 = idleCount1;
    uint32_t delta0 = curIdle0 - prevIdle0;
    uint32_t delta1 = curIdle1 - prevIdle1;
    prevIdle0 = curIdle0;
    prevIdle1 = curIdle1;
    lastCpuCalcTime = millis();

    if (delta0 > maxIdle0) maxIdle0 = delta0;
    if (delta1 > maxIdle1) maxIdle1 = delta1;

    uint8_t load0 = (maxIdle0 > 0) ? 100 - min(100UL, delta0 * 100UL / maxIdle0) : 0;
    uint8_t load1 = (maxIdle1 > 0) ? 100 - min(100UL, delta1 * 100UL / maxIdle1) : 0;
    cpuLoadPercent = (load0 + load1) / 2;
  }
  
  uint32_t displayInterval = (audioManager.getMode() != SOUND_OFF) ? 50 : 500;
  if (millis() - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = millis();
    
    DisplayStatus dStatus;
    dStatus.wifiConnected = (WiFi.status() == WL_CONNECTED);
    dStatus.apMode = (WiFi.getMode() == WIFI_AP);
    dStatus.ipAddress = dStatus.apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    
    SetupDef* setup = sceneManager.getActiveSetup();
    dStatus.fixtureCount = setup ? setup->instances.size() : 0;
    dStatus.enabledFixtures = 0;
    if (setup) {
      for (const auto& f : setup->instances) {
        if (f.enabled) dStatus.enabledFixtures++;
      }
    }
    
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

void computeSceneDmx(const String& sceneId, uint8_t* buf) {
  Scene* scene = sceneManager.getScene(sceneId);
  if (!scene) return;
  
  for (const auto& fv : scene->fixtureValues) {
    FixtureInstance* inst = sceneManager.getInstance(fv.fixtureId);
    if (!inst || !inst->enabled) continue;
    FixtureProfile* prof = sceneManager.getProfile(inst->profileId);
    if (!prof) continue;
    
    for (const auto& ch : prof->channels) {
      auto it = fv.values.find(ch.name);
      uint8_t value;
      if (it != fv.values.end()) {
        value = it->second;
      } else if (ch.type == "dimmer") {
        value = 255;
      } else {
        value = ch.defaultValue;
      }
      uint16_t dmxAddr = inst->dmxAddress + ch.offset;
      if (dmxAddr >= 1 && dmxAddr <= 512) {
        buf[dmxAddr] = value;
      }
    }
  }
}

void applyScene(const String& sceneId) {
  Scene* scene = sceneManager.getScene(sceneId);
  if (!scene) return;
  
  for (const auto& fv : scene->fixtureValues) {
    FixtureInstance* inst = sceneManager.getInstance(fv.fixtureId);
    if (!inst || !inst->enabled) continue;
    FixtureProfile* prof = sceneManager.getProfile(inst->profileId);
    if (!prof) continue;
    
    for (const auto& ch : prof->channels) {
      auto it = fv.values.find(ch.name);
      uint8_t value;
      if (it != fv.values.end()) {
        value = it->second;
      } else if (ch.type == "dimmer") {
        value = 255;
      } else {
        value = ch.defaultValue;
      }
      uint16_t dmxAddr = inst->dmxAddress + ch.offset;
      if (dmxAddr >= 1 && dmxAddr <= 512) {
        dmxEngine.setChannelValue(dmxAddr, value);
      }
    }
  }
  systemState.activeScene = sceneId;
}

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
  
  if (showPlayback.inTransition && step.transitionTime > 0) {
    if (elapsed < step.transitionTime) {
      if (showPlayback.smoothActive) {
        float t = (float)elapsed / (float)step.transitionTime;
        for (int i = 1; i <= 512; i++) {
          uint8_t v = (uint8_t)((1.0f - t) * showPlayback.prevDmx[i] + t * showPlayback.targetDmx[i]);
          dmxEngine.setChannelValue(i, v);
        }
      }
      return;
    } else {
      showPlayback.inTransition = false;
      showPlayback.smoothActive = false;
      showPlayback.stepStartTime = now;
      applyScene(step.sceneId);
      return;
    }
  }
  
  if (elapsed >= step.duration) {
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
      showPlayback.inTransition = false;
      applyScene(nextStep.sceneId);
    } else if (nextStep.smoothTransition) {
      showPlayback.smoothActive = true;
      for (int i = 1; i <= 512; i++) {
        showPlayback.prevDmx[i] = dmxEngine.getChannelValue(i);
      }
      memset(showPlayback.targetDmx, 0, sizeof(showPlayback.targetDmx));
      computeSceneDmx(nextStep.sceneId, showPlayback.targetDmx);
    } else {
      applyScene(nextStep.sceneId);
    }
  }
}

void setupWiFi() {
  SystemConfig config = configManager.getConfig();
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    displayManager.showWiFiConnecting(String(config.wifiSSID), attempts, 20);
    delay(500);
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(config.wifiSSID, config.wifiPassword);
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
}

// ── Web Server Routes ──
void setupServer() {
  
  server.on("/api/profiles", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getProfilesJSON());
  });
  
  server.on("/api/setups/active", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getActiveSetupJSON());
  });

  server.on("/api/setups", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getSetupsListJSON());
  });
  
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(1024);
    doc["activeSetupId"] = sceneManager.getActiveSetupId();
    doc["activeScene"] = systemState.activeScene;
    doc["activeAmbiance"] = systemState.activeAmbiance;
    doc["activeShow"] = systemState.activeShow;
    doc["strobeActive"] = systemState.strobeActive;
    doc["smokeActive"] = systemState.smokeActive;
    doc["masterBrightness"] = systemState.masterBrightness;
    doc["soundMode"] = (int)audioManager.getMode();
    doc["soundSensitivity"] = audioManager.getSensitivity();
    
    AudioData ad = audioManager.getData();
    JsonObject audio = doc.createNestedObject("audio");
    audio["volume"] = (int)(ad.volume * 100);
    audio["bass"] = (int)(ad.bass * 100);
    audio["mid"] = (int)(ad.mid * 100);
    audio["high"] = (int)(ad.high * 100);
    audio["beat"] = ad.beatDetected;
    
    JsonArray dmxOutput = doc.createNestedArray("dmxOutput");
    for (int i = 0; i < 16; i++) {
      dmxOutput.add(dmxEngine.getChannelValue(i + 1)); // returning first 16 for preview
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
    doc["activeSetupId"] = sceneManager.getActiveSetupId();
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

  // ── Modifiers ──

  server.on("/api/profiles", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, data, len)) return request->send(400);
      FixtureProfile p;
      p.id = doc["id"].as<String>();
      p.name = doc["name"].as<String>();
      p.type = doc["type"].as<String>();
      p.channelCount = doc["channelCount"] | 1;
      
      JsonArray chArr = doc["channels"];
      for (JsonObject chObj : chArr) {
        ChannelDef ch;
        ch.name = chObj["name"].as<String>();
        ch.offset = chObj["offset"] | 0;
        ch.defaultValue = chObj["defaultValue"] | 0;
        ch.type = chObj["type"].as<String>();
        p.channels.push_back(ch);
      }
      if (doc.containsKey("strobeChannel")) {
        JsonObject sc = doc["strobeChannel"];
        p.strobeChannel.enabled = sc["enabled"] | false;
        p.strobeChannel.offset = sc["offset"] | 0;
        p.strobeChannel.value = sc["value"] | 255;
      }
      sceneManager.saveProfile(p);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/profiles/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    if (sceneManager.deleteProfile(id)) request->send(200, "application/json", "{\"status\":\"ok\"}");
    else request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  server.on("/api/setups", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(512);
      if (deserializeJson(doc, data, len)) return request->send(400);
      if (sceneManager.createSetup(doc["id"], doc["name"])) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Failed\"}");
      }
    });

  server.on("/api/instances", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, data, len)) return request->send(400);
      FixtureInstance i;
      i.id = doc["id"].as<String>();
      i.profileId = doc["profileId"].as<String>();
      i.name = doc["name"].as<String>();
      i.dmxAddress = doc["dmxAddress"] | 1;
      i.enabled = doc["enabled"] | true;
      sceneManager.saveInstance(i);
      
      // Update DMX engine if active
      FixtureProfile* profile = sceneManager.getProfile(i.profileId);
      if (profile) dmxEngine.addFixture(0, i.dmxAddress, profile->channelCount);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/instances/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    sceneManager.deleteInstance(id);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/virtual-groups", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, data, len)) return request->send(400);
      VirtualGroup vg;
      vg.id = doc["id"].as<String>();
      vg.name = doc["name"].as<String>();
      JsonArray arr = doc["assignments"];
      for (JsonObject obj : arr) {
        vg.assignments.push_back({obj["instanceId"].as<String>(), obj["channelName"].as<String>()});
      }
      sceneManager.saveVirtualGroup(vg);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/virtual-groups/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    sceneManager.deleteVirtualGroup(id);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Scenes
  server.on("/api/scenes", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
      if (index + len == total) {
        DynamicJsonDocument doc(8192);
        if (deserializeJson(doc, bodyBuffer)) return request->send(400);
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
          for (JsonPair kv : vals) fv.values[String(kv.key().c_str())] = kv.value().as<uint8_t>();
          s.fixtureValues.push_back(fv);
        }
        sceneManager.saveScene(s);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
    });

  server.on("/api/scenes/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    sceneManager.deleteScene(id);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/shows", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
      if (index + len == total) {
        DynamicJsonDocument doc(4096);
        if (deserializeJson(doc, bodyBuffer)) return request->send(400);
        Show s;
        s.id = doc["id"].as<String>();
        s.name = doc["name"].as<String>();
        s.description = doc["description"].as<String>();
        s.icon = doc["icon"].as<String>();
        s.loop = doc["loop"] | true;
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
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
    });

  server.on("/api/shows/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    sceneManager.deleteShow(id);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ── Runtime Controls ──
  server.on("/api/control/setup", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, data, len);
      if (doc.containsKey("setupId")) {
        showPlayback.running = false;
        systemState.activeShow = "";
        systemState.activeScene = "";
        sceneManager.setActiveSetup(doc["setupId"]);
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/scene", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, data, len);
      if (doc.containsKey("sceneId")) {
        if (showPlayback.running) {
          showPlayback.running = false;
          systemState.activeShow = "";
        }
        applyScene(doc["sceneId"].as<String>());
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/show", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, data, len);
      if (doc.containsKey("showId")) {
        String showId = doc["showId"].as<String>();
        Show* show = sceneManager.getShow(showId);
        if (show && !show->steps.empty()) {
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

  server.on("/api/control/show-stop", HTTP_POST, [](AsyncWebServerRequest* request) {
    showPlayback.running = false;
    systemState.activeShow = "";
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/control/group", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, data, len);
      if (doc.containsKey("groupId") && doc.containsKey("value")) {
        sceneManager.setVirtualGroupValue(doc["groupId"], doc["value"]);
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  // Re-use remaining original endpoints...
  server.on("/api/control/strobe", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("speed")) {
        dmxEngine.setStrobeSpeed(doc["speed"]);
        dmxEngine.setStrobeActive(true);
        systemState.strobeActive = true;
        SetupDef* setup = sceneManager.getActiveSetup();
        if (setup) {
          for (const auto& i : setup->instances) {
            FixtureProfile* p = sceneManager.getProfile(i.profileId);
            if (p && p->strobeChannel.enabled) {
              uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
              if (addr >= 1 && addr <= 512) dmxEngine.setChannelValue(addr, p->strobeChannel.value);
            }
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/strobe-stop", HTTP_POST, [](AsyncWebServerRequest* request) {
    dmxEngine.setStrobeActive(false);
    systemState.strobeActive = false;
    SetupDef* setup = sceneManager.getActiveSetup();
    if (setup) {
      for (const auto& i : setup->instances) {
        FixtureProfile* p = sceneManager.getProfile(i.profileId);
        if (p && p->strobeChannel.enabled) {
          uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
          if (addr >= 1 && addr <= 512) dmxEngine.setChannelValue(addr, 0);
        }
      }
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html");
  server.onNotFound(handleNotFound);
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      StaticJsonDocument<512> doc;
      if (deserializeJson(doc, data, len)) return;
      
      if (doc.containsKey("dmx")) {
        JsonObject dmx = doc["dmx"];
        for (JsonPair kv : dmx) {
          dmxEngine.setChannelValue(atoi(kv.key().c_str()), kv.value().as<int>());
        }
      } else if (doc.containsKey("ch") && doc.containsKey("val")) {
        dmxEngine.setChannelValue(doc["ch"], doc["val"]);
      } else if (doc.containsKey("group") && doc.containsKey("val")) {
        sceneManager.setVirtualGroupValue(doc["group"].as<String>(), doc["val"]);
      }
    }
  }
}

void handleNotFound(AsyncWebServerRequest* request) {
  if (WiFi.getMode() == WIFI_AP && !request->url().startsWith("/api/")) {
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    return;
  }
  request->send(404, "application/json", "{\"error\":\"Not found\"}");
}
