#include "dmx_engine.h"
#include "config.h"
#include "driver/uart.h"

#define DMX_UART UART_NUM_2
// Number of BREAK bit-periods appended after each frame
// At 250000 baud: 1 bit = 4µs → 50 bits = 200µs (spec ≥ 88µs)
#define DMX_BREAK_BITS 50

DMXEngine::DMXEngine() {
  strobeSpeed = 5;
  strobeActive = false;
  masterBrightness = 255;
  lastStrobeToggle = millis();
  strobeState = false;
  memset(dmxBuffer, 0, sizeof(dmxBuffer));
}

void DMXEngine::begin() {
  // Configure UART2 for DMX512 output using ESP-IDF driver directly
  // This gives us access to uart_write_bytes_with_break() for reliable framing
  uart_config_t uart_config = {};
  uart_config.baud_rate = DMX_BAUD_RATE;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_2;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_APB;

  uart_param_config(DMX_UART, &uart_config);
  uart_set_pin(DMX_UART, DMX_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  // TX buffer must be larger than frame (513 bytes) + overhead. 0 = no buffer (blocking write).
  // Using 0 for TX buffer forces fully synchronous writes — safest for DMX timing.
  uart_driver_install(DMX_UART, 256, 0, 0, NULL, 0);
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
  // Build complete DMX frame: start code (0x00) + 512 channel values
  uint8_t frame[513];
  frame[0] = 0x00;  // DMX start code
  memcpy(frame + 1, dmxBuffer, 512);

  // Wait for previous frame to finish transmitting
  uart_wait_tx_done(DMX_UART, pdMS_TO_TICKS(50));

  // Send frame data followed by a BREAK signal (hardware-generated)
  // The BREAK after this frame serves as the BREAK before the next frame.
  // This is handled entirely by the ESP32 UART hardware — no manual timing.
  uart_write_bytes_with_break(DMX_UART, (const char*)frame, 513, DMX_BREAK_BITS);
}
