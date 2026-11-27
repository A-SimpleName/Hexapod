#include "hexapod_server.h"
#include "servocontrols.h"
#include "algorithm.h"
#include "movement.h"
#include <ctype.h>
#include <esp_wifi.h>
#include <math.h>
#include <string.h>


WiFiUDP udp;
constexpr int udpPort = 4210;
const char *apSsid = "Hexapod-ESP32";
const char *apPassword = "hexapod123";

enum class ActiveCommand {
    None,
    WaveLegs,
    CircleJerk,
    BobUpDown,
    Move,
    Bipod,
    BipodStopping
};

static ActiveCommand currentCommand = ActiveCommand::None;
static long currentBobPeriodMs = BOB_DEFAULT_PERIOD_MS;
static float currentMoveThetaDeg = 0.0f;
static unsigned long lastBipodCommandMs = 0;
constexpr unsigned long BIPOD_COMMAND_TIMEOUT_MS = 250;
static String pendingMovement;
static unsigned long pendingMovementReceivedAtMs = 0;
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

static bool startWifiAp() {
    if (!wifiEventHandlerRegistered) {
        WiFi.onEvent(onWiFiEvent);
        wifiEventHandlerRegistered = true;
    }

    // Set storage before initializing Wi-Fi so the AP uses this build's config.
    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_AP)) {
        Serial.println("[WiFi] FEHLER: AP-Modus konnte nicht aktiviert werden");
        return false;
    }
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setSleep(false);

    const IPAddress apIp(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);
    if (!WiFi.softAPConfig(apIp, gateway, subnet)) {
        Serial.println("[WiFi] FEHLER: AP-IP konnte nicht konfiguriert werden");
        return false;
    }

    // Arduino ESP32 2.x's softAP helper leaves some AP config fields unset.
    wifi_config_t config{};
    strlcpy(reinterpret_cast<char*>(config.ap.ssid), apSsid, sizeof(config.ap.ssid));
    strlcpy(reinterpret_cast<char*>(config.ap.password), apPassword, sizeof(config.ap.password));
    config.ap.ssid_len = strlen(apSsid);
    config.ap.channel = 1;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    config.ap.pairwise_cipher = WIFI_CIPHER_TYPE_CCMP;
    config.ap.max_connection = 4;
    config.ap.beacon_interval = 100;

    const esp_err_t result = esp_wifi_set_config(WIFI_IF_AP, &config);
    if (result != ESP_OK) {
        Serial.printf("[WiFi] FEHLER: AP-Konfiguration: %s\n", esp_err_to_name(result));
        return false;
    }
    return true;
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
static bool extractNumericField(const String& payload, const char* fieldName,
                                bool allowFloatSyntax, String& value) {
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
            const bool digitOrSign = isdigit((unsigned char)c) || c == '-' || c == '+';
            const bool fractionOrExponent = allowFloatSyntax && (c == '.' || c == 'e' || c == 'E');
            if (!digitOrSign && !fractionOrExponent) {
                break;
            }
        }
        end++;
    }

    if (end <= start) {
        return false;
    }

    value = payload.substring(start, end);
    return true;
}

bool extractFloatField(const String& payload, const char* fieldName, float& outValue) {
    String value;
    if (!extractNumericField(payload, fieldName, true, value)) {
        return false;
    }
    outValue = value.toFloat();
    return true;
}

bool extractLongField(const String& payload, const char* fieldName, long& outValue) {
    String value;
    if (!extractNumericField(payload, fieldName, false, value)) {
        return false;
    }
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
    if (!startWifiAp()) {
        return;
    }

    Serial.println("[WiFi] SoftAP gestartet");
    Serial.print("[WiFi] SSID: ");
    Serial.println(apSsid);
    Serial.print("[WiFi] AP IP: ");
    Serial.println(WiFi.softAPIP());
}

static void stopActiveCommand() {
    if (currentCommand == ActiveCommand::Bipod) {
        startBipodStop();
        currentCommand = ActiveCommand::BipodStopping;
    } else if (currentCommand != ActiveCommand::BipodStopping) {
        currentCommand = ActiveCommand::None;
    }
}

static bool isWalkCommand(const String& commandId) {
    return commandId == "move" || commandId == "move_bipod";
}

