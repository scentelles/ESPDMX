#include "dmx_engine.h"
#include "config.h"

DMXEngine::DMXEngine() {
  strobeSpeed = 5;
  strobeActive = false;
  masterBrightness = 255;
  lastStrobeToggle = millis();
  strobeState = false;
  memset(dmxBuffer, 0, sizeof(dmxBuffer));
}

void DMXEngine::begin() {
  // Initialize UART2 for DMX output
  dmxSerial = &Serial2;
  dmxSerial->begin(DMX_BAUD_RATE, SERIAL_8N2, DMX_RX_PIN, DMX_TX_PIN);
  
  // Configure RTS pin for RS-485 direction control
  pinMode(DMX_DE_PIN, OUTPUT);
  digitalWrite(DMX_DE_PIN, LOW);  // Start in receive mode
}

void DMXEngine::update() {
  // Handle strobe effect
  if (strobeActive) {
    uint32_t now = millis();
    uint32_t interval = map(strobeSpeed, 1, 10, 500, 50);  // 500ms to 50ms
    
    if (now - lastStrobeToggle >= interval) {
      strobeState = !strobeState;
      lastStrobeToggle = now;
      
      // Apply strobe by setting brightness
      if (!strobeState) {
        for (int i = 0; i < 512; i++) {
          dmxBuffer[i] = 0;
        }
      }
    }
  }
  
  // Send DMX frame
  sendDMXFrame();
}

void DMXEngine::setChannelValue(uint16_t channel, uint8_t value) {
  if (channel >= 1 && channel <= 512) {
    dmxBuffer[channel - 1] = value;
  }
}

void DMXEngine::setFixtureValues(uint16_t fixtureId, const uint8_t* values, uint8_t count) {
  if (fixtureId >= fixtures.size()) return;
  
  DMXFixture& fixture = fixtures[fixtureId];
  if (!fixture.enabled) return;
  
  uint16_t addr = fixture.address - 1;
  for (uint8_t i = 0; i < count && i < fixture.channels; i++) {
    if (addr + i < 512) {
      dmxBuffer[addr + i] = (values[i] * masterBrightness) / 255;
    }
  }
}

uint8_t DMXEngine::getChannelValue(uint16_t channel) {
  if (channel >= 1 && channel <= 512) {
    return dmxBuffer[channel - 1];
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

void DMXEngine::sendDMXFrame() {
  // Set RS-485 to transmit mode
  digitalWrite(DMX_DE_PIN, HIGH);
  delayMicroseconds(50);
  
  sendBreakSignal();
  sendMAB();
  sendDMXData();
  
  // Set RS-485 to receive mode
  delayMicroseconds(100);
  digitalWrite(DMX_DE_PIN, LOW);
}

void DMXEngine::sendBreakSignal() {
  // Send break: at least 88 microseconds at low level
  dmxSerial->write(0x00);  // UART will send at least 11 bits at LOW
  dmxSerial->flush();
}

void DMXEngine::sendMAB() {
  // Mark After Break: at least 8 microseconds at high level
  delayMicroseconds(DMX_MAB_DURATION);
}

void DMXEngine::sendDMXData() {
  // Send Start Code (0x00)
  dmxSerial->write(0x00);
  
  // Send all 512 channels
  for (int i = 0; i < 512; i++) {
    dmxSerial->write(dmxBuffer[i]);
  }
  
  dmxSerial->flush();
}
