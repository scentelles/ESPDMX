#include "dmx_engine.h"
#include "config.h"
#include <esp_dmx.h>

// Use DMX port 1 (UART1). Port 0 is used by Serial for debug output.
#define DMX_PORT 1

DMXEngine::DMXEngine() {
  strobeSpeed = 5;
  strobeActive = false;
  masterBrightness = 255;
  lastStrobeToggle = millis();
  strobeState = false;
  dmxTaskHandle = NULL;
  memset(dmxData, 0, sizeof(dmxData));
}

void DMXEngine::begin() {
  // Install esp_dmx driver on UART1
  // TX only — RX pin set to -1, no enable pin (auto-direction RS-485 module)
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {};
  dmx_driver_install(DMX_PORT, &config, personalities, 0);
  dmx_set_pin(DMX_PORT, DMX_TX_PIN, -1, -1);
  
  // Start the DMX output task on Core 0 (web server runs on Core 1)
  xTaskCreatePinnedToCore(
    dmxTask,        // Task function
    "DMX_Task",     // Name
    4096,           // Stack size
    this,           // Parameter (pass DMXEngine instance)
    2,              // Priority (higher than idle, lower than WiFi)
    &dmxTaskHandle, // Task handle
    0               // Core 0 (separate from Arduino loop on Core 1)
  );
}

// FreeRTOS task: continuously sends DMX frames (~44Hz)
void DMXEngine::dmxTask(void* param) {
  DMXEngine* engine = static_cast<DMXEngine*>(param);
  
  for (;;) {
    // Strobe is now handled per-fixture via strobe channel values
    // set in the HTTP handler — no blanking needed here
    
    // Write and send DMX frame
    dmx_write(DMX_PORT, engine->dmxData, DMX_PACKET_SIZE);
    dmx_send_num(DMX_PORT, DMX_PACKET_SIZE);
    // Block THIS task (not the main loop) until frame is sent
    dmx_wait_sent(DMX_PORT, DMX_TIMEOUT_TICK);
  }
}

void DMXEngine::setChannelValue(uint16_t channel, uint8_t value) {
  if (channel >= 1 && channel <= 512) {
    // dmxData[0] = start code (always 0), channels are 1-indexed in the packet
    dmxData[channel] = value;
  }
}

void DMXEngine::setFixtureValues(uint16_t fixtureId, const uint8_t* values, uint8_t count) {
  if (fixtureId >= fixtures.size()) return;
  
  DMXFixture& fixture = fixtures[fixtureId];
  if (!fixture.enabled) return;
  
  for (uint8_t i = 0; i < count && i < fixture.channels; i++) {
    uint16_t addr = fixture.address + i;
    if (addr >= 1 && addr <= 512) {
      dmxData[addr] = (values[i] * masterBrightness) / 255;
    }
  }
}

uint8_t DMXEngine::getChannelValue(uint16_t channel) {
  if (channel >= 1 && channel <= 512) {
    return dmxData[channel];
  }
  return 0;
}

void DMXEngine::addFixture(uint16_t id, uint16_t address, uint8_t channels) {
  DMXFixture fixture;
  fixture.address = address;
  fixture.channels = channels;
  fixture.enabled = true;
  memset(fixture.values, 0, sizeof(fixture.values));
  fixtures.push_back(fixture);
}

void DMXEngine::removeFixture(uint16_t id) {
  if (id < fixtures.size()) {
    fixtures.erase(fixtures.begin() + id);
  }
}

void DMXEngine::clearFixtures() {
  fixtures.clear();
}

void DMXEngine::setFixtureEnabled(uint16_t id, bool enabled) {
  if (id < fixtures.size()) {
    fixtures[id].enabled = enabled;
  }
}

void DMXEngine::setStrobeSpeed(uint8_t speed) {
  strobeSpeed = constrain(speed, STROBE_MIN_SPEED, STROBE_MAX_SPEED);
}

void DMXEngine::setStrobeActive(bool active) {
  strobeActive = active;
}

void DMXEngine::setMasterBrightness(uint8_t brightness) {
  masterBrightness = brightness;
}
