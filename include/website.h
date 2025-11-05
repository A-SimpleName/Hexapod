#include <WiFi.h>
#include "FS.h"
#include "SPIFFS.h"
#include <Arduino.h>
#include <WebServer.h>


#pragma once
#ifndef WEBSITE_H
#define WEBSITE_H

void setupWebServer();
void handleRoot();
void handleMove();

extern const char* ssid;
extern const char* password;
extern WebServer server;


#endif