// DMX Engine - Handles DMX protocol and rendering via esp_dmx library
// DMX output runs in a dedicated FreeRTOS task on Core 0

#ifndef DMX_ENGINE_H
#define DMX_ENGINE_H

#include <Arduino.h>
#include <esp_dmx.h>
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
  void begin();  // Installs driver AND starts the DMX task
  
  // DMX Output (thread-safe: single-byte writes are atomic)
  void setChannelValue(uint16_t channel, uint8_t value);
  void setFixtureValues(uint16_t fixtureId, const uint8_t* values, uint8_t count);
  uint8_t getChannelValue(uint16_t channel);
  
  // Fixtures
  void addFixture(uint16_t id, uint16_t address, uint8_t channels);
  void removeFixture(uint16_t id);
  void clearFixtures();
  void setFixtureEnabled(uint16_t id, bool enabled);
  
  // Effects
  void setStrobeSpeed(uint8_t speed);  // 1-10
  void setStrobeActive(bool active);
  void setMasterBrightness(uint8_t brightness);  // 0-255
  
private:
  static void dmxTask(void* param);  // FreeRTOS task function
  
  // dmxData[0] = start code (0x00), dmxData[1..512] = channels
  uint8_t dmxData[DMX_PACKET_SIZE];
  std::vector<DMXFixture> fixtures;
  
  uint8_t strobeSpeed;
  volatile bool strobeActive;
  volatile uint8_t masterBrightness;
  uint32_t lastStrobeToggle;
  bool strobeState;
  
  TaskHandle_t dmxTaskHandle;
};

#endif
