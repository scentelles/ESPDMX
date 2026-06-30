#include <Arduino.h>
#include <WiFi.h>
#include <esp_freertos_hooks.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <esp_coexist.h>
#include "config.h"
#include "dmx_engine.h"
#include "config_manager.h"
#include "scene_manager.h"
#include "display_manager.h"
#include "audio_manager.h"
#include "ble_midi.h"

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
  String activeSceneG2;
  String activeAmbiance;
  String activeShow;
  bool strobeActive;
  bool smokeActive;
  uint8_t masterBrightness;
  uint32_t uptime;
  uint32_t freeMemory;
  uint32_t lastPedalPingTime;
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
void applySceneValues(const String& sceneId);
void recalculateDmx();
void toggleScene(const String& sceneId);
void computeSceneDmx(const String& sceneId, uint8_t* buf);
void updateShowPlayback();
void triggerNextSceneSequence(int targetGroupId = -1);
void triggerRandomSceneSequence();
void triggerNextShowSequence();
void handleNotFound(AsyncWebServerRequest* request);
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
bool createBackupFile(const String& path);
bool restoreBackupFile(const String& path);
void handlePedalAction(int button, const String& state);
void handleBlePedalAction(uint8_t ccId, uint8_t value);

String pendingRestoreBackupPath = "";

bool g_showModeActive = false;
bool g_transitionToShowMode = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
  esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
  
  Serial.println("\n\nSUDSHOW - Professional DMX Lighting Controller");
  Serial.println("Initializing...");
  
  displayManager.begin();
  displayManager.showBootLogo();
  delay(500);
  
  esp_task_wdt_init(30, false); // Increase WDT timeout to 30 seconds for SPIFFS formatting
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    displayManager.update();
    delay(3000);
    ESP.restart();
    return;
  }
  esp_task_wdt_init(5, true); // Restore default WDT
  
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
  audioManager.setSensitivity(configManager.getConfig().soundSensitivity);
  audioManager.setDynamics(configManager.getConfig().soundDynamics);
  
  showPlayback.running = false;
  showPlayback.currentStep = -1;
  
  displayManager.showBootMessage("Recherche BLE...", "3s");
  Serial.println("Recherche du pedalier BLE...");
  
  bleMidiInit([](uint8_t ccId, uint8_t value) {
    handleBlePedalAction(ccId, value);
  });
  
  uint32_t bleStart = millis();
  bool pedalFound = false;
  int lastSecondsLeft = -1;
  while (millis() - bleStart < 5000) {
    bleMidiLoop();
    if (isBleMidiConnected()) {
      pedalFound = true;
      break;
    }
    int secondsLeft = 5 - ((millis() - bleStart) / 1000);
    if (secondsLeft != lastSecondsLeft) {
      displayManager.showBootMessage("Recherche BLE...", String(secondsLeft) + "s");
      lastSecondsLeft = secondsLeft;
    }
    delay(10);
  }
  
  if (pedalFound) {
    Serial.println("Pedalier BLE connecte, demarrage en MODE SHOW");
    g_showModeActive = true;
    displayManager.showBootMessage("ACTIVATION", "MODE BLE");
    delay(1500);
  } else {
    Serial.println("Pas de pedalier BLE, desactivation BLE et demarrage Wi-Fi");
    bleMidiDeinit();
    displayManager.showBootMessage("ACTIVATION", "MODE WIFI");
    delay(1500);
    
    setupWiFi();
    setupServer();
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.begin();
    Serial.println("Web server started on port " + String(WEB_PORT));
  }
  
  bool apMode = (WiFi.getMode() == WIFI_AP);
  String ip = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  displayManager.showReady(ip, apMode);
  delay(2000);
  
  prevIdle0 = idleCount0;
  prevIdle1 = idleCount1;
  lastCpuCalcTime = millis();
}

bool isColorChannel(const String& name, const String& colorStr) {
  String lower = name; lower.toLowerCase();
  return lower.indexOf(colorStr) != -1;
}

void parseHexColor(const String& hex, uint8_t& r, uint8_t& g, uint8_t& b) {
  if (hex.length() >= 7 && hex[0] == '#') {
    r = strtol(hex.substring(1, 3).c_str(), NULL, 16);
    g = strtol(hex.substring(3, 5).c_str(), NULL, 16);
    b = strtol(hex.substring(5, 7).c_str(), NULL, 16);
  } else {
    r = g = b = 0;
  }
}

void updateSceneFX(Scene* scene, uint32_t now) {
  if (!scene) return;
  
  for (const auto& fv : scene->fixtureValues) {
    if (fv.effect.type == "none" || fv.effect.type.length() == 0) continue;
    
    FixtureInstance* inst = sceneManager.getInstance(fv.fixtureId);
    if (!inst || !inst->enabled) continue;
    FixtureProfile* prof = sceneManager.getProfile(inst->profileId);
    if (!prof) continue;
    
    std::vector<uint16_t> rs, gs, bs;
    for (const auto& ch : prof->channels) {
      if (isColorChannel(ch.name, "red") || isColorChannel(ch.name, "rouge")) rs.push_back(ch.offset);
      else if (isColorChannel(ch.name, "green") || isColorChannel(ch.name, "vert")) gs.push_back(ch.offset);
      else if (isColorChannel(ch.name, "blue") || isColorChannel(ch.name, "bleu")) bs.push_back(ch.offset);
    }
    
    std::sort(rs.begin(), rs.end());
    std::sort(gs.begin(), gs.end());
    std::sort(bs.begin(), bs.end());
    
    size_t numModules = max(rs.size(), max(gs.size(), bs.size()));
    if (numModules == 0) continue;
    
    uint8_t fxR, fxG, fxB;
    parseHexColor(fv.effect.colorHex, fxR, fxG, fxB);
    
    // Speed is 1 to 100. Map to frequency: 1 -> 0.5Hz, 100 -> 20Hz (50ms period)
    float freq = 0.5f + (fv.effect.speed - 1) * (19.5f / 99.0f);
    uint32_t period = (uint32_t)(1000.0f / freq);
    if (period < 50) period = 50;
    
    if (fv.effect.type == "chaser") {
      size_t activeModule = (now / period) % numModules;
      if (fv.effect.reverse) activeModule = (numModules - 1) - activeModule;
      for (size_t i = 0; i < numModules; i++) {
        uint8_t r = (i == activeModule) ? fxR : 0;
        uint8_t g = (i == activeModule) ? fxG : 0;
        uint8_t b = (i == activeModule) ? fxB : 0;
        if (i < rs.size()) dmxEngine.setChannelValue(inst->dmxAddress + rs[i], r);
        if (i < gs.size()) dmxEngine.setChannelValue(inst->dmxAddress + gs[i], g);
        if (i < bs.size()) dmxEngine.setChannelValue(inst->dmxAddress + bs[i], b);
      }
    } else if (fv.effect.type == "sparkle") {
      randomSeed(now / period);
      size_t activeModule = random(numModules);
      for (size_t i = 0; i < numModules; i++) {
        uint8_t r = (i == activeModule) ? fxR : 0;
        uint8_t g = (i == activeModule) ? fxG : 0;
        uint8_t b = (i == activeModule) ? fxB : 0;
        if (i < rs.size()) dmxEngine.setChannelValue(inst->dmxAddress + rs[i], r);
        if (i < gs.size()) dmxEngine.setChannelValue(inst->dmxAddress + gs[i], g);
        if (i < bs.size()) dmxEngine.setChannelValue(inst->dmxAddress + bs[i], b);
      }
    } else if (fv.effect.type == "up") {
      size_t filled = (now / period) % (numModules + 1);
      for (size_t i = 0; i < numModules; i++) {
        bool isOn = fv.effect.reverse ? (i >= (numModules - filled)) : (i < filled);
        uint8_t r = isOn ? fxR : 0;
        uint8_t g = isOn ? fxG : 0;
        uint8_t b = isOn ? fxB : 0;
        if (i < rs.size()) dmxEngine.setChannelValue(inst->dmxAddress + rs[i], r);
        if (i < gs.size()) dmxEngine.setChannelValue(inst->dmxAddress + gs[i], g);
        if (i < bs.size()) dmxEngine.setChannelValue(inst->dmxAddress + bs[i], b);
      }
    } else if (fv.effect.type == "sine") {
      float angle = (float)(now % period) / (float)period * 2.0f * PI;
      float multiplier = (sin(angle) + 1.0f) / 2.0f; // 0.0 to 1.0
      
      uint8_t r = fxR * multiplier;
      uint8_t g = fxG * multiplier;
      uint8_t b = fxB * multiplier;
      
      for (size_t i = 0; i < numModules; i++) {
        if (i < rs.size()) dmxEngine.setChannelValue(inst->dmxAddress + rs[i], r);
        if (i < gs.size()) dmxEngine.setChannelValue(inst->dmxAddress + gs[i], g);
        if (i < bs.size()) dmxEngine.setChannelValue(inst->dmxAddress + bs[i], b);
      }
    } else if (fv.effect.type == "sine2") {
      for (size_t i = 0; i < numModules; i++) {
        float offset = (float)i / (float)numModules * 2.0f * PI;
        if (fv.effect.reverse) offset = -offset;
        
        float angle = ((float)(now % period) / (float)period * 2.0f * PI) + offset;
        float multiplier = (sin(angle) + 1.0f) / 2.0f; // 0.0 to 1.0
        
        uint8_t r = fxR * multiplier;
        uint8_t g = fxG * multiplier;
        uint8_t b = fxB * multiplier;
        
        if (i < rs.size()) dmxEngine.setChannelValue(inst->dmxAddress + rs[i], r);
        if (i < gs.size()) dmxEngine.setChannelValue(inst->dmxAddress + gs[i], g);
        if (i < bs.size()) dmxEngine.setChannelValue(inst->dmxAddress + bs[i], b);
      }
    }
  }
}

