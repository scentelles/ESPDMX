// Scene Manager - Handles lighting configurations, profiles, scenes, and shows

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <ArduinoJson.h>
#include <vector>
#include <map>
#include "dmx_engine.h"

struct ChannelDef {
  String name;
  uint8_t offset;
  uint8_t defaultValue;
  String type;  // "dimmer", "color", "position", "speed", "gobo", "other"
};

struct StrobeChannelCfg {
  bool enabled;
  uint8_t offset;
  uint8_t value;
};

// The Catalog Entry
struct FixtureProfile {
  String id;
  String name;
  String type;
  uint8_t channelCount;
  std::vector<ChannelDef> channels;
  StrobeChannelCfg strobeChannel;
};

// Instantiation of a Profile in a Setup
struct FixtureInstance {
  String id;
  String profileId;
  String name;
  uint16_t dmxAddress;
  bool enabled;
};

// Virtual Group
struct VirtualGroupAssignment {
  String instanceId;
  String channelName;
};

struct VirtualGroup {
  String id;
  String name;
  std::vector<VirtualGroupAssignment> assignments;
};

struct FixtureEffect {
  String type;      // "none", "chaser", "sparkle", "up", "sine", "sine2"
  uint8_t speed;    // 1-100
  String colorHex;  // "#ff0000" etc.
  bool reverse;
};

struct FixtureChannelValues {
  String fixtureId; // This is the instanceId
  std::map<String, uint8_t> values;
  FixtureEffect effect;
};

struct VirtualGroupSceneValue {
  String groupId;
  String colorHex;
  int dimmer = -1;
};

struct Scene {
  String id;
  String name;
  String description;
  String icon;
  int groupId = 1; // 1 or 2
  std::vector<FixtureChannelValues> fixtureValues;
  std::vector<VirtualGroupSceneValue> virtualGroupValues;
};

struct ShowStep {
  String sceneId;
  uint32_t duration;
  uint32_t transitionTime;
  bool smoothTransition;
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

// Pedal button configuration
struct PedalButtonConfig {
  String action;    // "none", "smoke", "strobe", "scene", "show"
  String targetId;  // sceneId or showId (when action is "scene" or "show")
};

// Represents a full configuration map
struct SetupDef {
  String id;
  String name;
  std::vector<FixtureInstance> instances;
  std::vector<VirtualGroup> virtualGroups;
  std::vector<Scene> scenes;
  std::vector<Show> shows;
  PedalButtonConfig pedalButtons[3]; // 3 configurable pedal buttons
  PedalButtonConfig blePedalButtons[16]; // 16 configurable BLE pedal buttons
};

// Basic metadata for the index
struct SetupMeta {
  String id;
  String name;
};

class SceneManager {
public:
  SceneManager();
  void begin(DMXEngine* dmxEngine);
  
  // Profiles
  std::vector<FixtureProfile>& getProfiles();
  bool saveProfile(const FixtureProfile& profile);
  bool deleteProfile(const String& id);
  FixtureProfile* getProfile(const String& id);
  
  // Setups (Configurations) Management
  String getSetupsListJSON();
  bool createSetup(const String& id, const String& name);
  bool deleteSetup(const String& id);
  
  // Active Setup State
  String getActiveSetupId();
  bool setActiveSetup(const String& id);
  SetupDef* getActiveSetup();
  bool saveActiveSetup();

  // Modifiers on Active Setup
  bool saveInstance(const FixtureInstance& inst);
  bool deleteInstance(const String& id);
  FixtureInstance* getInstance(const String& id);

  bool saveVirtualGroup(const VirtualGroup& vg);
  bool deleteVirtualGroup(const String& id);
  VirtualGroup* getVirtualGroup(const String& id);

  bool saveScene(const Scene& scene);
  bool deleteScene(const String& id);
  Scene* getScene(const String& id);
  
  bool saveShow(const Show& show);
  bool deleteShow(const String& id);
  Show* getShow(const String& id);

  // Pedal config (per setup)
  String getPedalConfigJSON();
  bool savePedalConfig(int button, const String& action, const String& targetId);
  String getBlePedalConfigJSON();
  bool saveBlePedalConfig(int button, const String& action, const String& targetId, bool autoSave = true);

  // Runtime Controls
  void setVirtualGroupValue(const String& groupId, uint8_t value);
  int resolveDMXAddress(const String& instanceId, const String& channelName);
  
  // JSON serialization
  String getProfilesJSON();
  String getActiveSetupJSON();
  
  // Persistence override trigger
  bool saveAll();
  
private:
  DMXEngine* engine;
  std::vector<FixtureProfile> profiles;
  std::vector<SetupMeta> setupsMeta;
  
  SetupDef activeSetup;
  String activeSetupId;
  
  void loadDefaults();
  void loadDefaultProfiles();
  void loadDefaultSetups();
  
  bool saveProfilesToFile();
  bool loadProfilesFromFile();
  
  bool saveSetupsIndex();
  bool loadSetupsIndex();
  
  bool loadSetupFromFile(const String& id, SetupDef& setup);
  bool saveSetupToFile(const SetupDef& setup);
};

#endif
