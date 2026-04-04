#include "config_manager.h"
#include "config.h"
#include "SPIFFS.h"

ConfigManager::ConfigManager() {
  loadDefaults();
}

void ConfigManager::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    loadDefaults();
    return;
  }
  
  if (!loadConfig()) {
    saveConfig();
  }
}

void ConfigManager::loadDefaults() {
  strcpy(config.wifiSSID, WIFI_SSID);
  strcpy(config.wifiPassword, WIFI_PASSWORD);
  strcpy(config.adminPin, ADMIN_PIN);
  config.dmxBaud = DMX_BAUD_RATE;
  config.maxFixtures = MAX_FIXTURES;
  config.updateInterval = UPDATE_INTERVAL;
}

bool ConfigManager::loadConfig() {
  if (!SPIFFS.exists("/config.json")) {
    return false;
  }
  
  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    return false;
  }
  
  size_t size = file.size();
  String content = file.readString();
  file.close();
  
  return loadFromJSON(content.c_str());
}

bool ConfigManager::saveConfig() {
  DynamicJsonDocument doc(512);
  
  doc["wifiSSID"] = config.wifiSSID;
  doc["wifiPassword"] = config.wifiPassword;
  doc["adminPin"] = config.adminPin;
  doc["dmxBaud"] = config.dmxBaud;
  doc["maxFixtures"] = config.maxFixtures;
  doc["updateInterval"] = config.updateInterval;
  
  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    return false;
  }
  
  serializeJson(doc, file);
  file.close();
  return true;
}

SystemConfig ConfigManager::getConfig() {
  return config;
}

void ConfigManager::setConfig(const SystemConfig& newConfig) {
  config = newConfig;
  saveConfig();
}

String ConfigManager::getJSON() {
  DynamicJsonDocument doc(512);
  
  doc["wifiSSID"] = config.wifiSSID;
  doc["wifiPassword"] = config.wifiPassword;
  doc["adminPin"] = config.adminPin;
  doc["dmxBaud"] = config.dmxBaud;
  doc["maxFixtures"] = config.maxFixtures;
  doc["updateInterval"] = config.updateInterval;
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool ConfigManager::loadFromJSON(const char* json) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    return false;
  }
  
  strlcpy(config.wifiSSID, doc["wifiSSID"] | "", sizeof(config.wifiSSID));
  strlcpy(config.wifiPassword, doc["wifiPassword"] | "", sizeof(config.wifiPassword));
  strlcpy(config.adminPin, doc["adminPin"] | "", sizeof(config.adminPin));
  config.dmxBaud = doc["dmxBaud"] | DMX_BAUD_RATE;
  config.maxFixtures = doc["maxFixtures"] | MAX_FIXTURES;
  config.updateInterval = doc["updateInterval"] | UPDATE_INTERVAL;
  
  return true;
}
