#include "scene_manager.h"
#include "SPIFFS.h"

SceneManager::SceneManager() {}

void SceneManager::begin() {
  if (!loadAll()) {
    loadDefaults();
    saveAll();
  }
}

void SceneManager::loadDefaults() {
  fixtures.clear();
  scenes.clear();
  shows.clear();
  
  // Default RGB fixture
  FixtureDef f;
  f.id = "fixture-1";
  f.name = "Main RGB Lights";
  f.type = "rgb";
  f.dmxAddress = 1;
  f.channelCount = 3;
  f.enabled = true;
  f.channels.push_back({"Red", 0, 0, "color"});
  f.channels.push_back({"Green", 1, 0, "color"});
  f.channels.push_back({"Blue", 2, 0, "color"});
  fixtures.push_back(f);
  
  // Default scenes
  Scene s1;
  s1.id = "scene-warm";
  s1.name = "Warm White";
  s1.description = "Classic warm lighting";
  s1.icon = "🟡";
  s1.fixtureValues.push_back({"fixture-1", {{"Red", 255}, {"Green", 180}, {"Blue", 100}}});
  scenes.push_back(s1);
  
  Scene s2;
  s2.id = "scene-red";
  s2.name = "Red";
  s2.description = "Passion and energy";
  s2.icon = "🔴";
  s2.fixtureValues.push_back({"fixture-1", {{"Red", 255}, {"Green", 0}, {"Blue", 0}}});
  scenes.push_back(s2);
  
  Scene s3;
  s3.id = "scene-blue";
  s3.name = "Blue";
  s3.description = "Cool and calm";
  s3.icon = "💙";
  s3.fixtureValues.push_back({"fixture-1", {{"Red", 0}, {"Green", 100}, {"Blue", 255}}});
  scenes.push_back(s3);
  
  // Default show
  Show sh;
  sh.id = "show-cycle";
  sh.name = "Color Cycle";
  sh.description = "Cycles through color scenes";
  sh.icon = "🌈";
  sh.loop = true;
  sh.isRunning = false;
  sh.steps.push_back({"scene-warm", 5000, 2000});
  sh.steps.push_back({"scene-red", 5000, 2000});
  sh.steps.push_back({"scene-blue", 5000, 2000});
  shows.push_back(sh);
}

// ── Fixtures ────────────────────────────────────────────────────────

std::vector<FixtureDef>& SceneManager::getFixtures() {
  return fixtures;
}

bool SceneManager::saveFixture(const FixtureDef& fixture) {
  for (auto& f : fixtures) {
    if (f.id == fixture.id) {
      f = fixture;
      saveFixturesToFile();
      return true;
    }
  }
  fixtures.push_back(fixture);
  saveFixturesToFile();
  return true;
}

bool SceneManager::deleteFixture(const String& id) {
  for (size_t i = 0; i < fixtures.size(); i++) {
    if (fixtures[i].id == id) {
      fixtures.erase(fixtures.begin() + i);
      saveFixturesToFile();
      return true;
    }
  }
  return false;
}

FixtureDef* SceneManager::getFixture(const String& id) {
  for (auto& f : fixtures) {
    if (f.id == id) return &f;
  }
  return nullptr;
}

// ── Scenes ──────────────────────────────────────────────────────────

std::vector<Scene>& SceneManager::getScenes() {
  return scenes;
}

bool SceneManager::saveScene(const Scene& scene) {
  for (auto& s : scenes) {
    if (s.id == scene.id) {
      s = scene;
      saveScenesToFile();
      return true;
    }
  }
  scenes.push_back(scene);
  saveScenesToFile();
  return true;
}

bool SceneManager::deleteScene(const String& id) {
  for (size_t i = 0; i < scenes.size(); i++) {
    if (scenes[i].id == id) {
      scenes.erase(scenes.begin() + i);
      saveScenesToFile();
      return true;
    }
  }
  return false;
}

Scene* SceneManager::getScene(const String& id) {
  for (auto& s : scenes) {
    if (s.id == id) return &s;
  }
  return nullptr;
}

// ── Shows ───────────────────────────────────────────────────────────

