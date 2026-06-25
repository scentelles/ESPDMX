#include "scene_manager.h"
#include "SPIFFS.h"

SceneManager::SceneManager() : engine(nullptr) {}

void SceneManager::begin(DMXEngine* dmxEngine) {
  this->engine = dmxEngine;
  
  bool profilesOk = loadProfilesFromFile();
  bool setupsIndexOk = loadSetupsIndex();
  
  if (!profilesOk) {
    loadDefaultProfiles();
    saveProfilesToFile();
  }
  
  if (!setupsIndexOk || setupsMeta.empty()) {
    loadDefaultSetups();
    saveSetupsIndex();
  }
  
  if (setupsMeta.size() > 0) {
    // Restore persisted active setup, fallback to first
    String savedId = "";
    if (SPIFFS.exists("/active_setup.txt")) {
      File f = SPIFFS.open("/active_setup.txt", "r");
      if (f) {
        savedId = f.readString();
        savedId.trim();
        f.close();
      }
    }
    
    // Verify the saved ID exists in the setups list
    bool found = false;
    if (savedId.length() > 0) {
      for (const auto& meta : setupsMeta) {
        if (meta.id == savedId) { found = true; break; }
      }
    }
    
    setActiveSetup(found ? savedId : setupsMeta[0].id);
  }
}

void SceneManager::loadDefaultProfiles() {
  profiles.clear();
  FixtureProfile p;
  p.id = "prof-rgbwd";
  p.name = "Generic RGBW+Dimmer";
  p.type = "rgbwd";
  p.channelCount = 8;
  p.channels.push_back({"Dimmer", 0, 0, "dimmer"});
  p.channels.push_back({"Red", 4, 0, "color"});
  p.channels.push_back({"Green", 5, 0, "color"});
  p.channels.push_back({"Blue", 6, 0, "color"});
  p.channels.push_back({"White", 7, 0, "color"});
  p.strobeChannel = {false, 0, 255};
  profiles.push_back(p);
}

void SceneManager::loadDefaultSetups() {
  setupsMeta.clear();
  setupsMeta.push_back({"setup-1", "Default Setup"});
  
  SetupDef s;
  s.id = "setup-1";
  s.name = "Default Setup";
  
  FixtureInstance inst;
  inst.id = "inst-1";
  inst.profileId = "prof-rgbwd";
  inst.name = "Main Lights 1";
  inst.dmxAddress = 1;
  inst.enabled = true;
  s.instances.push_back(inst);
  
  Scene s1;
  s1.id = "scene-warm";
  s1.name = "Warm White";
  s1.description = "Classic warm lighting";
  s1.icon = "🟡";
  s1.fixtureValues.push_back({"inst-1", {{"Dimmer", 255}, {"Red", 255}, {"Green", 180}, {"Blue", 100}, {"White", 200}}});
  s.scenes.push_back(s1);

  saveSetupToFile(s);
}

// ── Profiles ────────────────────────────────────────────────────────

std::vector<FixtureProfile>& SceneManager::getProfiles() {
  return profiles;
}

bool SceneManager::saveProfile(const FixtureProfile& profile) {
  for (auto& p : profiles) {
    if (p.id == profile.id) {
      p = profile;
      saveProfilesToFile();
      return true;
    }
  }
  profiles.push_back(profile);
  saveProfilesToFile();
  return true;
}

bool SceneManager::deleteProfile(const String& id) {
  for (size_t i = 0; i < profiles.size(); i++) {
    if (profiles[i].id == id) {
      profiles.erase(profiles.begin() + i);
      saveProfilesToFile();
      return true;
    }
  }
  return false;
}

FixtureProfile* SceneManager::getProfile(const String& id) {
  for (auto& p : profiles) {
    if (p.id == id) return &p;
  }
  return nullptr;
}

// ── Setups Index ────────────────────────────────────────────────────

