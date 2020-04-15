#include "main.h"

//#################
//--- main
//#################

void setup()
{
  pinMode(STAT_LED, OUTPUT);
  ticker.attach(0.6, tick);

#if OLED_DISP
  displayInit();
#endif

#if SERIAL_DEBUG || SERIAL_RESPONSE
  Serial.begin(115200);
#endif

  // CONFIG and WiFi manager
  wifiManInit();
  delay(1000);

// init ui stuff
#if OLED_DISP
  uiInit();
#endif

  // init RFID reader
  SPI.begin();
  mfrc522.PCD_Init();
}

//#################
//--- main
//#################

// update UI -> read card -> create & send GET -> deserialize -> handle Response

void loop()
{
  yield(); // give background processes time to work (eg WiFi stack)
#if OLED_DISP
  int remainingTimeBudget = ui.update(); // refresh display frame
  if (remainingTimeBudget > 0)
  {
    if (millis() > timer + 6000)
    {
      if (ui.getUiState()->currentFrame != 0) // go back to clock frame after 5 seconds
      {
        ui.switchToFrame(0);
      }
    }
    else
    {
      return; // block reading for 6 seconds
    }
  }
  else
  {
    return;
  }
#else
  if (millis() < timer + 4000)
  {
    return; // block reading for 4 seconds
  }
#endif
  // Stop the blinking status light -> we just go here after 4-6 when timer < millis()
  if (ticker.active())
  {
    ticker.detach();
    SPI.begin();
    mfrc522.PCD_Init();
    //mfrc522.PCD_AntennaOn();
  }

  timeClient.update();

  // Check for card
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  // Read the card
  String remoteID;
  for (uint8_t i = 0; i < mfrc522.uid.size; i++)
  {
    id[i] = mfrc522.uid.uidByte[i];
    remoteID += String(id[i], HEX);
  }
  // I guess the reader antenna is interfering the wifi signal so we disable it until we get a response
  // Then whe reuse the Output for the LED (pin 13 is LED and MOSI of the reader)
  mfrc522.PCD_AntennaOff();
  mfrc522.PICC_HaltA();
  SPI.end();
  pinMode(STAT_LED, OUTPUT);

#if SERIAL_DEBUG
  Serial.println("\nRequesting: " + remoteID);
#endif

  // Build the request init http client
  WiFiClient client;
  HTTPClient http;
  String request = uri + remoteID;
  http.begin(client, "192.168.178.200", port, request, false);

  // http.setReuse(true);
  http.useHTTP10(true); // Stream only useable on HTTP1.0 but no keep-alive
  http.addHeader("accept", "application/json");
  http.addHeader("X-AUTH-USER", api_user);
  http.addHeader("X-AUTH-TOKEN", api_token);

  // Send the request
  uint16_t httpCode = http.GET();

  // if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NOT_FOUND) {
  DeserializationError error = deserializeJson(doc, (Stream &)http.getStream());
  if (error)
  {
#if SERIAL_DEBUG
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
#endif
    mfrc522.PCD_AntennaOn();
    return;
  }

  // Build a readable response for debug or display msg
  switch (httpCode)
  {
  case HTTP_CODE_OK:
  {
    d_user = doc["user"].as<String>();
#if SERIAL_DEBUG || SERIAL_RESPONSE
    Serial.println("\nUser id: " + d_user);
#endif
    float duration = doc["duration"].as<unsigned int>() / 60;
#if SERIAL_DEBUG || SERIAL_RESPONSE
    Serial.print("Duration: ");
#endif
    if (duration >= 60)
    {
      d_duration = String(duration / 60) + " h";
#if SERIAL_DEBUG || SERIAL_RESPONSE
      Serial.printf("Stop %4.2f h\n", duration / 60);
#endif
    }
    else if (duration > 0)
    {
      d_duration = String((int)duration) + " min";
#if SERIAL_DEBUG || SERIAL_RESPONSE
      Serial.printf("Stop %d min\n", (int)duration);
#endif
    }
    else
    {
      d_duration = "Started";
#if SERIAL_DEBUG || SERIAL_RESPONSE
      Serial.println(d_duration);
#endif
    }
    ticker.attach(0.8, tick);
#if OLED_DISP
    ui.transitionToFrame(1);
#endif
    break;
  }
  case HTTP_CODE_NOT_FOUND:
  {
#if SERIAL_DEBUG || SERIAL_RESPONSE
    d_user = doc["message"].as<String>();
    Serial.println("\nError: " + d_user);
#endif
    ticker.attach(0.5, tick);
#if OLED_DISP
    ui.transitionToFrame(2);
#endif
    break;
  }
  default:
  {
#if SERIAL_DEBUG
    Serial.printf("Error: %d \nRestarting device...\n", httpCode);
#endif
    http.end();
    client.stop();
    ticker.attach(0.1, tick);
    delay(4000);
    ESP.restart();
    break;
  }
  }
  timer = millis();
}

//#################
//--- Config & WiFi manager
//#################

