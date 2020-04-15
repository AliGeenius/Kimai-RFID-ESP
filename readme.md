# Kimai RFID ESP
This programm is build for for an ESP8265/ESP8266 to communicate with the open source [Kimai timetracking software](https://www.kimai.org) via the API sending RFIDs to the server for starting/stopping timesheets. Written in C++ with PlatformIO Extension in VSCode.

## Features

+ Show username and timesheet duration
+ Captive Portal for configuration
+ OLED display support
+ NTP Client for current time

[Little video preview](https://cloud.aligeenius.de/s/RLkSndS8BSYRxQW)

## Intro

The programm is omptimized for an ESP8265 controller but the performance should be the same on a ESP8266.
I made it modular for the usage with serial output or/and a display. 
The platformio.ini file includes settings for you environment like board type, debug (for WiFiManager and HTTPClient) and board speed.
In the main.ccp you can set the debug for the programm or enable/disable display.

All links to the needed parts are picked randomly on Amazon and maybe you can find cheaper modules on other sites.

## First Start

The first start will set up a WiFi accesspoint that you can manage your settings for WiFi and your Kimai Server. 
Note: Actually theres no HTTPS client implemented!

![alt text][captive_portal]

If everything went well you are ready to run your Kimai remote device.

## Usage

The usage is simply requesting the Remote Kimai API with the RFID and printing infos via serial or showing infos on display.

## Status with display

(My camera is faster than the refreshing rate, so not everything seems to be correctly drawn on the display) 
After a successful start the menu is a analogue clock.

![alt text][oled_idle]

When reading a card and doing a successful request to the Kimai API the status of the timesheet will be shown.

![alt text][oled_response]

## Status without display or serial debug

+ Constant LED light when connected to WiFi
+ 1 Hz blink after successfull request
+ 2 Hz blink on a not registred card
+ 10 Hz blink on connection errrors

# Hardware

In my case I used a "Sonoff Touch" switch for the reader thats based on a ESP8265 contoller. But the librarys are also usable for the ESP8266 controllers.
On the PCB of the Sonoff Switch is a useable LED that can used for displaying the status.
Additionally a MFRC522 RFID reader is neccessary and if you want to a OLED display.

## Sonoff hack ESP8265

### Prerequirements

+ [Sonoff Touch switch](https://www.amazon.de/Sonoff-Touch-Lichtschalter/dp/B077R59S7Z/ref=sr_1_11?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=sonoff+touch&qid=1586978865&sr=8-11)
+ [SPI RFID reader board](https://www.amazon.de/AZDelivery-Reader-Arduino-Raspberry-gratis/dp/B01M28JAAZ/ref=sr_1_4?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=rfid+reader&qid=1586978958&sr=8-4)
+ [UART interface (with 3.3 V supply)](https://www.amazon.de/UART-Wandler-Adapter-serielle-Schnittstelle-Converter-Anschlussleitung/dp/B0773G2K92/ref=sr_1_7?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=uart+usb&qid=1586978990&sr=8-7)
+ soldering iron
+ thin cable
+ (Drill if you want to reuse the onboard LED)
+ [I2C OLED Display](https://www.amazon.de/Gosear-Zoll-serielle-Display-Modul-Arduino-blau/dp/B00NHKM1C0/ref=sr_1_19?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=i2c+oled&qid=1586979040&sr=8-19)
+ coffee ☕

![alt text][sonoff_mats]

### Preparation
The Sonoff switch consits of two PCBs the controller and power part that are connected with 4 pins. We just have to mod the upper controller PCB that's glued with double sided tape to the power PCB.
The reader fits very well into the enclosure.

![alt text][sonoff_prep]

Theres a flat white diffusor for the relay status on top of the PCB. We have to remove it to reduce the height.


Now there are three ways:
1. If you want to reuse the WiFi status LED of the switch you have to drill through the readers PCB
2. Use the LED and modify the reader
3. Dont use the LED and dont modify the reader

I encountered some problems with the reader when the controller board is really close to it. Only the cards delivered with the reader work well. I guess it's a shielding problem.
But to get more distance to the controller PCB you need to change the readers crystal position so you can place the reader with the coil side to the enclosure.

You also should remove one of the 4 pins for the power PCB (see picture in wireing). Its connected to the relay but the pin (12) will be used for the reader.

### Wireing

![alt text][sonoff_pins]

|Description|ESP GPIO|Reader Pin|Display Pin|
|---|:---:|:---:|:---:|
|Chip select| 15 |SDA||
|Clock| 14 |SCK||
|MOSI and LED|13|MOSI||
|MISO|12|MISO||
|Reset| 0| RST||
|Data| 4 | | SDA|
|Clock| 5| | SCL|
|Ground|anywhere|GND|GND|
|VCC (3,3 V)|anywhere|VCC|VCC| 

In my case it looks like this.

![alt text][sonoff_wireing]

## NodeMCU (ESP8266)

With the NodeMCU it should be a plug and play solution without soldering and just useing the jumper cables. But sometimes the board headers aren't presoldered so please have a look before buying.

### Prerequirements
+ [NodeMCU](https://www.amazon.de/AZDelivery-NodeMCU-ESP8266-ESP-12E-Development/dp/B06Y1ZPNMS/ref=sr_1_3?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=nodemcu&qid=1586979946&sr=8-3)
+ [SPI RFID reader board](https://www.amazon.de/AZDelivery-Reader-Arduino-Raspberry-gratis/dp/B01M28JAAZ/ref=sr_1_4?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=rfid+reader&qid=1586978958&sr=8-4)
+ [jumper cables](https://www.amazon.de/Female-Female-Male-Female-Male-Male-Steckbr%C3%BCcken-Drahtbr%C3%BCcken-bunt/dp/B01EV70C78/ref=sr_1_3?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=jumper+kabel&qid=1586980081&sr=8-3)
+ [I2C OLED Display](https://www.amazon.de/Gosear-Zoll-serielle-Display-Modul-Arduino-blau/dp/B00NHKM1C0/ref=sr_1_19?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=i2c+oled&qid=1586979040&sr=8-19)
+ coffee ☕

The GPIOs are compatibel with the ESP8265. Just use the upper table.

# Software

1. Clone or download this git.
2. Open in PlatformIO
3. Do your configstuff
4. Flash your device

## Building

You need PlatformIO to build the code. It should install all neccessary libraries automaticly. Choose your build environment in the plaform.ini file. Actually it's just building for the ESP8265. Change it to your needs. In this file you can also setup specific parameters like the debug settings for the HTTP client.
```ini
;example for the Sonoff hack
[platformio]
default_envs = esp8285
```
In the src/main.h file are additional settings for the usage with a display or for serial debugging.

```c++
#define SERIAL_DEBUG true
#define SERIAL_RESPONSE true
#define OLED_DISP true
```
You also should configure your timezone.

```c++
// edit it to your needs. The 7200 stands for + 2h
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);
```

## Flashing

### Sonoff 

For flashing the Sonoff board you'll need an UART port eg. with an USB TTL adapter. For further details have a look for Sonoff flashing. there are several guides out. 

### ESP8266 with USB port

Just put in a micro USB cable and upload it.. 

## Used librarys

+ [WiFi manager](https://github.com/tzapu/WiFiManager)
+ [Arduino Json](https://github.com/bblanchon/ArduinoJson)
+ [RFID Reader](https://github.com/miguelbalboa/rfid)
+ [NTP](https://github.com/arduino-libraries/NTPClient)
+ [Display](https://github.com/ThingPulse/esp8266-oled-ssd1306)

Thank you guys for your great work!

## Known issues

+ In my Kimai test environment with XAMPP on Windows the request time was sometimes over 3 seconds. Running it on my Nginx server response was under 400 ms with a non persistent TCP connection
+ The refreshing of the WiFI signal is done every 5 seconds. Sometimes the RSSI value isn't correct in that moment so the signal strength sometimes varies a little bit.

[sonoff_mats]: images/sonoff_materials.JPG "Sonoff materials"
[sonoff_prep]: images/sonoff_prep.JPG "Sonoff preparation"
[captive_portal]: images/captive_portal.png "Captive Portal"
[sonoff_pins]: images/sonoff_pins.jpg "Sonoff pins"
[oled_idle]: images/oled_idle.JPG "Menu idle"
[oled_response]: images/oled_response.JPG "Response"
[sonoff_wireing]: images/sonoff_wireing.JPG "Sonoff wireing"

