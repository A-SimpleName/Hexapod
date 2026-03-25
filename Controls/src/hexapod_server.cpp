#include "hexapod_server.h"
#include "servocontrols.h"
#include "algorithm.h"


WiFiUDP udp;
constexpr int udpPort = 4210;
const char *ssid = "HTL-Perg Gast";
const char *password = "FIT2024!";

enum class ActiveCommand {
    None,
    WaveLegs,
    CircleJerk
};

static ActiveCommand currentCommand = ActiveCommand::None;
static unsigned long lastWifiAttemptMs = 0;

static void startWifiConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(true);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.disconnect(true, true);
    delay(100);
    WiFi.begin(ssid, password);
    lastWifiAttemptMs = millis();
}

static void maintainWiFiConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    const unsigned long now = millis();
    if (now - lastWifiAttemptMs >= 10000UL) {
        Serial.println("[WiFi] reconnect attempt");
        startWifiConnect();
    }
}

// Parses {"id":"..."} with a lightweight string approach to avoid extra deps.
String extractIdField(const String& payload) {
    int idPos = payload.indexOf("\"id\"");
    if (idPos < 0) {
        return "";
    }

    int colonPos = payload.indexOf(':', idPos);
    if (colonPos < 0) {
        return "";
    }

    int firstQuote = payload.indexOf('"', colonPos + 1);
    if (firstQuote < 0) {
        return "";
    }

    int secondQuote = payload.indexOf('"', firstQuote + 1);
    if (secondQuote < 0) {
        return "";
    }

    return payload.substring(firstQuote + 1, secondQuote);
}

void setupWebServer() {
    udp.begin(udpPort);

    Serial.println("Network services started:");
    Serial.print("  UDP commands: ");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(udpPort);
}

void connectWiFi() {
    startWifiConnect();

    Serial.print("Connecting to WiFi");
    const unsigned long started = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - started) < 15000UL) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" WiFi connected");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(" WiFi timeout - continuing, will retry in background");
    }
}

void pollUdpCommands() {
    maintainWiFiConnection();
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    int packetSize = udp.parsePacket();
    if (packetSize <= 0) {
        return;
    }

    char buffer[256];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        return;
    }
    buffer[len] = '\0';

    String payload = String(buffer);
    String commandId = extractIdField(payload);

    if (commandId == "wave_legs") {
        currentCommand = ActiveCommand::WaveLegs;
        Serial.println("UDP command: wave_legs");
    } else if (commandId == "circle_jerk") {
        currentCommand = ActiveCommand::CircleJerk;
        Serial.println("UDP command: circle_jerk");
    } else if (commandId == "stop") {
        currentCommand = ActiveCommand::None;
        Serial.println("UDP command: stop");
    } else if (commandId == "recover_pwm") {
        currentCommand = ActiveCommand::None;
        recoverPwmDrivers();
        Serial.println("UDP command: recover_pwm");
    } else {
        Serial.println("UDP command unknown: " + payload);
    }
}

void runActiveCommand() {
    switch (currentCommand) {
        case ActiveCommand::WaveLegs:
            waveLegs();
            break;
        case ActiveCommand::CircleJerk:
            circleJerk();
            break;
        case ActiveCommand::None:
        default:
            delay(10);
            break;
    }
}
