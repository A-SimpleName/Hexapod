#include "hexapod_server.h"
#include "servocontrols.h"
#include "algorithm.h"
#include <ctype.h>


WiFiUDP udp;
constexpr int udpPort = 4210;
const char *ssid = "HTL-Perg Gast";
const char *password = "FIT2024!";

enum class ActiveCommand {
    None,
    WaveLegs,
    CircleJerk,
    Move
};

static ActiveCommand currentCommand = ActiveCommand::None;
static unsigned long lastWifiAttemptMs = 0;
static float currentMoveThetaDeg = 0.0f;
static long lastSessionId = -1;
static long lastProcessedSeq = -1;
static bool wifiEventHandlerRegistered = false;

static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        Serial.println("[WiFi] STA connected to AP");
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        Serial.print("[WiFi] STA disconnected, reason=");
        Serial.println((int)info.wifi_sta_disconnected.reason);
    }
}

static void startWifiConnect() {
    if (!wifiEventHandlerRegistered) {
        WiFi.onEvent(onWiFiEvent);
        wifiEventHandlerRegistered = true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setAutoConnect(true);
    WiFi.persistent(false);

    // Keep reconnect lightweight; avoid forced disconnect loops.
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
    Serial.print("  UDP commands: ");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(udpPort);
}

void connectWiFi() {
    startWifiConnect();

    Serial.print("Connecting to WiFi");
    const unsigned long started = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - started) < 25000UL) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" WiFi connected");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] RSSI: ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println(" WiFi timeout - continuing, will retry in background");
        Serial.print("[WiFi] status code: ");
        Serial.println((int)WiFi.status());
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
