// Configuration file for ESP32 DMX Lighting Controller (LOLIN D32 + ST7735S TFT)

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "SUDSHOW"
#define WIFI_PASSWORD "12345678"

// Web Server Configuration
#define WEB_PORT 80
#define MAX_JSON_SIZE 2048

// DMX Configuration
#define DMX_TX_PIN 16                // GPIO16 - TX for RS-485 (safe, not a strapping pin)
#define DMX_CHANNELS 512             // Full DMX universe

// I2S Microphone Configuration (INMP441)
#define I2S_WS_PIN  33               // GPIO33 - Word Select (LRCLK)
#define I2S_BCK_PIN 32               // GPIO32 - Bit Clock (BCLK)
#define I2S_SD_PIN  34               // GPIO34 - Serial Data (DOUT) — input-only, reliable

// System Configuration
#define ADMIN_PIN "1234"             // Default admin PIN (change in settings!)
#define UPDATE_INTERVAL 500          // milliseconds between state broadcasts
#define MAX_FIXTURES 64

// TFT Display Configuration (ST7735S 80x160 - SPI, landscape)
// SPI pins configured via build_flags in platformio.ini (TFT_eSPI)
#define TFT_BL_PIN 18                // Backlight control (BLK)
#define SCREEN_W 160                 // Landscape width
#define SCREEN_H 80                  // Landscape height
#define TFT_BOOT_DURATION 2500       // ms to show boot logo

// SPIFFS Configuration
#define SPIFFS_FORMAT_IF_FAILED true

// Timing
#define STROBE_MIN_SPEED 1
#define STROBE_MAX_SPEED 10
#define SMOKE_DEFAULT_DURATION 3000   // milliseconds

#endif