String SceneManager::getSetupsListJSON() {
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("setups");
  
  for (const auto& meta : setupsMeta) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = meta.id;
    obj["name"] = meta.name;
    if (meta.id == activeSetupId) {
      obj["active"] = true;
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool SceneManager::createSetup(const String& id, const String& name) {
  for (const auto& meta : setupsMeta) {
    if (meta.id == id) return false; // Already exists
  }
  
  setupsMeta.push_back({id, name});
  saveSetupsIndex();
  
  SetupDef s;
  s.id = id;
  s.name = name;
  saveSetupToFile(s);
  
  return true;
}

bool SceneManager::deleteSetup(const String& id) {
  if (id == activeSetupId) return false; // Cannot delete active setup
  
  for (size_t i = 0; i < setupsMeta.size(); i++) {
    if (setupsMeta[i].id == id) {
      setupsMeta.erase(setupsMeta.begin() + i);
      saveSetupsIndex();
      SPIFFS.remove("/setup_" + id + ".json");
      return true;
    }
  }
  return false;
}

// ── Active Setup State ──────────────────────────────────────────────

String SceneManager::getActiveSetupId() {
  return activeSetupId;
}

bool SceneManager::setActiveSetup(const String& id) {
  if (activeSetupId == id) return true;
  
  SetupDef newSetup;
  if (!loadSetupFromFile(id, newSetup)) {
    return false;
  }
  
  // Important: Reset DMX & Shows
  if (engine != nullptr) {
    for (int i = 1; i <= 512; i++) {
      engine->setChannelValue(i, 0);
    }
    engine->clearFixtures();
    
    for (const auto& f : newSetup.instances) {
      if (f.enabled) {
        FixtureProfile* profile = getProfile(f.profileId);
        if (profile) {
          engine->addFixture(0, f.dmxAddress, profile->channelCount);
        }
      }
    }
  }
  
  // Stop shows managed externally (app logic can handle that, or we rely on systemState in main.cpp)
  
  activeSetup = newSetup;
  activeSetupId = id;
  
  // Persist active setup ID for reboot recovery
  File f = SPIFFS.open("/active_setup.txt", "w");
  if (f) {
    f.print(id);
    f.close();
  }
  
  return true;
}

SetupDef* SceneManager::getActiveSetup() {
  return &activeSetup;
}

bool SceneManager::saveActiveSetup() {
  if (activeSetupId.isEmpty()) return false;
  return saveSetupToFile(activeSetup);
}

// ── Modifiers on Active Setup ───────────────────────────────────────

bool SceneManager::saveInstance(const FixtureInstance& inst) {
  for (auto& f : activeSetup.instances) {
    if (f.id == inst.id) {
      f = inst;
      saveActiveSetup();
      return true;
    }
  }
  activeSetup.instances.push_back(inst);
  saveActiveSetup();
  return true;
}

bool SceneManager::deleteInstance(const String& id) {
  for (size_t i = 0; i < activeSetup.instances.size(); i++) {
    if (activeSetup.instances[i].id == id) {
      activeSetup.instances.erase(activeSetup.instances.begin() + i);
      saveActiveSetup();
      return true;
    }
  }
  return false;
}

FixtureInstance* SceneManager::getInstance(const String& id) {
  for (auto& f : activeSetup.instances) {
    if (f.id == id) return &f;
  }
  return nullptr;
}

bool SceneManager::saveVirtualGroup(const VirtualGroup& vg) {
  for (auto& v : activeSetup.virtualGroups) {
    if (v.id == vg.id) {
      v = vg;
      saveActiveSetup();
      return true;
    }
  }
  activeSetup.virtualGroups.push_back(vg);
  saveActiveSetup();
  return true;
}

bool SceneManager::deleteVirtualGroup(const String& id) {
  for (size_t i = 0; i < activeSetup.virtualGroups.size(); i++) {
    if (activeSetup.virtualGroups[i].id == id) {
      activeSetup.virtualGroups.erase(activeSetup.virtualGroups.begin() + i);
      saveActiveSetup();
      return true;
    }
  }
  return false;
}

VirtualGroup* SceneManager::getVirtualGroup(const String& id) {
  for (auto& v : activeSetup.virtualGroups) {
    if (v.id == id) return &v;
  }
  return nullptr;
}

bool SceneManager::saveScene(const Scene& scene) {
  for (auto& s : activeSetup.scenes) {
    if (s.id == scene.id) {
      s = scene;
      saveActiveSetup();
      return true;
    }
  }
  activeSetup.scenes.push_back(scene);
  saveActiveSetup();
  return true;
}

bool SceneManager::deleteScene(const String& id) {
  for (size_t i = 0; i < activeSetup.scenes.size(); i++) {
    if (activeSetup.scenes[i].id == id) {
      activeSetup.scenes.erase(activeSetup.scenes.begin() + i);
      saveActiveSetup();
      return true;
    }
  }
  return false;
}

Scene* SceneManager::getScene(const String& id) {
  for (auto& s : activeSetup.scenes) {
    if (s.id == id) return &s;
  }
  return nullptr;
}

bool SceneManager::saveShow(const Show& show) {
  for (auto& s : activeSetup.shows) {
    if (s.id == show.id) {
      s = show;
      saveActiveSetup();
      return true;
    }
  }
  activeSetup.shows.push_back(show);
  saveActiveSetup();
  return true;
}

bool SceneManager::deleteShow(const String& id) {
  for (size_t i = 0; i < activeSetup.shows.size(); i++) {
    if (activeSetup.shows[i].id == id) {
      activeSetup.shows.erase(activeSetup.shows.begin() + i);
      saveActiveSetup();
      return true;
    }
  }
  return false;
}

Show* SceneManager::getShow(const String& id) {
  for (auto& s : activeSetup.shows) {
    if (s.id == id) return &s;
  }
  return nullptr;
}

// ── JSON Serialization ──────────────────────────────────────────────

String SceneManager::getProfilesJSON() {
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.createNestedArray("profiles");
  
  for (const auto& p : profiles) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = p.id;
    obj["name"] = p.name;
    obj["type"] = p.type;
    obj["channelCount"] = p.channelCount;
    JsonArray chArr = obj.createNestedArray("channels");
    for (const auto& ch : p.channels) {
      JsonObject chObj = chArr.createNestedObject();
      chObj["name"] = ch.name;
      chObj["offset"] = ch.offset;
      chObj["defaultValue"] = ch.defaultValue;
      chObj["type"] = ch.type;
    }
    JsonObject strobe = obj.createNestedObject("strobeChannel");
    strobe["enabled"] = p.strobeChannel.enabled;
    strobe["offset"] = p.strobeChannel.offset;
    strobe["value"] = p.strobeChannel.value;
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

String SceneManager::getActiveSetupJSON() {
  DynamicJsonDocument doc(16384);
  JsonObject obj = doc.createNestedObject("setup");
  obj["id"] = activeSetup.id;
  obj["name"] = activeSetup.name;
  
  JsonArray instArr = obj.createNestedArray("instances");
  for (const auto& i : activeSetup.instances) {
    JsonObject iObj = instArr.createNestedObject();
    iObj["id"] = i.id;
    iObj["profileId"] = i.profileId;
    iObj["name"] = i.name;
    iObj["dmxAddress"] = i.dmxAddress;
    iObj["enabled"] = i.enabled;
  }
  
  JsonArray vgArr = obj.createNestedArray("virtualGroups");
  for (const auto& vg : activeSetup.virtualGroups) {
    JsonObject vgObj = vgArr.createNestedObject();
    vgObj["id"] = vg.id;
    vgObj["name"] = vg.name;
    JsonArray assignArr = vgObj.createNestedArray("assignments");
    for (const auto& a : vg.assignments) {
      JsonObject aObj = assignArr.createNestedObject();
      aObj["instanceId"] = a.instanceId;
      aObj["channelName"] = a.channelName;
    }
  }
  
  JsonArray scenesArr = obj.createNestedArray("scenes");
  for (const auto& s : activeSetup.scenes) {
    JsonObject sObj = scenesArr.createNestedObject();
    sObj["id"] = s.id;
    sObj["name"] = s.name;
    sObj["description"] = s.description;
    sObj["icon"] = s.icon;
    sObj["groupId"] = s.groupId;
    JsonArray fvArr = sObj.createNestedArray("fixtureValues");
    for (const auto& fv : s.fixtureValues) {
      JsonObject fvObj = fvArr.createNestedObject();
      fvObj["fixtureId"] = fv.fixtureId;
      JsonObject vals = fvObj.createNestedObject("values");
      for (const auto& kv : fv.values) {
        vals[kv.first] = kv.second;
      }
    }
    JsonArray vgvArr = sObj.createNestedArray("virtualGroupValues");
    for (const auto& vgv : s.virtualGroupValues) {
      JsonObject vgvObj = vgvArr.createNestedObject();
      vgvObj["groupId"] = vgv.groupId;
      if (vgv.colorHex.length() > 0) vgvObj["colorHex"] = vgv.colorHex;
      if (vgv.dimmer >= 0) vgvObj["dimmer"] = vgv.dimmer;
    }
  }
  
  JsonArray showsArr = obj.createNestedArray("shows");
  for (const auto& s : activeSetup.shows) {
    JsonObject sObj = showsArr.createNestedObject();
    sObj["id"] = s.id;
    sObj["name"] = s.name;
    sObj["description"] = s.description;
    sObj["icon"] = s.icon;
    sObj["loop"] = s.loop;
    sObj["isRunning"] = s.isRunning;
    JsonArray stepsArr = sObj.createNestedArray("steps");
    for (const auto& step : s.steps) {
      JsonObject stepObj = stepsArr.createNestedObject();
      stepObj["sceneId"] = step.sceneId;
      stepObj["duration"] = step.duration;
      stepObj["transitionTime"] = step.transitionTime;
      stepObj["smoothTransition"] = step.smoothTransition;
    }
  }
  
  // Pedal Config
  JsonArray pedalArr = obj.createNestedArray("pedalConfig");
  for (int i = 0; i < 3; i++) {
    JsonObject pObj = pedalArr.createNestedObject();
    pObj["action"] = activeSetup.pedalButtons[i].action.length() > 0 ? activeSetup.pedalButtons[i].action : "none";
    pObj["targetId"] = activeSetup.pedalButtons[i].targetId;
  }

  // BLE Pedal Config
  JsonArray blePedalArr = obj.createNestedArray("blePedalConfig");
  for (int i = 0; i < 16; i++) {
    JsonObject pObj = blePedalArr.createNestedObject();
    pObj["action"] = activeSetup.blePedalButtons[i].action.length() > 0 ? activeSetup.blePedalButtons[i].action : "none";
    pObj["targetId"] = activeSetup.blePedalButtons[i].targetId;
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

// ── Virtual Groups Logic ────────────────────────────────────────────

int SceneManager::resolveDMXAddress(const String& instanceId, const String& channelName) {
  FixtureInstance* inst = getInstance(instanceId);
  if (!inst) return -1;
  FixtureProfile* prof = getProfile(inst->profileId);
  if (!prof) return -1;
  
  for (const auto& ch : prof->channels) {
    if (ch.name == channelName) {
      return inst->dmxAddress + ch.offset;
    }
  }
  return -1;
}

void SceneManager::setVirtualGroupValue(const String& groupId, uint8_t value) {
  if (engine == nullptr) return;
  VirtualGroup* vg = getVirtualGroup(groupId);
  if (!vg) return;
  
  for (const auto& a : vg->assignments) {
    FixtureInstance* inst = getInstance(a.instanceId);
    if (!inst || !inst->enabled) continue;
    FixtureProfile* prof = getProfile(inst->profileId);
    if (!prof) continue;
    
    for (const auto& ch : prof->channels) {
      if (a.channelName != "ALL" && a.channelName != ch.name) continue;
      
      if (ch.type.equalsIgnoreCase("dimmer") || ch.name.equalsIgnoreCase("dimmer") || ch.name.equalsIgnoreCase("dim")) {
        int addr = inst->dmxAddress + ch.offset;
        if (addr >= 1 && addr <= 512) {
          engine->setVirtualConsoleValue(addr, value);
        }
      }
    }
  }
}

// ── Pedal Config ────────────────────────────────────────────────────

String SceneManager::getPedalConfigJSON() {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("pedalConfig");
  for (int i = 0; i < 3; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["button"] = i + 1;
    obj["action"] = activeSetup.pedalButtons[i].action.length() > 0 ? activeSetup.pedalButtons[i].action : "none";
    obj["targetId"] = activeSetup.pedalButtons[i].targetId;
  }
  String result;
  serializeJson(doc, result);
  return result;
}

bool SceneManager::savePedalConfig(int button, const String& action, const String& targetId) {
  if (button < 1 || button > 3) return false;
  activeSetup.pedalButtons[button - 1].action = action;
  activeSetup.pedalButtons[button - 1].targetId = targetId;
  saveActiveSetup();
  return true;
}

String SceneManager::getBlePedalConfigJSON() {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["ccId"] = i + 1;
    obj["action"] = activeSetup.blePedalButtons[i].action.length() > 0 ? activeSetup.blePedalButtons[i].action : "none";
    obj["param"] = activeSetup.blePedalButtons[i].targetId;
  }
  String result;
  serializeJson(doc, result);
  return result;
}

bool SceneManager::saveBlePedalConfig(int button, const String& action, const String& targetId, bool autoSave) {
  if (button < 1 || button > 16) return false;
  activeSetup.blePedalButtons[button - 1].action = action;
  activeSetup.blePedalButtons[button - 1].targetId = targetId;
  if (autoSave) saveActiveSetup();
  return true;
}

// ── Persistence ─────────────────────────────────────────────────────

bool SceneManager::saveAll() {
  return saveProfilesToFile() && saveSetupsIndex() && saveActiveSetup();
}

bool SceneManager::saveProfilesToFile() {
  File file = SPIFFS.open("/profiles.json", "w");
  if (!file) return false;
  
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& p : profiles) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = p.id;
    obj["name"] = p.name;
    obj["type"] = p.type;
    obj["channelCount"] = p.channelCount;
    JsonArray chArr = obj.createNestedArray("channels");
    for (const auto& ch : p.channels) {
      JsonObject chObj = chArr.createNestedObject();
      chObj["name"] = ch.name;
      chObj["offset"] = ch.offset;
      chObj["defaultValue"] = ch.defaultValue;
      chObj["type"] = ch.type;
    }
    JsonObject strobe = obj.createNestedObject("strobeChannel");
    strobe["enabled"] = p.strobeChannel.enabled;
    strobe["offset"] = p.strobeChannel.offset;
    strobe["value"] = p.strobeChannel.value;
  }
  serializeJson(doc, file);
  file.close();
  return true;
}

bool SceneManager::loadProfilesFromFile() {
  if (!SPIFFS.exists("/profiles.json")) return false;
  File file = SPIFFS.open("/profiles.json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc, file)) { file.close(); return false; }
  file.close();
  
  profiles.clear();
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    FixtureProfile p;
    p.id = obj["id"].as<String>();
    p.name = obj["name"].as<String>();
    p.type = obj["type"].as<String>();
    p.channelCount = obj["channelCount"] | 1;
    JsonArray chArr = obj["channels"];
    for (JsonObject chObj : chArr) {
      ChannelDef ch;
      ch.name = chObj["name"].as<String>();
      ch.offset = chObj["offset"] | 0;
      ch.defaultValue = chObj["defaultValue"] | 0;
      ch.type = chObj["type"].as<String>();
      p.channels.push_back(ch);
    }
    if (obj.containsKey("strobeChannel")) {
      JsonObject sc = obj["strobeChannel"];
      p.strobeChannel.enabled = sc["enabled"] | false;
      p.strobeChannel.offset = sc["offset"] | 0;
      p.strobeChannel.value = sc["value"] | 255;
    } else {
      p.strobeChannel = {false, 0, 255};
    }
    profiles.push_back(p);
  }
  return true;
}

bool SceneManager::saveSetupsIndex() {
  File file = SPIFFS.open("/setups.json", "w");
  if (!file) return false;
  
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& meta : setupsMeta) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = meta.id;
    obj["name"] = meta.name;
  }
  serializeJson(doc, file);
  file.close();
  return true;
}