void updateFX() {
  if (showPlayback.running) return;
  uint32_t now = millis();
  
  if (systemState.activeScene.length() > 0) {
    updateSceneFX(sceneManager.getScene(systemState.activeScene), now);
  }
  if (systemState.activeSceneG2.length() > 0) {
    updateSceneFX(sceneManager.getScene(systemState.activeSceneG2), now);
  }
}

uint32_t rebootTime = 0;

void loop() {
  updateFX();
  if (rebootTime > 0 && millis() > rebootTime) {
    ESP.restart();
  }

  if (pendingRestoreBackupPath.length() > 0) {
    String path = pendingRestoreBackupPath;
    pendingRestoreBackupPath = "";
    Serial.println("Restoring backup from " + path);
    delay(100);
    if (restoreBackupFile(path)) {
      if (path == "/bk_uploaded.json") SPIFFS.remove(path);
      rebootTime = millis() + 1500;
    } else {
      if (path == "/bk_uploaded.json") SPIFFS.remove(path);
      Serial.println("Restore failed!");
    }
  }

  if (g_transitionToShowMode) {
    g_transitionToShowMode = false;
    g_showModeActive = true;
    
    Serial.println(">>> BASCULEMENT EN MODE SHOW <<<");
    Serial.println("Fermeture du serveur Web et du Wi-Fi...");
    
    server.end();
    ws.closeAll();
    dnsServer.stop();
    WiFi.mode(WIFI_OFF);
    
    Serial.println("Initialisation du Bluetooth (BLE MIDI)...");
    bleMidiInit([](uint8_t ccId, uint8_t value) {
      handleBlePedalAction(ccId, value);
    });
    
    displayManager.showReady("MODE SHOW", false);
  }

  if (g_showModeActive) {
    bleMidiLoop();
  } else {
    if (WiFi.getMode() == WIFI_AP) {
      dnsServer.processNextRequest();
    }
    ws.cleanupClients();
  }
  audioManager.update();

  if (audioManager.getMode() == SOUND_SCENE_G1) {
    if (audioManager.getData().beatDetected) triggerNextSceneSequence(1);
  } else if (audioManager.getMode() == SOUND_SCENE_G2) {
    if (audioManager.getData().beatDetected) triggerNextSceneSequence(2);
  } else if (audioManager.getMode() == SOUND_SCENE_SEQ) {
    if (audioManager.getData().beatDetected) triggerNextSceneSequence(-1);
  } else if (audioManager.getMode() == SOUND_SCENE_RND) {
    if (audioManager.getData().beatDetected) triggerRandomSceneSequence();
  } else if (audioManager.getMode() != SOUND_OFF && audioManager.getMode() <= SOUND_VU && !systemState.strobeActive) {
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
  
  if (systemState.smokeActive && smokeEndTime > 0 && millis() > smokeEndTime) {
    systemState.smokeActive = false;
  }
  
  static bool pedalWasConnected = false;
  static bool blePedalWasConnected = false;
  bool pedalIsConnected = (systemState.lastPedalPingTime > 0) && (millis() - systemState.lastPedalPingTime < 10000);
  bool blePedalIsConnected = isBleMidiConnected();
  
  if ((pedalWasConnected && !pedalIsConnected) || (blePedalWasConnected && !blePedalIsConnected)) {
    // Pedal disconnected: cancel ongoing pedal actions and turn off DMX output
    systemState.smokeActive = false;
    
    if (systemState.strobeActive) {
      dmxEngine.setStrobeActive(false);
      systemState.strobeActive = false;
      dmxEngine.clearAllOverrides();
    }
    
    // Turn off active scenes, shows, and audio
    systemState.activeScene = "";
    systemState.activeSceneG2 = "";
    systemState.activeShow = "";
    showPlayback.running = false;
    audioManager.setMode(SOUND_OFF);
    dmxEngine.restoreVirtualConsole();
  }
  pedalWasConnected = pedalIsConnected;
  blePedalWasConnected = blePedalIsConnected;
  
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
    Scene* activeSceneG2 = systemState.activeSceneG2.length() > 0 ? sceneManager.getScene(systemState.activeSceneG2) : nullptr;
    Show* activeShow = systemState.activeShow.length() > 0 ? sceneManager.getShow(systemState.activeShow) : nullptr;
    dStatus.activeSetupName = setup ? setup->name : "Sans nom";
    dStatus.activeScene = activeScene ? activeScene->name : "";
    dStatus.activeSceneG2 = activeSceneG2 ? activeSceneG2->name : "";
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
    dStatus.pedalConnected = (millis() - systemState.lastPedalPingTime < 10000);
    dStatus.blePedalConnected = isBleMidiConnected();
    dStatus.showModeActive = g_showModeActive;
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
  
  for (const auto& vgv : scene->virtualGroupValues) {
    VirtualGroup* vg = sceneManager.getVirtualGroup(vgv.groupId);
    if (!vg) continue;
    
    uint8_t r = 0, g = 0, b = 0;
    bool hasColor = false;
    if (vgv.colorHex.length() >= 7 && vgv.colorHex.startsWith("#")) {
      r = strtol(vgv.colorHex.substring(1, 3).c_str(), NULL, 16);
      g = strtol(vgv.colorHex.substring(3, 5).c_str(), NULL, 16);
      b = strtol(vgv.colorHex.substring(5, 7).c_str(), NULL, 16);
      hasColor = true;
    }
    
    for (const auto& a : vg->assignments) {
      int addr = sceneManager.resolveDMXAddress(a.instanceId, a.channelName);
      if (addr >= 1 && addr <= 512) {
        if (hasColor) {
          if (isColorChannel(a.channelName, "red") || isColorChannel(a.channelName, "rouge") || a.channelName.equalsIgnoreCase("r")) {
            buf[addr] = r;
          } else if (isColorChannel(a.channelName, "green") || isColorChannel(a.channelName, "vert") || a.channelName.equalsIgnoreCase("g")) {
            buf[addr] = g;
          } else if (isColorChannel(a.channelName, "blue") || isColorChannel(a.channelName, "bleu") || a.channelName.equalsIgnoreCase("b")) {
            buf[addr] = b;
          }
        }
        if (vgv.dimmer >= 0) {
          if (a.channelName.equalsIgnoreCase("dimmer") || a.channelName.equalsIgnoreCase("dim")) {
            buf[addr] = vgv.dimmer;
          }
        }
      }
    }
  }
}

void applySceneValues(const String& sceneId) {
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

  for (const auto& vgv : scene->virtualGroupValues) {
    VirtualGroup* vg = sceneManager.getVirtualGroup(vgv.groupId);
    if (!vg) continue;
    
    uint8_t r = 0, g = 0, b = 0;
    bool hasColor = false;
    if (vgv.colorHex.length() >= 7 && vgv.colorHex.startsWith("#")) {
      r = strtol(vgv.colorHex.substring(1, 3).c_str(), NULL, 16);
      g = strtol(vgv.colorHex.substring(3, 5).c_str(), NULL, 16);
      b = strtol(vgv.colorHex.substring(5, 7).c_str(), NULL, 16);
      hasColor = true;
    }
    
    for (const auto& a : vg->assignments) {
      FixtureInstance* inst = sceneManager.getInstance(a.instanceId);
      if (!inst || !inst->enabled) continue;
      FixtureProfile* prof = sceneManager.getProfile(inst->profileId);
      if (!prof) continue;
      
      for (const auto& ch : prof->channels) {
        if (a.channelName != "ALL" && a.channelName != ch.name) continue;
        
        int addr = inst->dmxAddress + ch.offset;
        if (addr >= 1 && addr <= 512) {
          if (hasColor) {
            if (isColorChannel(ch.name, "red") || isColorChannel(ch.name, "rouge") || ch.name.equalsIgnoreCase("r")) {
              dmxEngine.setChannelValue(addr, r);
            } else if (isColorChannel(ch.name, "green") || isColorChannel(ch.name, "vert") || ch.name.equalsIgnoreCase("g")) {
              dmxEngine.setChannelValue(addr, g);
            } else if (isColorChannel(ch.name, "blue") || isColorChannel(ch.name, "bleu") || ch.name.equalsIgnoreCase("b")) {
              dmxEngine.setChannelValue(addr, b);
            } else if (isColorChannel(ch.name, "white") || isColorChannel(ch.name, "blanc") || ch.name.equalsIgnoreCase("w")) {
              uint8_t w = min(r, min(g, b)); // simple white calculation
              dmxEngine.setChannelValue(addr, w);
            }
          }
          if (vgv.dimmer >= 0) {
            if (ch.type.equalsIgnoreCase("dimmer") || ch.name.equalsIgnoreCase("dimmer") || ch.name.equalsIgnoreCase("dim")) {
              dmxEngine.setChannelValue(addr, vgv.dimmer);
            }
          }
        }
      }
    }
  }
}

void recalculateDmx() {
  dmxEngine.restoreVirtualConsole();
  if (systemState.activeScene.length() > 0) {
    applySceneValues(systemState.activeScene);
  }
  if (systemState.activeSceneG2.length() > 0) {
    applySceneValues(systemState.activeSceneG2);
  }
}

void toggleScene(const String& sceneId) {
  Scene* scene = sceneManager.getScene(sceneId);
  if (!scene) return;
  
  if (scene->groupId == 2) {
    if (systemState.activeSceneG2 == sceneId) systemState.activeSceneG2 = "";
    else systemState.activeSceneG2 = sceneId;
  } else {
    if (systemState.activeScene == sceneId) systemState.activeScene = "";
    else systemState.activeScene = sceneId;
  }
  recalculateDmx();
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
      applySceneValues(step.sceneId);
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
      applySceneValues(nextStep.sceneId);
    } else if (nextStep.smoothTransition) {
      showPlayback.smoothActive = true;
      for (int i = 1; i <= 512; i++) {
        showPlayback.prevDmx[i] = dmxEngine.getChannelValue(i);
      }
      memset(showPlayback.targetDmx, 0, sizeof(showPlayback.targetDmx));
      computeSceneDmx(nextStep.sceneId, showPlayback.targetDmx);
    } else {
      applySceneValues(nextStep.sceneId);
    }
  }
}