void wifiManInit()
{
  if (SPIFFS.begin())
  {
    if (SPIFFS.exists("/config.json"))
    {
#if OLED_DISP
      display.drawString(0, 14, "Trying to load config.");
      display.display();
#endif

      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument doc(256);
        auto error = deserializeJson(doc, buf.get());
#if SERIAL_DEBUG
        serializeJson(doc, Serial);
#endif
        if (!error)
        {
#if SERIAL_DEBUG
          Serial.println("\nparsed json");
#endif
          strcpy(kimai_server, doc["kimai_server"]);
          strcpy(kimai_port, doc["kimai_port"]);
          strcpy(api_user, doc["api_user"]);
          strcpy(api_token, doc["api_token"]);
        }
        else
        {
#if SERIAL_DEBUG
          Serial.println("failed to load json config");
          Serial.println(error.c_str());
#endif
        }
        configFile.close();
      }
    }
  }
  else
  {
#if SERIAL_DEBUG
    Serial.println("failed to mount FS");
#endif
  }

  WiFiManagerParameter custom_kimai_server("server", "Kimai server",
                                           kimai_server, 40);
  WiFiManagerParameter custom_kimai_port("port", "Kimai port", kimai_port, 6);
  WiFiManagerParameter custom_api_user("user", "api user", api_user, 32);
  WiFiManagerParameter custom_api_token("token", "api token", api_token, 32);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(SERIAL_DEBUG);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_kimai_server);
  wifiManager.addParameter(&custom_kimai_port);
  wifiManager.addParameter(&custom_api_user);
  wifiManager.addParameter(&custom_api_token);

  if (!wifiManager.autoConnect("AutoConnectAP", "password"))
  {
#if SERIAL_DEBUG
    Serial.println("failed to connect and hit timeout");
#endif
#if OLED_DISP
    display.drawString(0, 26, "Starting AP");
    display.display();
#endif
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  ticker.detach();

  strcpy(kimai_server, custom_kimai_server.getValue());
  strcpy(kimai_port, custom_kimai_port.getValue());
  strcpy(api_user, custom_api_user.getValue());
  strcpy(api_token, custom_api_token.getValue());

  if (shouldSaveConfig)
  {
#if SERIAL_DEBUG
    Serial.println("saving config");
#endif
    DynamicJsonDocument doc(256);
    doc["kimai_server"] = kimai_server;
    doc["kimai_port"] = kimai_port;
    doc["api_user"] = api_user;
    doc["api_token"] = api_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
#if SERIAL_DEBUG
      Serial.println("failed to open config file for writing");
#endif
    }
    serializeJson(doc, Serial);
    serializeJson(doc, configFile);
    configFile.close();
  }
#if OLED_DISP
  display.drawString(0, 26, "Connecting to " + WiFi.SSID());
  display.display();
#endif

#if SERIAL_DEBUG
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
#endif

  port = atoi(kimai_port);
}

void tick() { digitalWrite(STAT_LED, !digitalRead(STAT_LED)); }

// callback notifying us of the need to save config
void saveConfigCallback()
{
#if SERIAL_DEBUG
  Serial.println("Should save config");
#endif
  shouldSaveConfig = true;
}

//#################
//--- main
//#################

int getBarsSignal(long rssi)
{
  // 5. High quality: 90% ~= -55db
  // 4. Good quality: 75% ~= -65db
  // 3. Medium quality: 50% ~= -75db
  // 2. Low quality: 30% ~= -85db
  // 1. Unusable quality: 8% ~= -96db
  // 0. No signal
  uint8_t bars;

  if (rssi > -55)
  {
    bars = 5;
  }
  else if ((rssi < -55) & (rssi > -65))
  {
    bars = 4;
  }
  else if ((rssi < -65) & (rssi > -75))
  {
    bars = 3;
  }
  else if ((rssi < -75) & (rssi > -85))
  {
    bars = 2;
  }
  else if ((rssi < -85) & (rssi > -96))
  {
    bars = 1;
  }
  else
  {
    bars = 0;
  }
  return bars;
}

void sigRefresh()
{
  if (WiFi.isConnected())
  {
    sigStrength = getBarsSignal(WiFi.RSSI()) * 20;
  }
}

//#################
//--- Display
//#################

void displayInit()
{
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Starting..");
  display.display();
}

void uiInit()
{
  display.clear();
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);
  ui.disableAutoTransition();
  ui.disableAllIndicators();
  ui.setTargetFPS(60);
  ui.setTimePerTransition(800);
  ui.init();
  display.flipScreenVertically();

  // Refresh WiFi signal strength every 5 seconds
  wifiTicker.attach(5, sigRefresh);

  // init timeclient for clock
  timeClient.begin();
  timeClient.update();
}

void printTopline(OLEDDisplay *display, OLEDDisplayUiState *state)
{
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(2, 0, "Time " + timeClient.getFormattedTime());
  display->drawProgressBar(100, 0, 26, 10, sigStrength);
  display->drawXbm(88, 0, 10, 10, Logo_bits);
  display->drawLine(0, 12, 128, 12);
}

void printResponse(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 22, d_user);
  display->drawString(0, 48, d_duration);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0, 12, "User:");
  display->drawString(0, 38, "Timesheet:");
}

void printError(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 22, d_user);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0, 12, "Error:");
}

// Example from Thingpulse
void printClock(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  display->drawCircle(clockCenterX + x, clockCenterY + y, 2);

  //hour ticks
  for (int z = 0; z < 360; z += 30)
  {
    //Begin at 0° and stop at 360°
    float angle = z;
    angle = (angle / 57.29577951); //Convert degrees to radians
    int x2 = (clockCenterX + (sin(angle) * clockRadius));
    int y2 = (clockCenterY - (cos(angle) * clockRadius));
    int x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 8))));
    int y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 8))));
    display->drawLine(x2 + x, y2 + y, x3 + x, y3 + y);
  }

  // display second hand
  float angle = timeClient.getSeconds() * 6;
  angle = (angle / 57.29577951); //Convert degrees to radians
  int x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 5))));
  int y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 5))));
  display->drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);

  // display minute hand
  angle = timeClient.getMinutes() * 6;
  angle = (angle / 57.29577951); //Convert degrees to radians
  x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 4))));
  y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 4))));
  display->drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);

  // display hour hand
  angle = timeClient.getHours() * 30 + int((timeClient.getMinutes() / 12) * 6);
  angle = (angle / 57.29577951); //Convert degrees to radians
  x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 2))));
  y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 2))));
  display->drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);
}