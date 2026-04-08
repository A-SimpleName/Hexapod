#include "hexapod_server.h"
#include "servocontrols.h"
#include "algorithm.h"
#include <ctype.h>


WiFiUDP udp;
constexpr int udpPort = 4210;
const char *apSsid = "Hexapod-ESP32";
const char *apPassword = "hexapod123";

enum class ActiveCommand {
    None,
    WaveLegs,
    CircleJerk,
    Move
};

static ActiveCommand currentCommand = ActiveCommand::None;
static float currentMoveThetaDeg = 0.0f;
static long lastSessionId = -1;
static long lastProcessedSeq = -1;
static bool wifiEventHandlerRegistered = false;

static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
        Serial.print("[WiFi] Client connected to AP, AID=");
        Serial.println((int)info.wifi_ap_staconnected.aid);
    } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
        Serial.print("[WiFi] Client disconnected from AP, AID=");
        Serial.println((int)info.wifi_ap_stadisconnected.aid);
    }
}

static void startWifiAp() {
    if (!wifiEventHandlerRegistered) {
        WiFi.onEvent(onWiFiEvent);
        wifiEventHandlerRegistered = true;
    }

    WiFi.mode(WIFI_AP);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setSleep(false);
    WiFi.persistent(false);

    const IPAddress apIp(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIp, gateway, subnet);

    const bool apStarted = WiFi.softAP(apSsid, apPassword);
    if (!apStarted) {
        Serial.println("[WiFi] FEHLER: SoftAP konnte nicht gestartet werden");
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

// Parses numeric fields like "theta": 12.3 or "theta":"12.3".
bool extractFloatField(const String& payload, const char* fieldName, float& outValue) {
    String key = String("\"") + fieldName + "\"";
    int fieldPos = payload.indexOf(key);
    if (fieldPos < 0) {
        return false;
    }

    int colonPos = payload.indexOf(':', fieldPos);
    if (colonPos < 0) {
        return false;
    }

    int start = colonPos + 1;
    while (start < payload.length() && isspace((unsigned char)payload[start])) {
        start++;
    }
    if (start >= payload.length()) {
        return false;
    }

    bool quoted = payload[start] == '"';
    if (quoted) {
        start++;
    }

    int end = start;
    while (end < payload.length()) {
        char c = payload[end];
        if (quoted) {
            if (c == '"') {
                break;
            }
        } else {
            if (!(isdigit((unsigned char)c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')) {
                break;
            }
        }
        end++;
    }

    if (end <= start) {
        return false;
    }

    String value = payload.substring(start, end);
    outValue = value.toFloat();
    return true;
}

bool extractLongField(const String& payload, const char* fieldName, long& outValue) {
    String key = String("\"") + fieldName + "\"";
    int fieldPos = payload.indexOf(key);
    if (fieldPos < 0) {
        return false;
    }

    int colonPos = payload.indexOf(':', fieldPos);
    if (colonPos < 0) {
        return false;
    }

    int start = colonPos + 1;
    while (start < payload.length() && isspace((unsigned char)payload[start])) {
        start++;
    }
    if (start >= payload.length()) {
        return false;
    }

    bool quoted = payload[start] == '"';
    if (quoted) {
        start++;
    }

    int end = start;
    while (end < payload.length()) {
        char c = payload[end];
        if (quoted) {
            if (c == '"') {
                break;
            }
        } else {
            if (!(isdigit((unsigned char)c) || c == '-' || c == '+')) {
                break;
            }
        }
        end++;
    }

    if (end <= start) {
        return false;
    }

    String value = payload.substring(start, end);
    outValue = value.toInt();
    return true;
}

void setupWebServer() {
    udp.begin(udpPort);

    Serial.println("Network services started:");
    Serial.print("  AP SSID: ");
    Serial.println(apSsid);
    Serial.print("  UDP commands: ");
    Serial.print(WiFi.softAPIP());
    Serial.print(":");
    Serial.println(udpPort);
}

void connectWiFi() {
    startWifiAp();

    Serial.println("[WiFi] SoftAP gestartet");
    Serial.print("[WiFi] SSID: ");
    Serial.println(apSsid);
    Serial.print("[WiFi] AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void pollUdpCommands() {
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

    long session = -1;
    bool hasSession = extractLongField(payload, "session", session);
    if (hasSession && session != lastSessionId) {
        lastSessionId = session;
        lastProcessedSeq = -1;
        Serial.print("UDP session switched: ");
        Serial.println(lastSessionId);
    }

    long seq = -1;
    bool hasSeq = extractLongField(payload, "seq", seq);
    if (hasSeq) {
        if (seq <= lastProcessedSeq) {
            Serial.print("UDP stale packet ignored. seq=");
            Serial.print(seq);
            Serial.print(" last=");
            Serial.println(lastProcessedSeq);
            return;
        }
        lastProcessedSeq = seq;
    }

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
    } else if (commandId == "move") {
        float thetaDeg = 0.0f;
        if (extractFloatField(payload, "theta", thetaDeg)) {
            currentMoveThetaDeg = thetaDeg;
            Serial.print("UDP command: move, theta=");
            Serial.println(currentMoveThetaDeg);
        } else {
            Serial.print("UDP command: move without valid theta, using last theta=");
            Serial.println(currentMoveThetaDeg);
        }
        currentCommand = ActiveCommand::Move;
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
        case ActiveCommand::Move:
            walk(currentMoveThetaDeg);
            break;
        case ActiveCommand::None:
        default:
            delay(10);
            break;
    }
}