bool SceneManager::loadSetupsIndex() {
  if (!SPIFFS.exists("/setups.json")) return false;
  File file = SPIFFS.open("/setups.json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, file)) { file.close(); return false; }
  file.close();
  
  setupsMeta.clear();
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    setupsMeta.push_back({obj["id"].as<String>(), obj["name"].as<String>()});
  }
  return true;
}

bool SceneManager::saveSetupToFile(const SetupDef& setup) {
  File file = SPIFFS.open("/setup_" + setup.id + ".json", "w");
  if (!file) return false;
  
  // High capacity for full setup
  DynamicJsonDocument doc(16384);
  doc["id"] = setup.id;
  doc["name"] = setup.name;
  
  JsonArray instArr = doc.createNestedArray("instances");
  for (const auto& i : setup.instances) {
    JsonObject iObj = instArr.createNestedObject();
    iObj["id"] = i.id;
    iObj["profileId"] = i.profileId;
    iObj["name"] = i.name;
    iObj["dmxAddress"] = i.dmxAddress;
    iObj["enabled"] = i.enabled;
  }
  
  JsonArray vgArr = doc.createNestedArray("virtualGroups");
  for (const auto& vg : setup.virtualGroups) {
    JsonObject vgObj = vgArr.createNestedObject();
    vgObj["id"] = vg.id;
    vgObj["name"] = vg.name;
    JsonArray assignArr = vgObj.createNestedArray("assignments");
    for (const auto& a : vg.assignments) {
      JsonObject aObj = assignArr.createNestedObject();
      aObj["instanceId"] = a.instanceId;
      aObj["channelName"] = a.channelName;
    }
  }
  
  JsonArray scenesArr = doc.createNestedArray("scenes");
  for (const auto& s : setup.scenes) {
    JsonObject sObj = scenesArr.createNestedObject();
    sObj["id"] = s.id;
    sObj["name"] = s.name;
    sObj["description"] = s.description;
    sObj["icon"] = s.icon;
    sObj["groupId"] = s.groupId;
    JsonArray fvArr = sObj.createNestedArray("fixtureValues");
    for (const auto& fv : s.fixtureValues) {
      JsonObject fvObj = fvArr.createNestedObject();
      fvObj["fixtureId"] = fv.fixtureId;
      if (fv.effect.type.length() > 0 && fv.effect.type != "none") {
        JsonObject fxObj = fvObj.createNestedObject("effect");
        fxObj["type"] = fv.effect.type;
        fxObj["speed"] = fv.effect.speed;
        fxObj["colorHex"] = fv.effect.colorHex;
        fxObj["reverse"] = fv.effect.reverse;
      }
      JsonObject vals = fvObj.createNestedObject("values");
      for (const auto& kv : fv.values) {
        vals[kv.first] = kv.second;
      }
    }
    JsonArray vgvArr = sObj.createNestedArray("virtualGroupValues");
    for (const auto& vgv : s.virtualGroupValues) {
      JsonObject vgvObj = vgvArr.createNestedObject();
      vgvObj["groupId"] = vgv.groupId;
      if (vgv.colorHex.length() > 0) vgvObj["colorHex"] = vgv.colorHex;
      if (vgv.dimmer >= 0) vgvObj["dimmer"] = vgv.dimmer;
    }
  }
  
  JsonArray showsArr = doc.createNestedArray("shows");
  for (const auto& s : setup.shows) {
    JsonObject sObj = showsArr.createNestedObject();
    sObj["id"] = s.id;
    sObj["name"] = s.name;
    sObj["description"] = s.description;
    sObj["icon"] = s.icon;
    sObj["loop"] = s.loop;
    JsonArray stepsArr = sObj.createNestedArray("steps");
    for (const auto& step : s.steps) {
      JsonObject stepObj = stepsArr.createNestedObject();
      stepObj["sceneId"] = step.sceneId;
      stepObj["duration"] = step.duration;
      stepObj["transitionTime"] = step.transitionTime;
      stepObj["smoothTransition"] = step.smoothTransition;
    }
  }
  
  // Pedal Config
  JsonArray pedalArr = doc.createNestedArray("pedalConfig");
  for (int i = 0; i < 3; i++) {
    JsonObject pObj = pedalArr.createNestedObject();
    pObj["action"] = setup.pedalButtons[i].action.length() > 0 ? setup.pedalButtons[i].action : "none";
    pObj["targetId"] = setup.pedalButtons[i].targetId;
  }
  
  // BLE Pedal Config
  JsonArray blePedalArr = doc.createNestedArray("blePedalConfig");
  for (int i = 0; i < 16; i++) {
    JsonObject pObj = blePedalArr.createNestedObject();
    pObj["action"] = setup.blePedalButtons[i].action.length() > 0 ? setup.blePedalButtons[i].action : "none";
    pObj["targetId"] = setup.blePedalButtons[i].targetId;
  }
  
  serializeJson(doc, file);
  file.close();
  return true;
}

