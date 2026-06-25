// ─────────────────────────────────────────────────────────
// Pédalier SUDSHOW — Configuration
// ESP8266 (Wemos D1 Mini) — 3 boutons via 2 pins
// ─────────────────────────────────────────────────────────

#ifndef CONFIG_H
#define CONFIG_H

// ── WiFi ────────────────────────────────────────────────
// Se connecte à l'Access Point créé par l'ESP32 SUDSHOW
#define WIFI_SSID       "SUDSHOW"
#define WIFI_PASSWORD   "12345678"

// ── ESP32 Server ────────────────────────────────────────
// IP de l'ESP32 en mode AP (par défaut 192.168.4.1)
#define ESP32_HOST      "192.168.4.1"
#define ESP32_PORT      80

// ── GPIO — Switches (2 pins → 3 boutons) ───────────────
// Logique combinatoire :
//   D3 seul LOW      → Bouton 1
//   D4 seul LOW      → Bouton 2
//   D3 + D4 tous LOW → Bouton 3
#define PIN_SWITCH1     D3    // GPIO0
#define PIN_SWITCH2     D4    // GPIO2 (aussi LED onboard)

// ── Anti-rebond ─────────────────────────────────────────
#define DEBOUNCE_MS     50    // Debounce en millisecondes

// ── Délai minimum entre 2 requêtes HTTP ─────────────────
#define HTTP_MIN_INTERVAL_MS  200

// ── Timeout connexion WiFi ──────────────────────────────
#define WIFI_TIMEOUT_MS 15000
#define WIFI_RETRY_MS   5000  // Délai entre tentatives de reconnexion

#endif
