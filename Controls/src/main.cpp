#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_PWMServoDriver.h>
#include <esp_system.h>
#include "servocontrols.h"
#include "algorithm.h"
#include "hexapod_server.h"

static const char* resetReasonToString(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXTERNAL_PIN";
        case ESP_RST_SW:        return "SOFTWARE";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "OTHER_WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "UNKNOWN";
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);

    // Lower CPU clock to reduce transient current draw on weak power rails.
    setCpuFrequencyMhz(80);

    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[BOOT] Reset reason: %s (%d)\n", resetReasonToString(reason), (int)reason);
    Serial.printf("[BOOT] CPU freq: %u MHz\n", getCpuFrequencyMhz());

    // Join WiFi before powering servo drivers to avoid RF/auth instability on weak rails.
    connectWiFi();

    setupPWM(pwm);
    delay(500);
    initJointStates();

    setupWebServer();
}

void loop() {
    pollUdpCommands();
    runActiveCommand();
}


