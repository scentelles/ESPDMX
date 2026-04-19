// Display Manager - TFT ST7735S 160x80 color display (landscape)
#include "display_manager.h"

// ── Lightning bolt XBM icon 16x24 ───────────────────────────────────
static const uint8_t bolt_icon[] PROGMEM = {
  0x00, 0x0E, 0x00, 0x0F, 0x80, 0x07, 0xC0, 0x07,
  0xC0, 0x03, 0xE0, 0x03, 0xE0, 0x01, 0xF0, 0x01,
  0xF0, 0x00, 0xF8, 0x00, 0xFC, 0x3F, 0xFE, 0x3F,
  0x00, 0x1E, 0x00, 0x0F, 0x00, 0x0F, 0x80, 0x07,
  0x80, 0x07, 0xC0, 0x03, 0xC0, 0x03, 0xE0, 0x01,
  0xE0, 0x01, 0xF0, 0x00, 0xF0, 0x00, 0x78, 0x00,
};

DisplayManager::DisplayManager()
  : tft()
  , spr(&tft)
  , currentScreen(SCREEN_BOOT_LOGO)
  , lastUpdate(0)
  , screenStartTime(0)
  , animFrame(0) {
}

void DisplayManager::begin() {
  // Backlight ON
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  tft.init();
  tft.setRotation(1);            // Landscape 160x80, rotated 180°
  tft.fillScreen(COL_BG);

  // Create full-screen sprite for flicker-free drawing
  spr.setColorDepth(16);
  spr.createSprite(SCREEN_W, SCREEN_H);
}

// ── Boot Logo Screen ────────────────────────────────────────────────
void DisplayManager::showBootLogo() {
  currentScreen = SCREEN_BOOT_LOGO;
  screenStartTime = millis();

  spr.fillSprite(COL_BG);

  // Double frame
  spr.drawRect(0, 0, SCREEN_W, SCREEN_H, COL_ACCENT);
  spr.drawRect(1, 1, SCREEN_W - 2, SCREEN_H - 2, COL_DIM);

  // Lightning bolt
  spr.drawXBitmap(6, 28, bolt_icon, 16, 24, COL_ACCENT);

  // Title
  spr.setTextDatum(TL_DATUM);
  spr.setTextFont(4);
  spr.setTextColor(COL_ACCENT, COL_BG);
  spr.drawString("ESPDMX", 30, 8);

  // Subtitles
  spr.setTextFont(1);
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(COL_TEXT, COL_BG);
  spr.drawString("Eclairage DMX512", SCREEN_W / 2, 40);
  spr.setTextColor(COL_DIM, COL_BG);
  spr.drawString("Controleur Pro", SCREEN_W / 2, 52);

  // Version
  spr.setTextDatum(BR_DATUM);
  spr.drawString("v2.0", SCREEN_W - 4, SCREEN_H - 2);

  spr.pushSprite(0, 0);
}

// ── WiFi Connecting Screen ──────────────────────────────────────────
void DisplayManager::showWiFiConnecting(const String& ssid, int attempt, int maxAttempts) {
  currentScreen = SCREEN_WIFI_CONNECTING;

  spr.fillSprite(COL_BG);
  drawHeader("WiFi");

  spr.setTextFont(1);
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(COL_TEXT, COL_BG);
  spr.drawString(truncate(ssid, 22), SCREEN_W / 2, 20);

  // Animated dots
  String dots = "";
  for (int i = 0; i < (attempt % 4); i++) dots += ".";
  spr.setTextFont(2);
  spr.setTextColor(COL_ACCENT, COL_BG);
  spr.drawString("Connexion" + dots, SCREEN_W / 2, 34);

  // Progress bar
  int progress = (attempt * 100) / maxAttempts;
  drawProgressBar(20, 58, 120, 10, progress, COL_ACCENT);

  // Counter
  spr.setTextFont(1);
  spr.setTextColor(COL_DIM, COL_BG);
  spr.setTextDatum(TC_DATUM);
  spr.drawString(String(attempt) + "/" + String(maxAttempts), SCREEN_W / 2, 72);

  spr.pushSprite(0, 0);
}

