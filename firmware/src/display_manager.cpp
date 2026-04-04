#include "display_manager.h"

// ── Lightning bolt XBM icon 16x24 ───────────────────────────────────
static const uint8_t bolt_icon[] PROGMEM = {
  0x00, 0x0E, // ........  .###....
  0x00, 0x0F, // ........  ####....
  0x80, 0x07, // .......#  .###....
  0xC0, 0x07, // ......##  .###....
  0xC0, 0x03, // ......##  ..##....
  0xE0, 0x03, // .....###  ..##....
  0xE0, 0x01, // .....###  ...#....
  0xF0, 0x01, // ....####  ...#....
  0xF0, 0x00, // ....####  ........
  0xF8, 0x00, // ...#####  ........
  0xFC, 0x3F, // ..######  ######..
  0xFE, 0x3F, // .#######  ######..
  0x00, 0x1E, // ........  .####...
  0x00, 0x0F, // ........  ####....
  0x00, 0x0F, // ........  ####....
  0x80, 0x07, // .......#  .###....
  0x80, 0x07, // .......#  .###....
  0xC0, 0x03, // ......##  ..##....
  0xC0, 0x03, // ......##  ..##....
  0xE0, 0x01, // .....###  ...#....
  0xE0, 0x01, // .....###  ...#....
  0xF0, 0x00, // ....####  ........
  0xF0, 0x00, // ....####  ........
  0x78, 0x00, // .####...  ........
};

// ── WiFi icon 12x9 ─────────────────────────────────────────────────
static const uint8_t wifi_icon[] PROGMEM = {
  0xF8, 0x01, // .#####...  .......
  0xFE, 0x07, // .#######  .###....
  0x07, 0x0E, // ###.....  .###....
  0xF1, 0x08, // #...####  ...#....
  0xFC, 0x03, // ..######  ........
  0x0C, 0x03, // ..##....  ##......
  0x60, 0x00, // .....##.  ........
  0xF0, 0x00, // ....####  ........
  0x60, 0x00, // .....##.  ........
};

DisplayManager::DisplayManager()
  : display(OLED_ADDRESS, OLED_SDA, OLED_SCL)
  , currentScreen(SCREEN_BOOT_LOGO)
  , lastUpdate(0)
  , screenStartTime(0)
  , animFrame(0) {
}

void DisplayManager::begin() {
  display.init();
  display.flipScreenVertically();  // LOLIN32 OLED is mounted upside down
  display.setContrast(255);
  display.clear();
  display.display();
}

// ── Boot Logo Screen ────────────────────────────────────────────────
void DisplayManager::showBootLogo() {
  currentScreen = SCREEN_BOOT_LOGO;
  screenStartTime = millis();

  display.clear();

  // Outer frame
  display.drawRect(0, 0, 128, 64);
  display.drawRect(1, 1, 126, 62);

  // Lightning bolt icon centered left area
  display.drawXbm(8, 20, 16, 24, bolt_icon);

  // Title
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(30, 4, "DMXESP");

  // Subtitle
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(80, 32, "DMX512 Lighting");
  display.drawString(80, 44, "Controller");

  display.display();
}

// ── WiFi Connecting Screen ──────────────────────────────────────────
void DisplayManager::showWiFiConnecting(const String& ssid, int attempt, int maxAttempts) {
  currentScreen = SCREEN_WIFI_CONNECTING;

  display.clear();
  drawHeader("WiFi");

  // SSID
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 18, truncate(ssid, 20));

  // Animated dots
  String dots = "";
  int dotCount = (attempt % 4);
  for (int i = 0; i < dotCount; i++) dots += ".";
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 30, "Connecting" + dots);

  // Progress bar
  int progress = (attempt * 100) / maxAttempts;
  drawProgressBar(14, 52, 100, 8, progress);

  display.display();
}

// ── Ready Screen ────────────────────────────────────────────────────
void DisplayManager::showReady(const String& ip, bool apMode) {
  currentScreen = SCREEN_READY;
  screenStartTime = millis();

  display.clear();
  drawHeader("Ready");

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);

  // Mode
  display.drawString(64, 18, apMode ? "Access Point Mode" : "WiFi Connected");

  // IP Address (larger)
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 32, ip);

  // Footer
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 52, "Web UI available");

  display.display();
}