bool createBackupFile(const String& path) {
  File file = SPIFFS.open(path, "w");
  if (!file) return false;
  
  file.print("{\"backupVersion\":1");
  
  // 1. Write config
  file.print(",\"config\":");
  if (SPIFFS.exists("/config.json")) {
    File f = SPIFFS.open("/config.json", "r");
    if (f) {
      while (f.available()) {
        file.write(f.read());
      }
      f.close();
    } else {
      file.print("null");
    }
  } else {
    file.print("null");
  }
  
  // 2. Write profiles
  file.print(",\"profiles\":");
  if (SPIFFS.exists("/profiles.json")) {
    File f = SPIFFS.open("/profiles.json", "r");
    if (f) {
      while (f.available()) {
        file.write(f.read());
      }
      f.close();
    } else {
      file.print("[]");
    }
  } else {
    file.print("[]");
  }
  
  // 3. Write setupsList & setupsData
  file.print(",\"setupsList\":");
  bool setupsOk = false;
  std::vector<String> setupIds;
  if (SPIFFS.exists("/setups.json")) {
    File f = SPIFFS.open("/setups.json", "r");
    if (f) {
      String content = f.readString();
      file.print(content);
      f.close();
      
      DynamicJsonDocument doc(8192);
      DeserializationError err = deserializeJson(doc, content);
      if (!err) {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject obj : arr) {
          setupIds.push_back(obj["id"].as<String>());
        }
      }
      setupsOk = true;
    }
  }
  if (!setupsOk) {
    file.print("[]");
  }
  
  file.print(",\"setupsData\":{");
  for (size_t i = 0; i < setupIds.size(); i++) {
    if (i > 0) file.print(",");
    file.print("\"" + setupIds[i] + "\":");
    String pathSetup = "/setup_" + setupIds[i] + ".json";
    if (SPIFFS.exists(pathSetup)) {
      File sf = SPIFFS.open(pathSetup, "r");
      if (sf) {
        while (sf.available()) {
          file.write(sf.read());
        }
        sf.close();
      } else {
        file.print("null");
      }
    } else {
      file.print("null");
    }
  }
  file.print("}"); // end setupsData
  
  file.print("}"); // end backup JSON
  file.close();
  return true;
}

bool restoreBackupFile(const String& path) {
  Serial.println("=== RESTORE START === path=" + path);
  Serial.println("Free heap before restore: " + String(ESP.getFreeHeap()));
  
  if (!SPIFFS.exists(path)) {
    Serial.println("Backup file does not exist: " + path);
    return false;
  }

  int backupVersion = 1;
  {
    File file = SPIFFS.open(path, "r");
    if (file) {
      StaticJsonDocument<64> filter;
      filter["backupVersion"] = true;
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, file, DeserializationOption::Filter(filter))) {
        if (doc.containsKey("backupVersion")) backupVersion = doc["backupVersion"].as<int>();
      }
      file.close();
    }
  }
  Serial.println("Detected Backup Version: " + String(backupVersion));

  // 1. Restore config.json
  Serial.println("Step 1: Restoring config...");
  {
    File file = SPIFFS.open(path, "r");
    if (!file) { Serial.println("Cannot open file"); return false; }
    StaticJsonDocument<128> filter;
    filter["config"] = true;
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter));
    file.close();
    Serial.println("  deserialize result: " + String(err.c_str()));
    if (!err && doc.containsKey("config")) {
      
      // -- MIGRATIONS FOR CONFIG --
      // if (backupVersion < 2) {
      //   // e.g. doc["config"]["newField"] = 42;
      // }
      
      File out = SPIFFS.open("/config.json", "w");
      if (out) { serializeJson(doc["config"], out); out.close(); }
      Serial.println("  config.json written OK");
    }
  }
  esp_task_wdt_reset();
  
  // 2. Restore profiles.json
  Serial.println("Step 2: Restoring profiles...");
  {
    File file = SPIFFS.open(path, "r");
    if (!file) { Serial.println("Cannot open file"); return false; }
    StaticJsonDocument<128> filter;
    filter["profiles"] = true;
    DynamicJsonDocument doc(32768);
    DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter));
    file.close();
    Serial.println("  deserialize result: " + String(err.c_str()) + " memUsed=" + String(doc.memoryUsage()));
    if (!err && doc.containsKey("profiles")) {
      
      // -- MIGRATIONS FOR PROFILES --
      
      File out = SPIFFS.open("/profiles.json", "w");
      if (out) { serializeJson(doc["profiles"], out); out.close(); }
      Serial.println("  profiles.json written OK");
    }
  }
  esp_task_wdt_reset();
  
  // 3. Restore setups.json and extract setupIds
  Serial.println("Step 3: Restoring setups list...");
  std::vector<String> setupIds;
  {
    File file = SPIFFS.open(path, "r");
    if (!file) { Serial.println("Cannot open file"); return false; }
    StaticJsonDocument<128> filter;
    filter["setupsList"] = true;
    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter));
    file.close();
    Serial.println("  deserialize result: " + String(err.c_str()));
    if (!err && doc.containsKey("setupsList")) {
      
      // -- MIGRATIONS FOR SETUPS LIST --
      
      File out = SPIFFS.open("/setups.json", "w");
      if (out) { serializeJson(doc["setupsList"], out); out.close(); }
      JsonArray arr = doc["setupsList"].as<JsonArray>();
      for (JsonObject obj : arr) {
        setupIds.push_back(obj["id"].as<String>());
      }
      Serial.println("  setups.json written OK, found " + String(setupIds.size()) + " setups");
    }
  }
  esp_task_wdt_reset();
  
  // 4. Restore each setup file /setup_<id>.json
  for (size_t si = 0; si < setupIds.size(); si++) {
    const String& setupId = setupIds[si];
    Serial.println("Step 4." + String(si) + ": Restoring setup " + setupId + "...");
    File file = SPIFFS.open(path, "r");
    if (!file) { Serial.println("Cannot open file"); continue; }
    DynamicJsonDocument filter(1024);
    filter["setupsData"][setupId] = true;
    DynamicJsonDocument doc(32768);
    DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter));
    file.close();
    Serial.println("  deserialize result: " + String(err.c_str()) + " memUsed=" + String(doc.memoryUsage()));
    if (!err && doc.containsKey("setupsData") && doc["setupsData"].containsKey(setupId)) {
      
      // -- MIGRATIONS FOR SETUP DATA --
      
      File out = SPIFFS.open("/setup_" + setupId + ".json", "w");
      if (out) { serializeJson(doc["setupsData"][setupId], out); out.close(); }
      Serial.println("  setup_" + setupId + ".json written OK");
    } else {
      Serial.println("  FAILED to find setupsData." + setupId);
    }
    esp_task_wdt_reset();
  }

  Serial.println("Free heap after restore: " + String(ESP.getFreeHeap()));
  Serial.println("=== RESTORE COMPLETE - reloading managers ===");

  // Reload managers
  configManager.begin();
  sceneManager.begin(&dmxEngine);
  return true;
}

