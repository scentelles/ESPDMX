// Display Manager - OLED SSD1306 128x64 status display

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <SSD1306Wire.h>
#include "config.h"

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
  SSD1306Wire display;
  DisplayScreen currentScreen;
  uint32_t lastUpdate;
  uint32_t screenStartTime;
  uint8_t animFrame;

  // Drawing helpers
  void drawHeader(const String& title);
  void drawWiFiIcon(int x, int y, bool connected);
  void drawDMXIcon(int x, int y, bool active);
  void drawProgressBar(int x, int y, int w, int h, int percent);
  String formatUptime(uint32_t seconds);
  String truncate(const String& str, int maxLen);
};

#endif