bool SceneManager::loadSetupFromFile(const String& id, SetupDef& setup) {
  if (!SPIFFS.exists("/setup_" + id + ".json")) return false;
  File file = SPIFFS.open("/setup_" + id + ".json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc, file)) { file.close(); return false; }
  file.close();
  
  setup.id = doc["id"].as<String>();
  setup.name = doc["name"].as<String>();
  
  setup.instances.clear();
  JsonArray instArr = doc["instances"];
  for (JsonObject iObj : instArr) {
    FixtureInstance i;
    i.id = iObj["id"].as<String>();
    i.profileId = iObj["profileId"].as<String>();
    i.name = iObj["name"].as<String>();
    i.dmxAddress = iObj["dmxAddress"] | 1;
    i.enabled = iObj["enabled"] | true;
    setup.instances.push_back(i);
  }
  
  setup.virtualGroups.clear();
  JsonArray vgArr = doc["virtualGroups"];
  for (JsonObject vgObj : vgArr) {
    VirtualGroup vg;
    vg.id = vgObj["id"].as<String>();
    vg.name = vgObj["name"].as<String>();
    JsonArray aArr = vgObj["assignments"];
    for (JsonObject aObj : aArr) {
      VirtualGroupAssignment a;
      a.instanceId = aObj["instanceId"].as<String>();
      a.channelName = aObj["channelName"].as<String>();
      vg.assignments.push_back(a);
    }
    setup.virtualGroups.push_back(vg);
  }
  
  setup.scenes.clear();
  JsonArray scenesArr = doc["scenes"];
  for (JsonObject sObj : scenesArr) {
    Scene s;
    s.id = sObj["id"].as<String>();
    s.name = sObj["name"].as<String>();
    s.description = sObj["description"].as<String>();
    s.icon = sObj["icon"].as<String>();
    s.groupId = sObj["groupId"] | 1;
    JsonArray fvArr = sObj["fixtureValues"];
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
      for (JsonPair kv : vals) {
        fv.values[String(kv.key().c_str())] = kv.value().as<uint8_t>();
      }
      s.fixtureValues.push_back(fv);
    }
    if (sObj.containsKey("virtualGroupValues")) {
      JsonArray vgvArr = sObj["virtualGroupValues"];
      for (JsonObject vgvObj : vgvArr) {
        VirtualGroupSceneValue vgv;
        vgv.groupId = vgvObj["groupId"].as<String>();
        vgv.colorHex = vgvObj["colorHex"] | "";
        vgv.dimmer = vgvObj.containsKey("dimmer") ? vgvObj["dimmer"].as<int>() : -1;
        s.virtualGroupValues.push_back(vgv);
      }
    }
    setup.scenes.push_back(s);
  }
  
  setup.shows.clear();
  JsonArray showsArr = doc["shows"];
  for (JsonObject sObj : showsArr) {
    Show s;
    s.id = sObj["id"].as<String>();
    s.name = sObj["name"].as<String>();
    s.description = sObj["description"].as<String>();
    s.icon = sObj["icon"].as<String>();
    s.loop = sObj["loop"] | true;
    s.isRunning = false;
    JsonArray stepsArr = sObj["steps"];
    for (JsonObject stepObj : stepsArr) {
      ShowStep step;
      step.sceneId = stepObj["sceneId"].as<String>();
      step.duration = stepObj["duration"] | 5000;
      step.transitionTime = stepObj["transitionTime"] | 1000;
      step.smoothTransition = stepObj["smoothTransition"] | false;
      s.steps.push_back(step);
    }
    setup.shows.push_back(s);
  }
  
  // Load pedal config
  for (int i = 0; i < 3; i++) {
    setup.pedalButtons[i].action = "none";
    setup.pedalButtons[i].targetId = "";
  }
  if (doc.containsKey("pedalConfig")) {
    JsonArray pedalArr = doc["pedalConfig"];
    int idx = 0;
    for (JsonObject pObj : pedalArr) {
      if (idx >= 3) break;
      setup.pedalButtons[idx].action = pObj["action"].as<String>();
      setup.pedalButtons[idx].targetId = pObj["targetId"].as<String>();
      idx++;
    }
  }

  // Load BLE pedal config
  for (int i = 0; i < 16; i++) {
    setup.blePedalButtons[i].action = "none";
    setup.blePedalButtons[i].targetId = "";
  }
  if (doc.containsKey("blePedalConfig")) {
    JsonArray blePedalArr = doc["blePedalConfig"];
    int idx = 0;
    for (JsonObject pObj : blePedalArr) {
      if (idx >= 16) break;
      setup.blePedalButtons[idx].action = pObj["action"].as<String>();
      setup.blePedalButtons[idx].targetId = pObj["targetId"].as<String>();
      idx++;
    }
  }
  
  return true;
}