void triggerRandomSceneSequence() {
  SetupDef* setup = sceneManager.getActiveSetup();
  if (!setup || setup->scenes.empty()) return;
  
  if (showPlayback.running) {
    showPlayback.running = false;
    systemState.activeShow = "";
  }
  
  int nextIndex = random(0, setup->scenes.size());
  
  // Try to avoid the currently active scene if we have more than 1 scene
  if (setup->scenes.size() > 1) {
    int attempts = 0;
    while ((setup->scenes[nextIndex].id == systemState.activeScene || 
            setup->scenes[nextIndex].id == systemState.activeSceneG2) && attempts < 10) {
      nextIndex = random(0, setup->scenes.size());
      attempts++;
    }
  }

  int targetGroupId = setup->scenes[nextIndex].groupId;
  
  if (targetGroupId == 2 && systemState.activeSceneG2 != "") {
    toggleScene(systemState.activeSceneG2);
  } else if (targetGroupId == 1 && systemState.activeScene != "") {
    toggleScene(systemState.activeScene);
  }
  toggleScene(setup->scenes[nextIndex].id);
}

void triggerNextSceneSequence(int targetGroupId) {
  SetupDef* setup = sceneManager.getActiveSetup();
  if (!setup || setup->scenes.empty()) return;
  
  if (showPlayback.running) {
    showPlayback.running = false;
    systemState.activeShow = "";
  }
  
  int currentIndex = -1;
  for (size_t i = 0; i < setup->scenes.size(); ++i) {
    if (targetGroupId == 1 || targetGroupId == -1) {
      if (setup->scenes[i].id == systemState.activeScene && setup->scenes[i].groupId == 1) {
        currentIndex = i;
        break;
      }
    }
    if (targetGroupId == 2 || targetGroupId == -1) {
      if (setup->scenes[i].id == systemState.activeSceneG2 && setup->scenes[i].groupId == 2) {
        currentIndex = i;
        break;
      }
    }
  }
  
  int nextIndex = -1;
  for (size_t i = 1; i <= setup->scenes.size(); ++i) {
    int testIndex = (currentIndex + i) % setup->scenes.size();
    if (targetGroupId == -1 || setup->scenes[testIndex].groupId == targetGroupId) {
      nextIndex = testIndex;
      break;
    }
  }
  
  if (nextIndex != -1 && nextIndex != currentIndex) {
    if (targetGroupId == 2 && systemState.activeSceneG2 != "") {
      toggleScene(systemState.activeSceneG2);
    } else if (targetGroupId == 1 && systemState.activeScene != "") {
      toggleScene(systemState.activeScene);
    }
    toggleScene(setup->scenes[nextIndex].id);
  } else if (nextIndex != -1 && currentIndex == -1) {
    toggleScene(setup->scenes[nextIndex].id);
  }
}

void triggerNextShowSequence() {
  SetupDef* setup = sceneManager.getActiveSetup();
  if (!setup || setup->shows.empty()) return;

  int currentIndex = -1;
  for (size_t i = 0; i < setup->shows.size(); ++i) {
    if (setup->shows[i].id == systemState.activeShow) {
      currentIndex = i;
      break;
    }
  }
  int nextIndex = (currentIndex + 1) % setup->shows.size();
  
  Show* nextShow = sceneManager.getShow(setup->shows[nextIndex].id);
  if (nextShow && !nextShow->steps.empty()) {
    nextShow->isRunning = true;
    showPlayback.running = true;
    showPlayback.showId = nextShow->id;
    showPlayback.currentStep = 0;
    showPlayback.stepStartTime = millis();
    showPlayback.inTransition = (nextShow->steps[0].transitionTime > 0);
    systemState.activeShow = nextShow->id;
    applySceneValues(nextShow->steps[0].sceneId);
  }
}

