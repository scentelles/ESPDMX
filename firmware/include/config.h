// Configuration file for ESP32 DMX Lighting Controller

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "ESP_EASY"
#define WIFI_PASSWORD "12345678"

// Web Server Configuration
#define WEB_PORT 80
#define MAX_JSON_SIZE 2048

// DMX Configuration
#define DMX_TX_PIN 17                // GPIO17 - TX for RS-485
#define DMX_RX_PIN 16                // GPIO16 - RX for RS-485
#define DMX_DE_PIN 15                // GPIO15 - DE (Driver Enable) for RS-485
#define DMX_BAUD_RATE 250000        // Standard DMX baud rate
#define DMX_CHANNELS 512             // Full DMX universe
#define DMX_BREAK_DURATION 90        // microseconds
#define DMX_MAB_DURATION 12          // microseconds (Mark After Break)

// System Configuration
#define ADMIN_PIN "1234"             // Default admin PIN (change in settings!)
#define UPDATE_INTERVAL 500          // milliseconds between state broadcasts
#define MAX_FIXTURES 64

// OLED Display Configuration (SSD1306 128x64)
#define OLED_SDA 5
#define OLED_SCL 4
#define OLED_ADDRESS 0x3C
#define OLED_BOOT_DURATION 2500       // ms to show boot logo

// SPIFFS Configuration
#define SPIFFS_FORMAT_IF_FAILED true

// Timing
#define STROBE_MIN_SPEED 1
#define STROBE_MAX_SPEED 10
#define SMOKE_DEFAULT_DURATION 3000   // milliseconds

#endif
