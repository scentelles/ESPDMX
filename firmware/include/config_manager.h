// Configuration Manager - Handles persistent storage

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <ArduinoJson.h>

struct SystemConfig {
  char wifiSSID[32];
  char wifiPassword[64];
  char adminPin[10];
  uint32_t dmxBaud;
  uint8_t maxFixtures;
  uint16_t updateInterval;
  uint8_t soundSensitivity;
  uint8_t soundDynamics;
};

class ConfigManager {
public:
  ConfigManager();
  void begin();
  
  // Load/Save
  bool loadConfig();
  bool saveConfig();
  
  // Getters/Setters
  SystemConfig getConfig();
  void setConfig(const SystemConfig& config);
  
  String getJSON();
  bool loadFromJSON(const char* json);
  
private:
  void loadDefaults();
  
  SystemConfig config;
  const char* configFile = "/config.json";
};

#endif
