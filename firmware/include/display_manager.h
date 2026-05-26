// Display Manager - TFT ST7735S 160x80 color display (landscape)

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "config.h"

// ── Color palette (RGB565) ──────────────────────────────────────────
#define COL_BG        TFT_BLACK
#define COL_TEXT      TFT_WHITE
#define COL_DIM       0x7BEF    // Medium gray
#define COL_DARK      0x2104    // Very dark gray
#define COL_ACCENT    0x07FF    // Cyan
#define COL_HEADER_BG 0x0A4A    // Dark teal
#define COL_SCENE     0xFFE0    // Yellow
#define COL_SHOW      0xF81F    // Magenta
#define COL_OK        0x07E0    // Green
#define COL_WARN      0xFD20    // Orange
#define COL_ERR       0xF800    // Red
#define COL_BASS      0xF800    // Red
#define COL_MID       0x07E0    // Green
#define COL_HIGH      0x041F    // Blue-cyan
#define COL_WAVE      0x07FF    // Cyan

enum DisplayScreen {
  SCREEN_BOOT_LOGO,
  SCREEN_WIFI_CONNECTING,
  SCREEN_READY,
  SCREEN_STATUS
};

struct DisplayStatus {
  // WiFi
  String ipAddress;
  bool wifiConnected;
  bool apMode;
  // Fixtures
  int fixtureCount;
  int enabledFixtures;
  // Playback
  String activeSetupName;
  String activeScene;
  String activeShow;
  bool showRunning;
  // System
  uint8_t masterBrightness;
  bool strobeActive;
  bool smokeActive;
  bool dmxActive;
  uint32_t uptime;
  uint32_t freeMemory;
  uint8_t cpuLoad;
  uint8_t soundMode;    // 0=off, 1-4=active modes
  uint8_t soundVolume;  // 0-100
  uint8_t soundBass;    // 0-100
  uint8_t soundMid;     // 0-100
  uint8_t soundHigh;    // 0-100
  uint8_t soundPeak;    // 0-100
};

class DisplayManager {
public:
  DisplayManager();
  void begin();

  // Screen transitions
  void showBootLogo();
  void showWiFiConnecting(const String& ssid, int attempt, int maxAttempts);
  void showReady(const String& ip, bool apMode);
  void showStatus(const DisplayStatus& status);

  // Call in loop() for animations
  void update();

private:
  TFT_eSPI tft;
  TFT_eSprite spr;       // Full-screen sprite for flicker-free rendering
  DisplayScreen currentScreen;
  uint32_t lastUpdate;
  uint32_t screenStartTime;
  uint8_t animFrame;

  // Drawing helpers
  void drawHeader(const String& title);
  void drawWiFiIcon(int x, int y, bool connected);
  void drawDMXIcon(int x, int y, bool active);
  void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color = COL_OK);
  String formatUptime(uint32_t seconds);
  String truncate(const String& str, int maxLen);
};

#endif