void handlePedalAction(int button, const String& state) {
  if (button < 1 || button > 3) return;
  
  static uint32_t pedalPressTime[3] = {0, 0, 0};
  if (state == "pressed") {
    pedalPressTime[button - 1] = millis();
  }
  
  SetupDef* setup = sceneManager.getActiveSetup();
  if (!setup) return;
  
  PedalButtonConfig& cfg = setup->pedalButtons[button - 1];
  
  Serial.printf("Pedal: btn=%d state=%s action=%s target=%s\n", 
    button, state.c_str(), cfg.action.c_str(), cfg.targetId.c_str());
  
  if (cfg.action == "smoke") {
    // Momentary: pressed = ON, released = OFF
    if (state == "pressed") {
      systemState.smokeActive = true;
      smokeEndTime = 0; // No auto-stop
    } else {
      systemState.smokeActive = false;
    }
  }
  else if (cfg.action == "strobe") {
    // Momentary: pressed = ON (keep current speed), released = OFF
    if (state == "pressed") {
      dmxEngine.setStrobeActive(true);
      systemState.strobeActive = true;
      if (setup) {
        for (const auto& i : setup->instances) {
          FixtureProfile* p = sceneManager.getProfile(i.profileId);
          if (p) {
            if (p->strobeChannel.enabled) {
              uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
              if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, p->strobeChannel.value);
            }
            for (const auto& ch : p->channels) {
              if (isColorChannel(ch.name, "red") || isColorChannel(ch.name, "rouge") || 
                  isColorChannel(ch.name, "green") || isColorChannel(ch.name, "vert") ||
                  isColorChannel(ch.name, "blue") || isColorChannel(ch.name, "bleu") ||
                  isColorChannel(ch.name, "white") || isColorChannel(ch.name, "blanc")) {
                uint16_t addr = i.dmxAddress + ch.offset;
                if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, 255);
              }
              if (ch.type == "dimmer") {
                uint16_t addr = i.dmxAddress + ch.offset;
                if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, 255);
              }
            }
          }
        }
      }
    } else {
      dmxEngine.setStrobeActive(false);
      systemState.strobeActive = false;
      dmxEngine.clearAllOverrides();
      if (setup) {
        for (const auto& i : setup->instances) {
          FixtureProfile* p = sceneManager.getProfile(i.profileId);
          if (p && p->strobeChannel.enabled) {
            uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
            if (addr >= 1 && addr <= 512) dmxEngine.setChannelValue(addr, 0);
          }
        }
      }
    }
  }
  else if (cfg.action == "scene" && state == "pressed") {
    // Disable audio mode when activating a scene
    audioManager.setMode(SOUND_OFF);
    
    // Trigger: apply scene on press, ignore release
    if (cfg.targetId.length() > 0) {
      if (showPlayback.running) {
        showPlayback.running = false;
        systemState.activeShow = "";
      }
      toggleScene(cfg.targetId);
    }
  }
  else if (cfg.action == "show" && state == "pressed") {
    // Disable audio mode when toggling a show
    audioManager.setMode(SOUND_OFF);
    
    // Toggle: press to start or stop
    if (cfg.targetId.length() > 0) {
      if (showPlayback.running && showPlayback.showId == cfg.targetId) {
        // Stop the show
        showPlayback.running = false;
        systemState.activeShow = "";
        Show* show = sceneManager.getShow(cfg.targetId);
        if (show) show->isRunning = false;
      } else {
        // Start the show
        Show* show = sceneManager.getShow(cfg.targetId);
        if (show && !show->steps.empty()) {
          show->isRunning = true;
          showPlayback.running = true;
          showPlayback.showId = cfg.targetId;
          showPlayback.currentStep = 0;
          showPlayback.stepStartTime = millis();
          showPlayback.inTransition = (show->steps[0].transitionTime > 0);
          systemState.activeShow = cfg.targetId;
          applySceneValues(show->steps[0].sceneId);
        }
      }
    }
  }
  else if (cfg.action == "scene_sequence" || cfg.action == "scene_sequence_g1" || cfg.action == "scene_sequence_g2") {
    if (state == "released") {
      if (millis() - pedalPressTime[button - 1] >= 2000) {
        // Appui long: désactiver
        if (showPlayback.running) {
          showPlayback.running = false;
          systemState.activeShow = "";
        }
        systemState.activeScene = "";
        systemState.activeSceneG2 = "";
        dmxEngine.restoreVirtualConsole();
      } else {
        // Appui court: scène suivante
        audioManager.setMode(SOUND_OFF);
        
        int targetGroupId = -1; // -1 means all groups
        if (cfg.action == "scene_sequence_g1") targetGroupId = 1;
        if (cfg.action == "scene_sequence_g2") targetGroupId = 2;

        static uint32_t lastSceneTrigger[3] = {0};
        if (millis() - lastSceneTrigger[button - 1] < 300) {
          // Double click: turn off the active scene(s) for this target group
          if ((targetGroupId == 1 || targetGroupId == -1) && systemState.activeScene != "") {
            toggleScene(systemState.activeScene);
          }
          if ((targetGroupId == 2 || targetGroupId == -1) && systemState.activeSceneG2 != "") {
            toggleScene(systemState.activeSceneG2);
          }
        } else {
          triggerNextSceneSequence(targetGroupId);
        }
        lastSceneTrigger[button - 1] = millis();
      }
    }
  }
  else if (cfg.action == "show_sequence") {
    if (state == "released") {
      if (millis() - pedalPressTime[button - 1] >= 2000) {
        // Appui long: désactiver
        if (showPlayback.running && systemState.activeShow.length() > 0) {
          Show* show = sceneManager.getShow(systemState.activeShow);
          if (show) show->isRunning = false;
        }
        showPlayback.running = false;
        systemState.activeShow = "";
        systemState.activeScene = "";
        systemState.activeSceneG2 = "";
        dmxEngine.restoreVirtualConsole();
      } else {
        // Appui court: show suivant
        audioManager.setMode(SOUND_OFF);
        triggerNextShowSequence();
      }
    }
  }
  else if (state == "pressed" && cfg.action.startsWith("sound_")) {
    // Toggle: press to activate, press again to deactivate
    SoundMode targetMode = SOUND_OFF;
    if (cfg.action == "sound_volume") targetMode = SOUND_VOLUME;
    else if (cfg.action == "sound_beat") targetMode = SOUND_BEAT;
    else if (cfg.action == "sound_color") targetMode = SOUND_COLOR;
    else if (cfg.action == "sound_vu") targetMode = SOUND_VU;
    else if (cfg.action == "sound_scene_g1") targetMode = SOUND_SCENE_G1;
    else if (cfg.action == "sound_scene_g2") targetMode = SOUND_SCENE_G2;
    else if (cfg.action == "sound_scene_seq") targetMode = SOUND_SCENE_SEQ;
    else if (cfg.action == "sound_scene_rnd") targetMode = SOUND_SCENE_RND;
    
    if (audioManager.getMode() == targetMode) {
      // Already active → turn off
      audioManager.setMode(SOUND_OFF);
      dmxEngine.restoreVirtualConsole();
    } else {
      // Activate
      audioManager.setMode(targetMode);
    }
  }
}

void handleBlePedalAction(uint8_t ccId, uint8_t value) {
  // La pédale envoie CC 0 à 15.
  // Le CC 16 sert au reboot.
  if (ccId == 16 && value == 0) {
    Serial.println("[BLE] CC 16 recu: Redemarrage en mode Web/WiFi...");
    ESP.restart();
    return;
  }

  SetupDef* setup = sceneManager.getActiveSetup();
  if (!setup) return;
  
  if (ccId > 15) return;
  
  PedalButtonConfig& cfg = setup->blePedalButtons[ccId];
  int actionIndex = ccId;
  
  String action = cfg.action;
  String targetId = cfg.targetId;
  
  String state = "ignored";
  static uint32_t blePedalPressTime[16] = {0};

  if (value == 0) {
    if (action == "smoke" || action == "strobe") {
      static bool toggleState[16] = {false};
      toggleState[actionIndex] = !toggleState[actionIndex];
      state = toggleState[actionIndex] ? "pressed" : "released";
    } else if (action == "scene_sequence" || action == "scene_sequence_g1" || action == "scene_sequence_g2" || action == "show_sequence") {
      state = "released";
      blePedalPressTime[actionIndex] = millis(); // Force diff to 0
    } else {
      state = "pressed";
    }
  }
  
  // Toujours logger la reception
  Serial.printf("[BLE] Reception: ccId=%d value=%d state=%s config_action=%s config_target=%s\n", 
    ccId, value, state.c_str(), action.c_str(), targetId.c_str());

  if (action == "none" || action == "") return;
  
  if (state == "pressed") {
    blePedalPressTime[actionIndex] = millis();
  }
  
  if (action == "smoke") {
    if (state == "pressed") {
      systemState.smokeActive = true;
      smokeEndTime = 0;
    } else {
      systemState.smokeActive = false;
    }
  }
  else if (action == "strobe") {
    if (state == "pressed") {
      dmxEngine.setStrobeActive(true);
      systemState.strobeActive = true;
      if (setup) {
        for (const auto& i : setup->instances) {
          FixtureProfile* p = sceneManager.getProfile(i.profileId);
          if (p) {
            if (p->strobeChannel.enabled) {
              uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
              if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, p->strobeChannel.value);
            }
            for (const auto& ch : p->channels) {
              if (isColorChannel(ch.name, "red") || isColorChannel(ch.name, "rouge") || 
                  isColorChannel(ch.name, "green") || isColorChannel(ch.name, "vert") ||
                  isColorChannel(ch.name, "blue") || isColorChannel(ch.name, "bleu") ||
                  isColorChannel(ch.name, "white") || isColorChannel(ch.name, "blanc")) {
                uint16_t addr = i.dmxAddress + ch.offset;
                if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, 255);
              }
              if (ch.type == "dimmer") {
                uint16_t addr = i.dmxAddress + ch.offset;
                if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, 255);
              }
            }
          }
        }
      }
    } else {
      dmxEngine.setStrobeActive(false);
      systemState.strobeActive = false;
      dmxEngine.clearAllOverrides();
      if (setup) {
        for (const auto& i : setup->instances) {
          FixtureProfile* p = sceneManager.getProfile(i.profileId);
          if (p && p->strobeChannel.enabled) {
            uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
            if (addr >= 1 && addr <= 512) dmxEngine.setChannelValue(addr, 0);
          }
        }
      }
    }
  }
  else if (action == "scene" && state == "pressed") {
    audioManager.setMode(SOUND_OFF);
    if (targetId.length() > 0) {
      if (showPlayback.running) {
        showPlayback.running = false;
        systemState.activeShow = "";
      }
      toggleScene(targetId);
    }
  }
  else if (action == "show" && state == "pressed") {
    audioManager.setMode(SOUND_OFF);
    if (targetId.length() > 0) {
      if (showPlayback.running && showPlayback.showId == targetId) {
        showPlayback.running = false;
        systemState.activeShow = "";
        Show* show = sceneManager.getShow(targetId);
        if (show) show->isRunning = false;
      } else {
        Show* show = sceneManager.getShow(targetId);
        if (show && !show->steps.empty()) {
          show->isRunning = true;
          showPlayback.running = true;
          showPlayback.showId = targetId;
          showPlayback.currentStep = 0;
          showPlayback.stepStartTime = millis();
          showPlayback.inTransition = (show->steps[0].transitionTime > 0);
          systemState.activeShow = targetId;
          applySceneValues(show->steps[0].sceneId);
        }
      }
    }
  }
  else if (action == "scene_sequence" || action == "scene_sequence_g1" || action == "scene_sequence_g2") {
    if (state == "released") {
      if (millis() - blePedalPressTime[actionIndex] >= 2000) {
        if (showPlayback.running) {
          showPlayback.running = false;
          systemState.activeShow = "";
        }
        systemState.activeScene = "";
        systemState.activeSceneG2 = "";
        dmxEngine.restoreVirtualConsole();
      } else {
        audioManager.setMode(SOUND_OFF);
        if (showPlayback.running) {
          showPlayback.running = false;
          systemState.activeShow = "";
        }

        int targetGroupId = -1;
        if (action == "scene_sequence_g1") targetGroupId = 1;
        if (action == "scene_sequence_g2") targetGroupId = 2;

        static uint32_t lastSceneTrigger[16] = {0};
        if (millis() - lastSceneTrigger[actionIndex] < 300) {
          // Double click: turn off the active scene(s) for this target group
          if ((targetGroupId == 1 || targetGroupId == -1) && systemState.activeScene != "") {
            toggleScene(systemState.activeScene);
          }
          if ((targetGroupId == 2 || targetGroupId == -1) && systemState.activeSceneG2 != "") {
            toggleScene(systemState.activeSceneG2);
          }
        } else {
          triggerNextSceneSequence(targetGroupId);
        }
        lastSceneTrigger[actionIndex] = millis();
      }
    }
  }
  else if (action == "show_sequence") {
    if (state == "released") {
      if (millis() - blePedalPressTime[actionIndex] >= 2000) {
        if (showPlayback.running && systemState.activeShow.length() > 0) {
          Show* show = sceneManager.getShow(systemState.activeShow);
          if (show) show->isRunning = false;
        }
        showPlayback.running = false;
        systemState.activeShow = "";
        systemState.activeScene = "";
        systemState.activeSceneG2 = "";
        dmxEngine.restoreVirtualConsole();
      } else {
        audioManager.setMode(SOUND_OFF);
        triggerNextShowSequence();
      }
    }
  }
  else if (state == "pressed" && action.startsWith("sound_")) {
    SoundMode targetMode = SOUND_OFF;
    if (action == "sound_volume") targetMode = SOUND_VOLUME;
    else if (action == "sound_beat") targetMode = SOUND_BEAT;
    else if (action == "sound_color") targetMode = SOUND_COLOR;
    else if (action == "sound_vu") targetMode = SOUND_VU;
    else if (action == "sound_scene_g1") targetMode = SOUND_SCENE_G1;
    else if (action == "sound_scene_g2") targetMode = SOUND_SCENE_G2;
    else if (action == "sound_scene_seq") targetMode = SOUND_SCENE_SEQ;
    else if (action == "sound_scene_rnd") targetMode = SOUND_SCENE_RND;
    
    if (audioManager.getMode() == targetMode) {
      audioManager.setMode(SOUND_OFF);
      dmxEngine.restoreVirtualConsole();
    } else {
      audioManager.setMode(targetMode);
    }
  }
}