static void applyCommand(const String& payload, const String& commandId) {
    if (commandId == "wave_legs") {
        currentCommand = ActiveCommand::WaveLegs;
        Serial.println("UDP command: wave_legs");
    } else if (commandId == "circle_jerk") {
        currentCommand = ActiveCommand::CircleJerk;
        Serial.println("UDP command: circle_jerk");
    } else if (commandId == "bob_up_down") {
        if (currentCommand != ActiveCommand::BobUpDown) {
            startBobUpDown();
            currentBobPeriodMs = BOB_DEFAULT_PERIOD_MS;
        }
        long periodMs;
        if (extractLongField(payload, "period_ms", periodMs) &&
            periodMs >= BOB_MIN_PERIOD_MS && periodMs <= BOB_MAX_PERIOD_MS) {
            currentBobPeriodMs = periodMs;
        }
        currentCommand = ActiveCommand::BobUpDown;
        Serial.printf("UDP command: bob_up_down, period=%ld ms\n", currentBobPeriodMs);
    } else if (commandId == "stop") {
        pendingMovement = "";
        stopActiveCommand();
        Serial.println("UDP command: stop");
    } else if (commandId == "recover_pwm") {
        pendingMovement = "";
        currentCommand = ActiveCommand::None;
        recoverPwmDrivers();
        Serial.println("UDP command: recover_pwm");
    } else if (isWalkCommand(commandId)) {
        const bool bipod = commandId == "move_bipod";
        const ActiveCommand requested = bipod ? ActiveCommand::Bipod : ActiveCommand::Move;
        float thetaDeg = 0.0f;
        if (extractFloatField(payload, "theta", thetaDeg) && isfinite(thetaDeg)) {
            currentMoveThetaDeg = fmodf(thetaDeg, 360.0f);
        }
        if (currentCommand != requested) {
            long stepMs = WALK_DEFAULT_STEP_MS;
            long requestedStepMs;
            if (extractLongField(payload, "step_ms", requestedStepMs) &&
                requestedStepMs >= WALK_MIN_STEP_MS && requestedStepMs <= WALK_MAX_STEP_MS) {
                stepMs = requestedStepMs;
            }
            long liftMm = 0;
            extractLongField(payload, "lift_mm", liftMm);
            // Apply timing and lift once per walk, before any leg enters swing.
            startWalking(stepMs, bipod ? WalkGait::Bipod : WalkGait::Tripod, liftMm);
            Serial.printf("UDP command: %s, theta=%.1f, step=%ld ms\n",
                          commandId.c_str(), currentMoveThetaDeg, stepMs);
        }
        if (bipod) {
            lastBipodCommandMs = millis();
        }
        currentCommand = requested;
    } else {
        Serial.println("UDP command unknown: " + payload);
    }
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

    const String payload = String(buffer);
    const String commandId = extractIdField(payload);

    long session = -1;
    bool hasSession = extractLongField(payload, "session", session);
    if (hasSession && session != lastSessionId) {
        lastSessionId = session;
        lastProcessedSeq = -1;
        pendingMovement = "";
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

    const bool commandsMovement = isWalkCommand(commandId) || commandId == "bob_up_down" ||
        commandId == "wave_legs" || commandId == "circle_jerk";
    if (commandsMovement) {
        if (currentCommand == ActiveCommand::Bipod && commandId != "move_bipod") {
            stopActiveCommand();
        }
        if (currentCommand == ActiveCommand::BipodStopping) {
            pendingMovement = payload;
            pendingMovementReceivedAtMs = millis();
            return;
        }
    }
    applyCommand(payload, commandId);
}

void runActiveCommand() {
    switch (currentCommand) {
        case ActiveCommand::WaveLegs:
            waveLegs();
            break;
        case ActiveCommand::CircleJerk:
            circleJerk();
            break;
        case ActiveCommand::BobUpDown:
            bobUpDown(currentBobPeriodMs);
            break;
        case ActiveCommand::Move:
            walk(currentMoveThetaDeg);
            break;
        case ActiveCommand::Bipod:
            if (millis() - lastBipodCommandMs >= BIPOD_COMMAND_TIMEOUT_MS) {
                stopActiveCommand();
            } else {
                walk(currentMoveThetaDeg);
            }
            break;
        case ActiveCommand::BipodStopping:
            if (walk(currentMoveThetaDeg)) {
                currentCommand = ActiveCommand::None;
                if (pendingMovement.length() > 0) {
                    const String commandId = extractIdField(pendingMovement);
                    // A released or disconnected joystick must not restart walking.
                    if (!isWalkCommand(commandId) ||
                        millis() - pendingMovementReceivedAtMs < BIPOD_COMMAND_TIMEOUT_MS) {
                        applyCommand(pendingMovement, commandId);
                    }
                    pendingMovement = "";
                }
            }
            break;
        case ActiveCommand::None:
        default:
            delay(10);
            break;
    }
}