std::vector<Show>& SceneManager::getShows() {
  return shows;
}

bool SceneManager::saveShow(const Show& show) {
  for (auto& s : shows) {
    if (s.id == show.id) {
      s = show;
      saveShowsToFile();
      return true;
    }
  }
  shows.push_back(show);
  saveShowsToFile();
  return true;
}

bool SceneManager::deleteShow(const String& id) {
  for (size_t i = 0; i < shows.size(); i++) {
    if (shows[i].id == id) {
      shows.erase(shows.begin() + i);
      saveShowsToFile();
      return true;
    }
  }
  return false;
}

Show* SceneManager::getShow(const String& id) {
  for (auto& s : shows) {
    if (s.id == id) return &s;
  }
  return nullptr;
}

// ── JSON Serialization ──────────────────────────────────────────────

String SceneManager::getFixturesJSON() {
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("fixtures");
  
  for (const auto& f : fixtures) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = f.id;
    obj["name"] = f.name;
    obj["type"] = f.type;
    obj["dmxAddress"] = f.dmxAddress;
    obj["channelCount"] = f.channelCount;
    obj["enabled"] = f.enabled;
    JsonArray chArr = obj.createNestedArray("channels");
    for (const auto& ch : f.channels) {
      JsonObject chObj = chArr.createNestedObject();
      chObj["name"] = ch.name;
      chObj["offset"] = ch.offset;
      chObj["defaultValue"] = ch.defaultValue;
      chObj["type"] = ch.type;
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

String SceneManager::getScenesJSON() {
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.createNestedArray("scenes");
  
  for (const auto& s : scenes) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = s.id;
    obj["name"] = s.name;
    obj["description"] = s.description;
    obj["icon"] = s.icon;
    JsonArray fvArr = obj.createNestedArray("fixtureValues");
    for (const auto& fv : s.fixtureValues) {
      JsonObject fvObj = fvArr.createNestedObject();
      fvObj["fixtureId"] = fv.fixtureId;
      JsonObject vals = fvObj.createNestedObject("values");
      for (const auto& kv : fv.values) {
        vals[kv.first] = kv.second;
      }
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

String SceneManager::getShowsJSON() {
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.createNestedArray("shows");
  
  for (const auto& s : shows) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = s.id;
    obj["name"] = s.name;
    obj["description"] = s.description;
    obj["icon"] = s.icon;
    obj["loop"] = s.loop;
    obj["isRunning"] = s.isRunning;
    JsonArray stepsArr = obj.createNestedArray("steps");
    for (const auto& step : s.steps) {
      JsonObject stepObj = stepsArr.createNestedObject();
      stepObj["sceneId"] = step.sceneId;
      stepObj["duration"] = step.duration;
      stepObj["transitionTime"] = step.transitionTime;
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

// ── File Persistence ────────────────────────────────────────────────

bool SceneManager::saveAll() {
  return saveFixturesToFile() && saveScenesToFile() && saveShowsToFile();
}

bool SceneManager::loadAll() {
  bool ok = loadFixturesFromFile();
  ok = loadScenesFromFile() && ok;
  ok = loadShowsFromFile() && ok;
  return ok;
}

bool SceneManager::saveFixturesToFile() {
  File file = SPIFFS.open("/fixtures.json", "w");
  if (!file) return false;
  
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& f : fixtures) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = f.id;
    obj["name"] = f.name;
    obj["type"] = f.type;
    obj["dmxAddress"] = f.dmxAddress;
    obj["channelCount"] = f.channelCount;
    obj["enabled"] = f.enabled;
    JsonArray chArr = obj.createNestedArray("channels");
    for (const auto& ch : f.channels) {
      JsonObject chObj = chArr.createNestedObject();
      chObj["name"] = ch.name;
      chObj["offset"] = ch.offset;
      chObj["defaultValue"] = ch.defaultValue;
      chObj["type"] = ch.type;
    }
  }
  serializeJson(doc, file);
  file.close();
  return true;
}

bool SceneManager::loadFixturesFromFile() {
  if (!SPIFFS.exists("/fixtures.json")) return false;
  File file = SPIFFS.open("/fixtures.json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, file)) { file.close(); return false; }
  file.close();
  
  fixtures.clear();
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    FixtureDef f;
    f.id = obj["id"].as<String>();
    f.name = obj["name"].as<String>();
    f.type = obj["type"].as<String>();
    f.dmxAddress = obj["dmxAddress"] | 1;
    f.channelCount = obj["channelCount"] | 1;
    f.enabled = obj["enabled"] | true;
    JsonArray chArr = obj["channels"];
    for (JsonObject chObj : chArr) {
      ChannelDef ch;
      ch.name = chObj["name"].as<String>();
      ch.offset = chObj["offset"] | 0;
      ch.defaultValue = chObj["defaultValue"] | 0;
      ch.type = chObj["type"].as<String>();
      f.channels.push_back(ch);
    }
    fixtures.push_back(f);
  }
  return true;
}

bool SceneManager::saveScenesToFile() {
  File file = SPIFFS.open("/scenes.json", "w");
  if (!file) return false;
  
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& s : scenes) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = s.id;
    obj["name"] = s.name;
    obj["description"] = s.description;
    obj["icon"] = s.icon;
    JsonArray fvArr = obj.createNestedArray("fixtureValues");
    for (const auto& fv : s.fixtureValues) {
      JsonObject fvObj = fvArr.createNestedObject();
      fvObj["fixtureId"] = fv.fixtureId;
      JsonObject vals = fvObj.createNestedObject("values");
      for (const auto& kv : fv.values) {
        vals[kv.first] = kv.second;
      }
    }
  }
  serializeJson(doc, file);
  file.close();
  return true;
}

bool SceneManager::loadScenesFromFile() {
  if (!SPIFFS.exists("/scenes.json")) return false;
  File file = SPIFFS.open("/scenes.json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc, file)) { file.close(); return false; }
  file.close();
  
  scenes.clear();
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    Scene s;
    s.id = obj["id"].as<String>();
    s.name = obj["name"].as<String>();
    s.description = obj["description"].as<String>();
    s.icon = obj["icon"].as<String>();
    JsonArray fvArr = obj["fixtureValues"];
    for (JsonObject fvObj : fvArr) {
      FixtureChannelValues fv;
      fv.fixtureId = fvObj["fixtureId"].as<String>();
      JsonObject vals = fvObj["values"];
      for (JsonPair kv : vals) {
        fv.values[String(kv.key().c_str())] = kv.value().as<uint8_t>();
      }
      s.fixtureValues.push_back(fv);
    }
    scenes.push_back(s);
  }
  return true;
}

bool SceneManager::saveShowsToFile() {
  File file = SPIFFS.open("/shows.json", "w");
  if (!file) return false;
  
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& s : shows) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = s.id;
    obj["name"] = s.name;
    obj["description"] = s.description;
    obj["icon"] = s.icon;
    obj["loop"] = s.loop;
    JsonArray stepsArr = obj.createNestedArray("steps");
    for (const auto& step : s.steps) {
      JsonObject stepObj = stepsArr.createNestedObject();
      stepObj["sceneId"] = step.sceneId;
      stepObj["duration"] = step.duration;
      stepObj["transitionTime"] = step.transitionTime;
    }
  }
  serializeJson(doc, file);
  file.close();
  return true;
}

bool SceneManager::loadShowsFromFile() {
  if (!SPIFFS.exists("/shows.json")) return false;
  File file = SPIFFS.open("/shows.json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, file)) { file.close(); return false; }
  file.close();
  
  shows.clear();
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    Show s;
    s.id = obj["id"].as<String>();
    s.name = obj["name"].as<String>();
    s.description = obj["description"].as<String>();
    s.icon = obj["icon"].as<String>();
    s.loop = obj["loop"] | true;
    s.isRunning = false;
    JsonArray stepsArr = obj["steps"];
    for (JsonObject stepObj : stepsArr) {
      ShowStep step;
      step.sceneId = stepObj["sceneId"].as<String>();
      step.duration = stepObj["duration"] | 5000;
      step.transitionTime = stepObj["transitionTime"] | 1000;
      s.steps.push_back(step);
    }
    shows.push_back(s);
  }
  return true;
}
