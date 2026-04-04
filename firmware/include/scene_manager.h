// Scene Manager - Handles lighting scenes and shows

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <ArduinoJson.h>
#include <vector>
#include <map>

struct ChannelDef {
  String name;
  uint8_t offset;
  uint8_t defaultValue;
  String type;  // "dimmer", "color", "position", "speed", "gobo", "other"
};

struct FixtureDef {
  String id;
  String name;
  String type;
  uint16_t dmxAddress;
  uint8_t channelCount;
  std::vector<ChannelDef> channels;
  bool enabled;
};

struct FixtureChannelValues {
  String fixtureId;
  std::map<String, uint8_t> values;  // channel name -> value
};

struct Scene {
  String id;
  String name;
  String description;
  String icon;
  std::vector<FixtureChannelValues> fixtureValues;
};

struct ShowStep {
  String sceneId;
  uint32_t duration;       // ms to hold
  uint32_t transitionTime; // ms to fade
};

struct Show {
  String id;
  String name;
  String description;
  String icon;
  bool loop;
  std::vector<ShowStep> steps;
  bool isRunning;
};

class SceneManager {
public:
  SceneManager();
  void begin();
  
  // Fixtures
  std::vector<FixtureDef>& getFixtures();
  bool saveFixture(const FixtureDef& fixture);
  bool deleteFixture(const String& id);
  FixtureDef* getFixture(const String& id);
  
  // Scenes
  std::vector<Scene>& getScenes();
  bool saveScene(const Scene& scene);
  bool deleteScene(const String& id);
  Scene* getScene(const String& id);
  
  // Shows
  std::vector<Show>& getShows();
  bool saveShow(const Show& show);
  bool deleteShow(const String& id);
  Show* getShow(const String& id);
  
  // JSON serialization
  String getFixturesJSON();
  String getScenesJSON();
  String getShowsJSON();
  
  // Persistence
  bool saveAll();
  bool loadAll();
  
private:
  std::vector<FixtureDef> fixtures;
  std::vector<Scene> scenes;
  std::vector<Show> shows;
  
  void loadDefaults();
  bool saveFixturesToFile();
  bool loadFixturesFromFile();
  bool saveScenesToFile();
  bool loadScenesFromFile();
  bool saveShowsToFile();
  bool loadShowsFromFile();
};

#endif
