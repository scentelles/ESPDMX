// DMX Engine - Handles DMX protocol and rendering

#ifndef DMX_ENGINE_H
#define DMX_ENGINE_H

#include <Arduino.h>
#include <vector>

struct DMXFixture {
  uint16_t address;      // 1-512
  uint8_t channels;      // Number of channels used
  bool enabled;
  uint8_t values[100];   // Up to 100 channels per fixture
};

class DMXEngine {
public:
  DMXEngine();
  void begin();
  void update();
  
  // DMX Output
  void setChannelValue(uint16_t channel, uint8_t value);
  void setFixtureValues(uint16_t fixtureId, const uint8_t* values, uint8_t count);
  uint8_t getChannelValue(uint16_t channel);
  
  // Fixtures
  void addFixture(uint16_t id, uint16_t address, uint8_t channels);
  void removeFixture(uint16_t id);
  void setFixtureEnabled(uint16_t id, bool enabled);
  
  // Effects
  void setStrobeSpeed(uint8_t speed);  // 1-10
  void setStrobeActive(bool active);
  void setMasterBrightness(uint8_t brightness);  // 0-255
  
private:
  void sendDMXFrame();
  void sendBreakSignal();
  void sendMAB();
  void sendDMXData();
  
  uint8_t dmxBuffer[512];
  std::vector<DMXFixture> fixtures;
  
  uint8_t strobeSpeed;
  bool strobeActive;
  uint8_t masterBrightness;
  uint32_t lastStrobeToggle;
  bool strobeState;
  
  HardwareSerial* dmxSerial;
};

#endif
