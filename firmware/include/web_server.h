// Web Server - Handles HTTP and WebSocket connections

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"

class WebServer {
public:
  WebServer();
  void begin();
  void handleClient();
  void broadcastState();
  
  // API Routes
  void setupRoutes(AsyncWebServer* server);
  
  // State management
  const char* getStateJSON();
  
private:
  void sendJSON(AsyncWebServerRequest* request, const JsonObject& obj);
  void sendError(AsyncWebServerRequest* request, const String& error);
  
  StaticJsonDocument<2048> stateDoc;
  uint32_t lastBroadcast;
};

#endif
