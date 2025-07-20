#pragma once

#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <MD5Builder.h>
#include <Huma_Buttons.h>
#include <NTPClient.h>
#include "webpages.h"

/* WiFi */
#define WIFI_SSID                 "blackdragon_2.4"
#define WIFI_PASS                 "07071992"
#define WIFI_AP_SSID              "ESP32 IP Camera"
#define WIFI_AP_PASS              ""
#define DOWNLOAD_READ_TIMEOUT     30000

/* IP CAMERA */
#define IPCAM_USERNAME            "admin"
#define IPCAM_PASSWORD            "Vietnam1989@"
#define IPCAM_SERVER              "http://192.168.0.103"
#define IPCAM_URI                 "/cgi-bin/snapshot.cgi"

/* Buttons */
#define BUTTON_1                  6
#define BUTTON_2                  5
#define BUTTON_CONFIG             15
#define BUTTON_CONFIG_HOLD_TIME   3000
#define SEND_LOG_DURATION         10000

/* Timezone */
#define TIMEZONE_OFFSET           25200 /* https://www.epochconverter.com/timezones */


/*
 * Database
 */
void DB_SetWifiCredentials(String &ssid, String &password);
void DB_GetWifiCredentials(String &ssid, String &password);
void DB_SetCameraIpAddress(String &address);
void DB_SetCameraAuth(String &username, String &password);
void DB_GetCameraAuth(String &username, String &password);
String DB_GetCameraIpAddress(void);
