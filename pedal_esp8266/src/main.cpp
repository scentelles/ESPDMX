// ─────────────────────────────────────────────────────────
// Pédalier SUDSHOW — Firmware ESP8266
// 3 boutons via 2 pins (logique combinatoire)
// Envoie des événements pressed/released génériques
// à l'ESP32 SUDSHOW via HTTP POST
//
// Logique pins → boutons :
//   D3 seul LOW      → Bouton 1
//   D4 seul LOW      → Bouton 2
//   D3 + D4 tous LOW → Bouton 3
// ─────────────────────────────────────────────────────────

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "config.h"

// ── État des switches ───────────────────────────────────
// Quel bouton est actuellement actif (0 = aucun, 1-3)
int activeButton = 0;
int lastActiveButton = 0;

// ── Debounce ────────────────────────────────────────────
int candidateButton = 0;
uint32_t candidateTime = 0;
bool debounceStable = true;

// ── Timing HTTP ─────────────────────────────────────────
uint32_t lastHttpSend = 0;

// ── WiFi ────────────────────────────────────────────────
uint32_t lastWifiAttempt = 0;
bool wifiConnected = false;

// ── Prototypes ──────────────────────────────────────────
void connectWiFi();
int readButtons();
bool sendPedalEvent(int button, const char* state);
void blinkLED();

// ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n╔═══════════════════════════════════════╗");
  Serial.println("║   SUDSHOW Pédalier — 3 Boutons        ║");
  Serial.println("╚═══════════════════════════════════════╝");

  // LED onboard pour feedback (D4 = GPIO2, inversée)
  pinMode(PIN_SWITCH2, OUTPUT);
  digitalWrite(PIN_SWITCH2, HIGH); // LED off

  // Connexion WiFi (avec feedback LED)
  connectWiFi();

  // Configurer les pins en entrée APRÈS le WiFi (D4 sert de LED pendant le boot)
  pinMode(PIN_SWITCH1, INPUT);
  pinMode(PIN_SWITCH2, INPUT);
  // Note: D3 (GPIO0) et D4 (GPIO2) ont des pullups internes sur l'ESP8266
  // Les foot-switches tirent vers GND → lecture LOW = appuyé
}

// ─────────────────────────────────────────────────────────
void loop() {
  // ── Vérifier connexion WiFi ───────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    
    if (millis() - lastWifiAttempt > WIFI_RETRY_MS) {
      Serial.println("WiFi déconnecté, reconnexion...");
      // Repasser D4 en output pour le feedback LED
      pinMode(PIN_SWITCH2, OUTPUT);
      connectWiFi();
      // Repasser en input
      pinMode(PIN_SWITCH1, INPUT);
      pinMode(PIN_SWITCH2, INPUT);
    }
    delay(100);
    return;
  }
  
  if (!wifiConnected) {
    wifiConnected = true;
    Serial.println("WiFi connecté ! IP: " + WiFi.localIP().toString());
  }

  // ── Lire l'état des boutons ───────────────────────────
  int currentButton = readButtons();

  // ── Debounce ──────────────────────────────────────────
  if (currentButton != candidateButton) {
    candidateButton = currentButton;
    candidateTime = millis();
    debounceStable = false;
  }
  
  if (!debounceStable && (millis() - candidateTime >= DEBOUNCE_MS)) {
    debounceStable = true;
    activeButton = candidateButton;
  }

  // ── Détecter les changements et envoyer ───────────────
  if (activeButton != lastActiveButton) {
    uint32_t now = millis();
    
    // Rate-limit HTTP
    if (now - lastHttpSend >= HTTP_MIN_INTERVAL_MS) {
      // Si un bouton était actif avant → envoyer released
      if (lastActiveButton > 0) {
        Serial.printf("Bouton %d RELEASED\n", lastActiveButton);
        sendPedalEvent(lastActiveButton, "released");
        lastHttpSend = millis();
      }
      
      // Si un nouveau bouton est actif → envoyer pressed
      if (activeButton > 0) {
        Serial.printf("Bouton %d PRESSED\n", activeButton);
        sendPedalEvent(activeButton, "pressed");
        lastHttpSend = millis();
      }
      
      lastActiveButton = activeButton;
    }
  }

  // ── Heartbeat Ping ──────────────────────────────────────
  static uint32_t lastPingTime = 0;
  if (millis() - lastPingTime >= 5000) {
    if (wifiConnected && millis() - lastHttpSend >= 1000) {
      WiFiClient client;
      HTTPClient http;
      String url = "http://" + String(ESP32_HOST) + ":" + String(ESP32_PORT) + "/api/control/pedal";
      http.begin(client, url);
      http.addHeader("Content-Type", "application/json");
      http.setTimeout(1000);
      int code = http.POST("{\"action\":\"ping\"}");
      http.end();
      lastPingTime = millis();
    }
  }

  delay(5);
}

// ─────────────────────────────────────────────────────────
// Lecture combinatoire des 2 pins → 3 boutons
// Exactement la logique de l'ancien sketch OSC
// ─────────────────────────────────────────────────────────
int readButtons() {
  int pin1 = digitalRead(PIN_SWITCH1); // D3
  int pin2 = digitalRead(PIN_SWITCH2); // D4

  if (pin1 == LOW && pin2 == LOW) {
    return 3; // Les deux appuyés → Bouton 3
  } else if (pin1 == LOW) {
    return 1; // D3 seul → Bouton 1
  } else if (pin2 == LOW) {
    return 2; // D4 seul → Bouton 2
  }
  return 0; // Rien
}

// ─────────────────────────────────────────────────────────
// Connexion WiFi
// ─────────────────────────────────────────────────────────
void connectWiFi() {
  lastWifiAttempt = millis();
  
  Serial.print("Connexion à ");
  Serial.print(WIFI_SSID);
  Serial.print("...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT_MS) {
    delay(100);
    digitalWrite(PIN_SWITCH2, HIGH); // LED off
    delay(100);
    digitalWrite(PIN_SWITCH2, LOW);  // LED on
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(PIN_SWITCH2, LOW); // LED on = connecté
    Serial.println("\n✓ Connecté !");
    Serial.println("  IP: " + WiFi.localIP().toString());
    Serial.println("  ESP32: http://" + String(ESP32_HOST) + ":" + String(ESP32_PORT));
  } else {
    wifiConnected = false;
    digitalWrite(PIN_SWITCH2, HIGH); // LED off
    Serial.println("\n✗ Échec connexion WiFi");
  }
}

// ─────────────────────────────────────────────────────────
// Envoi événement pédalier vers l'ESP32
// POST /api/control/pedal {"button": N, "state": "pressed"|"released"}
// ─────────────────────────────────────────────────────────
bool sendPedalEvent(int button, const char* state) {
  if (!wifiConnected) return false;

  WiFiClient client;
  HTTPClient http;
  
  String url = "http://" + String(ESP32_HOST) + ":" + String(ESP32_PORT) + "/api/control/pedal";
  String body = "{\"button\":" + String(button) + ",\"state\":\"" + String(state) + "\"}";
  
  Serial.print("HTTP POST → /api/control/pedal ");
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2000);
  
  int httpCode = http.POST(body);
  
  if (httpCode > 0) {
    Serial.println("→ " + String(httpCode));
    http.end();
    return (httpCode == 200);
  } else {
    Serial.println("→ ERREUR: " + http.errorToString(httpCode));
    http.end();
    return false;
  }
}
