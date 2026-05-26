#include <iostream>
#include "ArduinoJson.h"

int main() {
    DynamicJsonDocument doc(65536);
    doc["activeSetupId"] = "setup-123456";
    doc["activeScene"] = "scene-123";
    doc["activeAmbiance"] = "ambiance-123";
    doc["activeShow"] = "show-123";
    doc["strobeActive"] = true;
    doc["smokeActive"] = false;
    doc["masterBrightness"] = 255;
    doc["soundMode"] = 1;
    doc["soundSensitivity"] = 5;
    
    JsonObject audio = doc.createNestedObject("audio");
    audio["volume"] = 100;
    audio["bass"] = 100;
    audio["mid"] = 100;
    audio["high"] = 100;
    audio["beat"] = true;
    
    JsonArray dmxOutput = doc.createNestedArray("dmxOutput");
    for (int i = 0; i < 512; i++) {
        dmxOutput.add(255);
    }
    
    std::cout << "Memory usage: " << doc.memoryUsage() << " bytes" << std::endl;
    
    std::string response;
    serializeJson(doc, response);
    std::cout << "Serialized length: " << response.length() << " bytes" << std::endl;
    
    return 0;
}