void setupWiFi() {
  SystemConfig config = configManager.getConfig();
  
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.print("[WiFi] Station connected, MAC: ");
    for(int i=0; i<6; i++){
      Serial.printf("%02X", info.wifi_ap_staconnected.mac[i]);
      if(i<5) Serial.print(":");
    }
    Serial.println();
  }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.print("[WiFi] Station disconnected, MAC: ");
    for(int i=0; i<6; i++){
      Serial.printf("%02X", info.wifi_ap_stadisconnected.mac[i]);
      if(i<5) Serial.print(":");
    }
    Serial.println();
  }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  if (String(config.wifiSSID) == "SUDSHOW" || String(config.wifiSSID) == "") {
    Serial.println("[WiFi] Starting Access Point directly");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("SUDSHOW", "12345678");
    WiFi.setSleep(false);
    dnsServer.start(53, "*", WiFi.softAPIP());
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifiSSID, config.wifiPassword);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      displayManager.showWiFiConnecting(String(config.wifiSSID), attempts, 20);
      delay(500);
      attempts++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Connection failed, falling back to AP");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(config.wifiSSID, config.wifiPassword);
      WiFi.setSleep(false);
      dnsServer.start(53, "*", WiFi.softAPIP());
    }
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

  server.on("/api/setups/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    int idx = url.lastIndexOf('/');
    if (idx != -1) {
      String id = url.substring(idx + 1);
      if (sceneManager.deleteSetup(id)) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Cannot delete active setup or not found\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Invalid id\"}");
    }
  });
  
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    // Build JSON manually to avoid huge DynamicJsonDocument for 512 DMX values
    String json = "{";
    json += "\"activeSetupId\":\"" + sceneManager.getActiveSetupId() + "\"";
    json += ",\"activeScene\":\"" + systemState.activeScene + "\"";
    json += ",\"activeSceneG2\":\"" + systemState.activeSceneG2 + "\"";
    json += ",\"activeAmbiance\":\"" + systemState.activeAmbiance + "\"";
    json += ",\"activeShow\":\"" + systemState.activeShow + "\"";
    json += ",\"strobeActive\":" + String(systemState.strobeActive ? "true" : "false");
    json += ",\"smokeActive\":" + String(systemState.smokeActive ? "true" : "false");
    json += ",\"masterBrightness\":" + String(systemState.masterBrightness);
    json += ",\"soundMode\":" + String((int)audioManager.getMode());
    json += ",\"soundSensitivity\":" + String(audioManager.getSensitivity());
    json += ",\"soundDynamics\":" + String(audioManager.getDynamics());
    json += ",\"pedalConnected\":" + String((millis() - systemState.lastPedalPingTime < 10000) ? "true" : "false");
    
    AudioData ad = audioManager.getData();
    json += ",\"audio\":{";
    json += "\"volume\":" + String((int)(ad.volume * 100));
    json += ",\"bass\":" + String((int)(ad.bass * 100));
    json += ",\"mid\":" + String((int)(ad.mid * 100));
    json += ",\"high\":" + String((int)(ad.high * 100));
    json += ",\"beat\":" + String(ad.beatDetected ? "true" : "false");
    json += "}";
    
    json += ",\"dmxOutput\":[";
    for (int i = 0; i < 512; i++) {
      if (i > 0) json += ',';
      json += String(dmxEngine.getChannelValue(i + 1));
    }
    json += "]}";
    
    request->send(200, "application/json", json);
  });
  
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(256);
    doc["dmxBaud"] = configManager.getConfig().dmxBaud;
    doc["maxFixtures"] = configManager.getConfig().maxFixtures;
    doc["updateInterval"] = configManager.getConfig().updateInterval;
    doc["soundSensitivity"] = configManager.getConfig().soundSensitivity;
    doc["soundDynamics"] = configManager.getConfig().soundDynamics;
    doc["activeSetupId"] = sceneManager.getActiveSetupId();
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(512);
      if (deserializeJson(doc, data, len)) return request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      
      SystemConfig config = configManager.getConfig();
      if (doc.containsKey("dmxBaud")) config.dmxBaud = doc["dmxBaud"].as<int>();
      if (doc.containsKey("maxFixtures")) config.maxFixtures = doc["maxFixtures"].as<int>();
      if (doc.containsKey("updateInterval")) config.updateInterval = doc["updateInterval"].as<int>();
      if (doc.containsKey("wifiSSID")) strlcpy(config.wifiSSID, doc["wifiSSID"] | "", sizeof(config.wifiSSID));
      if (doc.containsKey("wifiPassword")) strlcpy(config.wifiPassword, doc["wifiPassword"] | "", sizeof(config.wifiPassword));
      if (doc.containsKey("adminPin")) strlcpy(config.adminPin, doc["adminPin"] | "", sizeof(config.adminPin));
      if (doc.containsKey("soundSensitivity")) {
        config.soundSensitivity = doc["soundSensitivity"].as<int>();
        audioManager.setSensitivity(config.soundSensitivity);
      }
      if (doc.containsKey("soundDynamics")) {
        config.soundDynamics = doc["soundDynamics"].as<int>();
        audioManager.setDynamics(config.soundDynamics);
      }
      
      configManager.setConfig(config);
      
      String response;
      serializeJson(doc, response); // Echo back what was changed
      request->send(200, "application/json", response);
    }
  );

  server.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    ESP.restart();
  });

  server.on("/api/system/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(256);
    doc["uptime"] = systemState.uptime;
    doc["freeMemory"] = systemState.freeMemory;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ── OTA Updates ──
  server.on("/api/system/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool shouldReboot = !Update.hasError();
    if (request->hasParam("reboot") && request->getParam("reboot")->value() == "false") {
      shouldReboot = false;
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", !Update.hasError() ? "{\"status\":\"ok\"}" : "{\"status\":\"error\"}");
    response->addHeader("Connection", "close");
    request->send(response);
    if(shouldReboot) {
      delay(100);
      ESP.restart();
    }
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      int cmd = U_FLASH;
      if (request->hasParam("type") && request->getParam("type")->value() == "spiffs") {
        cmd = U_SPIFFS;
      } else if (filename.indexOf("spiffs") != -1) {
        cmd = U_SPIFFS;
      }
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // ── Modifiers ──

  server.on("/api/profiles", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
      if (index + len == total) {
        DynamicJsonDocument doc(16384);
        DeserializationError err = deserializeJson(doc, bodyBuffer);
        if (err) {
          Serial.println("Profile POST parse error: " + String(err.c_str()));
          return request->send(400);
        }
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
      }
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
      DynamicJsonDocument doc(1024);
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
      DynamicJsonDocument doc(2048);
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
      if (index == 0) bodyBuffer = "";
      bodyBuffer += String((char*)data, len);
      if (index + len == total) {
        DynamicJsonDocument doc(16384);
        if (deserializeJson(doc, bodyBuffer)) return request->send(400);
        VirtualGroup vg;
        vg.id = doc["id"].as<String>();
        vg.name = doc["name"].as<String>();
        JsonArray arr = doc["assignments"];
        for (JsonObject obj : arr) {
          vg.assignments.push_back({obj["instanceId"].as<String>(), obj["channelName"].as<String>()});
        }
        sceneManager.saveVirtualGroup(vg);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
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
        DynamicJsonDocument doc(16384);
        DeserializationError err = deserializeJson(doc, bodyBuffer);
        if (err) {
          Serial.println("Scene POST parse error: " + String(err.c_str()));
          return request->send(400);
        }
        Scene s;
        s.id = doc["id"].as<String>();
        s.name = doc["name"].as<String>();
        s.description = doc["description"].as<String>();
        s.icon = doc["icon"].as<String>();
        s.groupId = doc["groupId"] | 1;
        JsonArray fvArr = doc["fixtureValues"];
        for (JsonObject fvObj : fvArr) {
          FixtureChannelValues fv;
          fv.fixtureId = fvObj["fixtureId"].as<String>();
          if (fvObj.containsKey("effect")) {
            JsonObject fxObj = fvObj["effect"];
            fv.effect.type = fxObj["type"].as<String>();
            fv.effect.speed = fxObj["speed"] | 50;
            fv.effect.colorHex = fxObj["colorHex"].as<String>();
            fv.effect.reverse = fxObj["reverse"] | false;
          } else {
            fv.effect.type = "none";
          }
          JsonObject vals = fvObj["values"];
          for (JsonPair kv : vals) fv.values[String(kv.key().c_str())] = kv.value().as<uint8_t>();
          s.fixtureValues.push_back(fv);
        }
        
        if (doc.containsKey("virtualGroupValues")) {
          JsonArray vgvArr = doc["virtualGroupValues"];
          for (JsonObject vgvObj : vgvArr) {
            VirtualGroupSceneValue vgv;
            vgv.groupId = vgvObj["groupId"].as<String>();
            vgv.colorHex = vgvObj["colorHex"] | "";
            vgv.dimmer = vgvObj.containsKey("dimmer") ? vgvObj["dimmer"].as<int>() : -1;
            s.virtualGroupValues.push_back(vgv);
          }
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
        DynamicJsonDocument doc(16384);
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
        systemState.activeSceneG2 = "";
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
        String sceneId = doc["sceneId"].as<String>();
        if (showPlayback.running) {
          showPlayback.running = false;
          systemState.activeShow = "";
        }
        if (systemState.activeScene == sceneId || systemState.activeSceneG2 == sceneId) {
          if (sceneManager.getScene(sceneId) && sceneManager.getScene(sceneId)->groupId == 2) {
            systemState.activeSceneG2 = "";
          } else {
            systemState.activeScene = "";
          }
          recalculateDmx();
          dmxEngine.clearFixtures();
        } else {
          toggleScene(sceneId);
        }
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
        if (systemState.activeShow == showId && showPlayback.running) {
          showPlayback.running = false;
          systemState.activeShow = "";
          systemState.activeScene = "";
          dmxEngine.restoreVirtualConsole();
        } else {
          Show* show = sceneManager.getShow(showId);
          if (show && !show->steps.empty()) {
            show->isRunning = true;
            showPlayback.running = true;
            showPlayback.showId = showId;
            showPlayback.currentStep = 0;
            showPlayback.stepStartTime = millis();
            showPlayback.inTransition = (show->steps[0].transitionTime > 0);
            systemState.activeShow = showId;
            applySceneValues(show->steps[0].sceneId);
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/show-stop", HTTP_POST, [](AsyncWebServerRequest* request) {
    showPlayback.running = false;
    systemState.activeShow = "";
    dmxEngine.restoreVirtualConsole();
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
      if (!deserializeJson(doc, data, len)) {
        // Speed is optional — if provided, update it; otherwise keep current speed
        if (doc.containsKey("speed")) {
          dmxEngine.setStrobeSpeed(doc["speed"]);
        }
        dmxEngine.setStrobeActive(true);
        systemState.strobeActive = true;
        SetupDef* setup = sceneManager.getActiveSetup();
        if (setup) {
          for (const auto& i : setup->instances) {
            FixtureProfile* p = sceneManager.getProfile(i.profileId);
            if (p) {
              if (p->strobeChannel.enabled) {
                uint16_t addr = i.dmxAddress + p->strobeChannel.offset;
                if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, p->strobeChannel.value);
              }
              for (const auto& ch : p->channels) {
                if (isColorChannel(ch.name, "red") || isColorChannel(ch.name, "rouge") || 
                    isColorChannel(ch.name, "green") || isColorChannel(ch.name, "vert") ||
                    isColorChannel(ch.name, "blue") || isColorChannel(ch.name, "bleu") ||
                    isColorChannel(ch.name, "white") || isColorChannel(ch.name, "blanc")) {
                  uint16_t addr = i.dmxAddress + ch.offset;
                  if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, 255);
                }
                if (ch.type == "dimmer") {
                  uint16_t addr = i.dmxAddress + ch.offset;
                  if (addr >= 1 && addr <= 512) dmxEngine.setChannelOverride(addr, 255);
                }
              }
            }
          }
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/strobe-stop", HTTP_POST, [](AsyncWebServerRequest* request) {
    dmxEngine.setStrobeActive(false);
    systemState.strobeActive = false;
    dmxEngine.clearAllOverrides();
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

  server.on("/api/control/smoke", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len)) {
        systemState.smokeActive = true;
        // Duration is optional — if provided, auto-stop after delay; otherwise stay active until smoke-stop
        if (doc.containsKey("duration")) {
          smokeEndTime = millis() + doc["duration"].as<uint32_t>() * 1000;
        } else {
          smokeEndTime = 0; // No auto-stop, controlled by smoke-stop endpoint
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/smoke-stop", HTTP_POST, [](AsyncWebServerRequest* request) {
    systemState.smokeActive = false;
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/control/brightness", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("brightness")) {
        systemState.masterBrightness = doc["brightness"].as<uint8_t>();
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/sound", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len)) {
        bool saveConfig = false;
        SystemConfig config = configManager.getConfig();
        
        if (doc.containsKey("mode")) audioManager.setMode((SoundMode)doc["mode"].as<int>());
        if (doc.containsKey("sensitivity")) {
          uint8_t sens = doc["sensitivity"].as<uint8_t>();
          audioManager.setSensitivity(sens);
          config.soundSensitivity = sens;
          saveConfig = true;
        }
        if (doc.containsKey("dynamics")) {
          uint8_t dyn = doc["dynamics"].as<uint8_t>();
          audioManager.setDynamics(dyn);
          config.soundDynamics = dyn;
          saveConfig = true;
        }
        
        if (saveConfig) {
          configManager.setConfig(config);
          configManager.saveConfig();
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/control/sound-stop", HTTP_POST, [](AsyncWebServerRequest* request) {
    audioManager.setMode(SOUND_OFF);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/control/dmx", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
        if (!deserializeJson(doc, data, len) && doc.containsKey("channel") && doc.containsKey("value")) {
          dmxEngine.setVirtualConsoleValue(doc["channel"].as<int>(), doc["value"].as<int>());
        }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  // ── Pedal Control ──

  server.on("/api/control/pedal", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len)) {
        if (doc.containsKey("action") && doc["action"].as<String>() == "ping") {
          systemState.lastPedalPingTime = millis();
        } else if (doc.containsKey("button") && doc.containsKey("state")) {
          int button = doc["button"].as<int>();
          String state = doc["state"].as<String>();
          systemState.lastPedalPingTime = millis(); // Any action acts as a ping
          handlePedalAction(button, state);
        }
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/pedal-config", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getPedalConfigJSON());
  });

  server.on("/api/pedal-config", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("button") && doc.containsKey("action")) {
        int button = doc["button"].as<int>();
        String action = doc["action"].as<String>();
        String targetId = doc["targetId"] | "";
        sceneManager.savePedalConfig(button, action, targetId);
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/ble-pedal", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", sceneManager.getBlePedalConfigJSON());
  });

  server.on("/api/ble-pedal", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(2048);
      if (!deserializeJson(doc, data, len) && doc.is<JsonArray>()) {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject obj : arr) {
          int ccId = obj["ccId"] | 0;
          if (ccId >= 1 && ccId <= 16) {
            String action = obj["action"] | "none";
            String param = obj["param"] | "";
            sceneManager.saveBlePedalConfig(ccId, action, param, false);
          }
        }
        sceneManager.saveActiveSetup();
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.on("/api/audio", HTTP_GET, [](AsyncWebServerRequest* request) {
    AudioData ad = audioManager.getData();
    String json = "{";
    json += "\"volume\":" + String(ad.volume * 100) + ",";
    json += "\"bass\":" + String(ad.bass * 100) + ",";
    json += "\"mid\":" + String(ad.mid * 100) + ",";
    json += "\"high\":" + String(ad.high * 100) + ",";
    json += "\"beat\":" + String(ad.beatDetected ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
  });

  // ── Backup management endpoints ──
  // IMPORTANT: Specific routes (like /api/backups/*) MUST be registered before base routes (/api/backups)
  // because ESPAsyncWebServer matches routes in order and /api/backups acts as a prefix matcher.

  server.on("/api/backups/*", HTTP_DELETE, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    String path = "/bk_" + id + ".json";
    if (SPIFFS.exists(path)) {
      SPIFFS.remove(path);
      
      // Cleanup metadata
      if (SPIFFS.exists("/backups_meta.json")) {
        DynamicJsonDocument metaDoc(8192);
        File metaFile = SPIFFS.open("/backups_meta.json", "r");
        if (metaFile) {
          deserializeJson(metaDoc, metaFile);
          metaFile.close();
        }
        if (metaDoc.containsKey(id)) {
          metaDoc.remove(id);
          File out = SPIFFS.open("/backups_meta.json", "w");
          if (out) {
            serializeJson(metaDoc, out);
            out.close();
          }
        }
      }
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(404, "application/json", "{\"error\":\"Not found\"}");
    }
  });

  server.on("/api/backup-description", HTTP_PUT, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument body(256);
      if (deserializeJson(body, data, len) || !body.containsKey("id") || !body.containsKey("description")) {
        request->send(400, "application/json", "{\"error\":\"Missing id or description\"}");
        return;
      }
      String id = body["id"].as<String>();
      
      DynamicJsonDocument metaDoc(8192);
      if (SPIFFS.exists("/backups_meta.json")) {
        File metaFile = SPIFFS.open("/backups_meta.json", "r");
        if (metaFile) {
          deserializeJson(metaDoc, metaFile);
          metaFile.close();
        }
      }
      
      metaDoc[id] = body["description"].as<String>();
      
      File out = SPIFFS.open("/backups_meta.json", "w");
      if (out) {
        serializeJson(metaDoc, out);
        out.close();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save metadata\"}");
      }
    });

  server.on("/api/backups/*", HTTP_GET, [](AsyncWebServerRequest* request) {
    String url = request->url();
    String id = url.substring(url.lastIndexOf('/') + 1);
    String path = "/bk_" + id + ".json";
    if (SPIFFS.exists(path)) {
      request->send(SPIFFS, path, "application/json", true);
    } else {
      request->send(404, "application/json", "{\"error\":\"Not found\"}");
    }
  });

  server.on("/api/backups/restore-upload", HTTP_POST, [](AsyncWebServerRequest *request) {
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static File uploadFile;
    if (index == 0) {
      uploadFile = SPIFFS.open("/bk_uploaded.json", "w");
    }
    if (uploadFile) {
      uploadFile.write(data, len);
    }
    if (index + len == total) {
      if (uploadFile) {
        uploadFile.close();
      }
      pendingRestoreBackupPath = "/bk_uploaded.json";
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  });

  server.on("/api/backups", HTTP_GET, [](AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(16384); // Increased to fit descriptions
    JsonArray backups = doc.createNestedArray("backups");
    
    DynamicJsonDocument metaDoc(8192);
    if (SPIFFS.exists("/backups_meta.json")) {
      File metaFile = SPIFFS.open("/backups_meta.json", "r");
      if (metaFile) {
        deserializeJson(metaDoc, metaFile);
        metaFile.close();
      }
    }
    
    File root = SPIFFS.open("/");
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        String name = file.name();
        String basename = name;
        if (name.startsWith("/")) {
          basename = name.substring(1);
        }
        
        if (basename.startsWith("bk_") && basename.endsWith(".json")) {
          String tsStr = basename.substring(3, basename.length() - 5);
          uint32_t timestamp = tsStr.toInt();
          JsonObject obj = backups.createNestedObject();
          obj["id"] = tsStr;
          obj["filename"] = basename;
          obj["timestamp"] = timestamp;
          obj["size"] = file.size();
          if (metaDoc.containsKey(tsStr)) {
            obj["description"] = metaDoc[tsStr].as<String>();
          }
        }
        file = root.openNextFile();
      }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/backups", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      uint32_t timestamp = 0;
      DynamicJsonDocument doc(256);
      if (!deserializeJson(doc, data, len) && doc.containsKey("timestamp")) {
        timestamp = doc["timestamp"].as<uint32_t>();
      }
      if (timestamp == 0) {
        timestamp = millis() / 1000;
      }
      
      String path = "/bk_" + String(timestamp) + ".json";
      if (createBackupFile(path)) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(500, "application/json", "{\"error\":\"Failed to create backup\"}");
      }
    });

  server.on("/api/restore", HTTP_POST,
    [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(256);
      if (deserializeJson(doc, data, len) || !doc.containsKey("id")) {
        request->send(400, "application/json", "{\"error\":\"Missing id\"}");
        return;
      }
      String id = doc["id"].as<String>();
      String path = "/bk_" + id + ".json";
      if (SPIFFS.exists(path)) {
        pendingRestoreBackupPath = path;
        Serial.println("Restore scheduled for: " + path);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        Serial.println("Backup file not found: " + path);
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
      }
    });

  server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html").setCacheControl("no-cache, no-store, must-revalidate");
  server.on("/start-show", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Mode Show activé. Le Wi-Fi va être coupé.\"}");
    g_transitionToShowMode = true;
  });
  server.onNotFound(handleNotFound);
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      DynamicJsonDocument doc(4096);
      if (deserializeJson(doc, data, len)) return;
      
      if (doc.containsKey("dmx")) {
        JsonObject dmx = doc["dmx"];
        for (JsonPair kv : dmx) {
          dmxEngine.setVirtualConsoleValue(atoi(kv.key().c_str()), kv.value().as<int>());
        }
      } else if (doc.containsKey("ch") && doc.containsKey("val")) {
        dmxEngine.setVirtualConsoleValue(doc["ch"].as<int>(), doc["val"].as<int>());
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