// ── Status Screen (main operating display) ──────────────────────────
void DisplayManager::showStatus(const DisplayStatus& status) {
  currentScreen = SCREEN_STATUS;

  display.clear();

  // ── Top bar: WiFi + IP + uptime ──
  drawWiFiIcon(0, 0, status.wifiConnected);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(14, 0, status.ipAddress);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 0, formatUptime(status.uptime));

  // Separator line
  display.drawHorizontalLine(0, 12, 128);

  // ── Row 1: Active scene or show ──
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  if (status.showRunning && status.activeShow.length() > 0) {
    display.drawString(0, 15, "Show:");
    display.setFont(ArialMT_Plain_16);
    display.drawString(32, 13, truncate(status.activeShow, 12));
    // Running indicator
    if ((millis() / 500) % 2 == 0) {
      display.fillCircle(123, 20, 3);
    } else {
      display.drawCircle(123, 20, 3);
    }
  } else if (status.activeScene.length() > 0) {
    display.drawString(0, 15, "Scene:");
    display.setFont(ArialMT_Plain_16);
    display.drawString(38, 13, truncate(status.activeScene, 11));
  } else {
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 15, "No scene active");
  }

  // Separator
  display.drawHorizontalLine(0, 31, 128);

  // ── Row 2: Fixtures info ──
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 34, "Fixtures: " + String(status.enabledFixtures) + "/" + String(status.fixtureCount));

  // DMX indicator
  drawDMXIcon(90, 34, status.dmxActive);
  display.drawString(103, 34, "DMX");

  // ── Row 3: Master brightness bar + effects ──
  display.drawString(0, 48, "Bri:");
  drawProgressBar(24, 50, 60, 8, status.masterBrightness);

  // Effect indicators
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  if (status.strobeActive) {
    if ((millis() / 100) % 2 == 0) {
      display.drawString(112, 48, "STR");
    }
  }
  if (status.smokeActive) {
    display.drawString(128, 48, "SMK");
  }

  display.display();
}

// ── Animation update (call in loop) ─────────────────────────────────
void DisplayManager::update() {
  // Only update animations every 250ms
  if (millis() - lastUpdate < 250) return;
  lastUpdate = millis();
  animFrame++;
}

// ── Drawing Helpers ─────────────────────────────────────────────────

void DisplayManager::drawHeader(const String& title) {
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);

  // Checkmark-style header bar
  display.fillRect(0, 0, 128, 14);
  display.setColor(BLACK);
  display.drawString(64, 1, title);
  display.setColor(WHITE);
}

void DisplayManager::drawWiFiIcon(int x, int y, bool connected) {
  if (connected) {
    // Simple WiFi arcs
    display.drawCircleQuads(x + 5, y + 9, 8, 0b0001);
    display.drawCircleQuads(x + 5, y + 9, 5, 0b0001);
    display.fillCircle(x + 5, y + 9, 2);
  } else {
    // X mark for disconnected
    display.drawLine(x, y + 2, x + 10, y + 9);
    display.drawLine(x + 10, y + 2, x, y + 9);
  }
}

void DisplayManager::drawDMXIcon(int x, int y, bool active) {
  if (active) {
    display.fillCircle(x + 4, y + 5, 4);
  } else {
    display.drawCircle(x + 4, y + 5, 4);
  }
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, int percent) {
  percent = constrain(percent, 0, 100);
  display.drawRect(x, y, w, h);
  int fillW = ((w - 2) * percent) / 100;
  if (fillW > 0) {
    display.fillRect(x + 1, y + 1, fillW, h - 2);
  }
}

String DisplayManager::formatUptime(uint32_t seconds) {
  if (seconds < 60) return String(seconds) + "s";
  if (seconds < 3600) return String(seconds / 60) + "m";
  return String(seconds / 3600) + "h" + String((seconds % 3600) / 60) + "m";
}

String DisplayManager::truncate(const String& str, int maxLen) {
  if ((int)str.length() <= maxLen) return str;
  return str.substring(0, maxLen - 1) + "~";
}
