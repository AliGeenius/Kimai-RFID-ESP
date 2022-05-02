#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Ticker.h>
#include <WiFiManager.h>
//#include <brzo_i2c.h>
//#include <SSD1306Brzo.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <OLEDDisplayUi.h>

#define SERIAL_DEBUG true
#define SERIAL_RESPONSE true

#define OLED_DISP true
// WiFi status LED
#define STAT_LED 13

// Everything we need for the display
// Display
#if OLED_DISP
void displayInit();
void uiInit();
void printTopline(OLEDDisplay *display, OLEDDisplayUiState *state);
void printResponse(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void printError(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void printClock(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);

// Logo
const uint8_t Logo_bits[] PROGMEM = {
    0xFE,
    0x01,
    0xFF,
    0x03,
    0x33,
    0x03,
    0xB6,
    0x01,
    0xFC,
    0x00,
    0x78,
    0x00,
    0x30,
    0x00,
    0x30,
    0x00,
    0x30,
    0x00,
    0x30,
    0x00,
};
// update Signal
void sigRefresh();

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
SSD1306Wire display(0x3c, 4, 5);
OLEDDisplayUi ui(&display);
Ticker wifiTicker(sigRefresh, 5000, 0, MILLIS);
uint8_t sigStrength = 0;

OverlayCallback overlays[] = {printTopline};
uint8_t overlaysCount = 1;

FrameCallback frames[] = {printClock, printResponse, printError};
uint8_t frameCount = 3;

// Clock
int clockCenterX = 128 / 2;
int clockCenterY = ((64 - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 23;

// NTP
WiFiUDP ntpUDP;
// edit it to your needs. The 7200 is for + 2h
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);
#endif

// RFID Reader
#define SS_PIN 15
#define RST_PIN 0

MFRC522 mfrc522(SS_PIN, RST_PIN);
byte id[7];

// Wifi man. globals
void wifiManInit();
void saveConfigCallback();
char kimai_server[40];
char kimai_port[6] = "80";
char api_user[34] = "";
char api_token[34] = "";
// flag for saving data
bool shouldSaveConfig = false;

// HTTP globals
String uri = "/api/remotes/shorttoggle?remote=";
String d_user;
String d_duration;
uint16_t port = 80;

DynamicJsonDocument doc(512);

// Status
void tick();
Ticker ticker(tick, 600, 0, MILLIS);
unsigned long timer = 0; // used for blocking the reader