// ── Ready Screen ────────────────────────────────────────────────────
void DisplayManager::showReady(const String& ip, bool apMode) {
  currentScreen = SCREEN_READY;
  screenStartTime = millis();

  spr.fillSprite(COL_BG);
  drawHeader("Pret");

  spr.setTextFont(1);
  spr.setTextDatum(TC_DATUM);

  if (apMode) {
    spr.setTextColor(COL_WARN, COL_BG);
    spr.drawString("Mode Point d'Acces", SCREEN_W / 2, 20);
  } else {
    spr.setTextColor(COL_OK, COL_BG);
    spr.drawString("WiFi Connecte", SCREEN_W / 2, 20);
  }

  // IP Address
  spr.setTextFont(2);
  spr.setTextColor(COL_TEXT, COL_BG);
  spr.drawString(ip, SCREEN_W / 2, 36);

  // Footer
  spr.setTextFont(1);
  spr.setTextColor(COL_DIM, COL_BG);
  spr.drawString("Interface Web disponible", SCREEN_W / 2, 62);

  spr.pushSprite(0, 0);
}

// ── Status Screen (main operating display) ──────────────────────────
void DisplayManager::showStatus(const DisplayStatus& status) {
  currentScreen = SCREEN_STATUS;

  spr.fillSprite(COL_BG);

  // ── Top bar: WiFi dot + IP + uptime ──
  drawWiFiIcon(1, 1, status.wifiConnected);
  spr.setTextFont(1);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(COL_TEXT, COL_BG);
  spr.drawString(status.ipAddress, 14, 1);
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(COL_DIM, COL_BG);
  spr.drawString(formatUptime(status.uptime), SCREEN_W - 2, 1);

  // Separator
  spr.drawFastHLine(0, 11, SCREEN_W, COL_DIM);

  if (status.soundMode > 0) {
    // ══════════════════════════════════════════════════════════════
    // AUDIO VISUALIZER MODE
    // ══════════════════════════════════════════════════════════════
    static uint8_t volHist[88] = {0};
    static uint8_t histIdx = 0;
    volHist[histIdx] = status.soundVolume;
    histIdx = (histIdx + 1) % 88;

    // Waveform box (left side)
    int waveW = 90, waveY = 13, waveH = SCREEN_H - waveY - 1;
    spr.drawRect(0, waveY, waveW, waveH, COL_DIM);
    for (int i = 0; i < waveW - 2; i++) {
      int pIdx = (histIdx + i) % 88;
      int nIdx = (histIdx + i + 1) % 88;
      int y1 = waveY + waveH - 1 - (volHist[pIdx] * (waveH - 2) / 100);
      int y2 = waveY + waveH - 1 - (volHist[nIdx] * (waveH - 2) / 100);
      uint16_t col = volHist[pIdx] > 80 ? COL_ERR : (volHist[pIdx] > 50 ? COL_WARN : COL_WAVE);
      spr.drawLine(i + 1, y1, i + 2, y2, col);
    }

    // VU Meters (right side)
    int barW = 14, barH = 48, barY = 14, gap = 4, startX = 96;
    auto drawVU = [&](int x, int pct, uint16_t col, const char* lbl) {
      spr.drawRect(x, barY, barW, barH, COL_DIM);
      int fh = (pct * (barH - 2)) / 100;
      if (fh > 0) spr.fillRect(x + 1, barY + barH - 1 - fh, barW - 2, fh, col);
      spr.setTextFont(1);
      spr.setTextDatum(TC_DATUM);
      spr.setTextColor(col, COL_BG);
      spr.drawString(lbl, x + barW / 2, barY + barH + 2);
    };
    drawVU(startX, status.soundBass, COL_BASS, "B");
    drawVU(startX + barW + gap, status.soundMid, COL_MID, "M");
    drawVU(startX + 2 * (barW + gap), status.soundHigh, COL_HIGH, "H");

    // Peak dot
    uint16_t pkCol = status.soundPeak > 85 ? COL_ERR : COL_DIM;
    spr.fillCircle(SCREEN_W - 5, 16, 3, pkCol);

  } else {
    // ══════════════════════════════════════════════════════════════
    // NORMAL STATUS MODE
    // ══════════════════════════════════════════════════════════════

    // ── Row 1: Scene / Show name (y=13..31) ──
    spr.setTextDatum(TL_DATUM);
    spr.setTextFont(1);

    if (status.showRunning && status.activeShow.length() > 0) {
      spr.setTextColor(COL_DIM, COL_BG);
      spr.drawString("Show:", 2, 14);
      spr.setTextFont(2);
      spr.setTextColor(COL_SHOW, COL_BG);
      spr.drawString(truncate(status.activeShow, 14), 34, 12);
      uint16_t dotCol = ((millis() / 500) % 2 == 0) ? COL_OK : COL_DARK;
      spr.fillCircle(SCREEN_W - 6, 20, 3, dotCol);
    } else if (status.activeScene.length() > 0) {
      spr.setTextColor(COL_DIM, COL_BG);
      spr.drawString("Scene:", 2, 14);
      spr.setTextFont(2);
      spr.setTextColor(COL_SCENE, COL_BG);
      spr.drawString(truncate(status.activeScene, 13), 42, 12);
    } else {
      spr.setTextColor(COL_DIM, COL_BG);
      spr.drawString("Aucune scene active", 2, 17);
    }

    spr.drawFastHLine(0, 32, SCREEN_W, COL_DARK);

    // ── Row 2: Fixtures + CPU + DMX (y=34..45) ──
    spr.setTextFont(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(COL_TEXT, COL_BG);
    spr.drawString("Fix:" + String(status.enabledFixtures) + "/" + String(status.fixtureCount), 2, 35);

    spr.setTextDatum(TC_DATUM);
    spr.drawString("CPU:" + String(status.cpuLoad) + "%", 80, 35);

    drawDMXIcon(136, 34, status.dmxActive);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(COL_OK, COL_BG);
    spr.drawString("D", 148, 35);

    spr.drawFastHLine(0, 46, SCREEN_W, COL_DARK);

    // ── Row 3: Brightness bar + effects (y=48..60) ──
    spr.setTextFont(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(COL_DIM, COL_BG);
    spr.drawString("Bri:", 2, 50);
    drawProgressBar(26, 50, 66, 8, status.masterBrightness, COL_OK);

    spr.setTextColor(COL_TEXT, COL_BG);
    spr.drawString(String(status.masterBrightness) + "%", 96, 50);

    if (status.strobeActive && (millis() / 100) % 2 == 0) {
      spr.setTextColor(COL_ERR, COL_BG);
      spr.drawString("STR", 122, 50);
    }
    if (status.smokeActive) {
      spr.setTextColor(COL_WARN, COL_BG);
      spr.drawString("SMK", 144, 50);
    }

    // ── Row 4: Memory (y=62..74) ──
    spr.setTextFont(1);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(COL_DIM, COL_BG);
    spr.drawString(String(status.freeMemory / 1024) + "KB free", 2, 64);
  }

  spr.pushSprite(0, 0);
}

// ── Animation update (call in loop) ─────────────────────────────────
void DisplayManager::update() {
  if (millis() - lastUpdate < 250) return;
  lastUpdate = millis();
  animFrame++;
}

// ── Drawing Helpers ─────────────────────────────────────────────────

void DisplayManager::drawHeader(const String& title) {
  spr.fillRect(0, 0, SCREEN_W, 14, COL_HEADER_BG);
  spr.setTextFont(1);
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(COL_ACCENT, COL_HEADER_BG);
  spr.drawString(title, SCREEN_W / 2, 3);
}

void DisplayManager::drawWiFiIcon(int x, int y, bool connected) {
  spr.fillCircle(x + 4, y + 4, 4, connected ? COL_OK : COL_ERR);
}

void DisplayManager::drawDMXIcon(int x, int y, bool active) {
  if (active) {
    spr.fillCircle(x + 4, y + 4, 4, COL_OK);
  } else {
    spr.drawCircle(x + 4, y + 4, 4, COL_ERR);
  }
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
  percent = constrain(percent, 0, 100);
  spr.drawRect(x, y, w, h, COL_DIM);
  int fillW = ((w - 2) * percent) / 100;
  if (fillW > 0) {
    spr.fillRect(x + 1, y + 1, fillW, h - 2, color);
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
