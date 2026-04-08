#pragma once

#ifndef HEXAPOD_SERVER_H
#define HEXAPOD_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

void setupWebServer();
void connectWiFi();
void pollUdpCommands();
void runActiveCommand();

extern const char* apSsid;
extern const char* apPassword;

#endif
