#include <website.h>
#include "servocontrols.h"

WebServer server(80);
const char *ssid = "HTL-Perg Gast";
const char *password = "FIT2024!";
String lastKeys = "";

void handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "File not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

/* void handleKeys() {
    if (server.hasArg("pressed")) {
        lastKeys = server.arg("pressed");
        Serial.println("Pressed keys: " + lastKeys);
    }
    server.send(200, "text/plain", "OK");   
} */

void handleMove() {
    if (server.hasArg("pressed")) {
        lastKeys = server.arg("pressed");

        if (lastKeys.indexOf("w") != -1) {
            Serial.println("Moving forward");
            
        } if (lastKeys.indexOf("s") != -1) {
            Serial.println("Moving backward");
            
        } if (lastKeys.indexOf("a") != -1) {
            Serial.println("Rotating left");
            
        } if (lastKeys.indexOf("d") != -1) {
            Serial.println("Rotating right");

        }
    }

    server.send(200, "text/plain", "OK");
}

void setupWebServer() {
    server.serveStatic("/JS", SPIFFS, "/JS");
    server.serveStatic("/CSS", SPIFFS, "/CSS");
    // SPIFFS hier initialisieren
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // WiFi hier verbinden (optional)
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("WiFi connected");

    // Routen definieren
    server.on("/", handleRoot);
    server.on("/keys", handleMove);
    
    server.begin();
    
    Serial.println("HTTP server started on IP:");
    Serial.print(WiFi.localIP());
    Serial.println(":80");
}