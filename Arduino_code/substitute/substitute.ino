

/*
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#ifdef ESP8266 
       #include <ESP8266WiFi.h>
#endif 
#ifdef ESP32   
       #include <WiFi.h>
#endif

#define FASTLED_ESP8266_DMA
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "UbidotsESPMQTT.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "SinricPro.h"
#include "SinricProLight.h"

#define WIFI_SSID         "Aryan"    
#define WIFI_PASS         "12345678"
#define APP_KEY           "7e8bb5c8-ccf2-4435-8761-ad360a39f065"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "dd111aee-6392-496f-be2d-b071be6aa770-c5c6dd55-86c4-487a-903a-dc34d655f8c9"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define LIGHT_ID          "6433207f918a3c911c79ace8"    // Should look like "5dc1564130xxxxxxxxxxxxxx"
#define BAUD_RATE         9600                // Change baudrate to your need

#define NUM_LEDS          7                  // how much LEDs are on the stripe
#define LED_PIN           D0                 // LED stripe is connected to PIN 3
#define BUTTON_PIN        D1

char* UBIDOTS_TOKEN = "BBFF-PCTOTaKFN5lc6IHELBgbdngokiT48p";  // Put here your Ubidots TOKEN
char* ubidots_device = "LedStrip";  // Replace with your device label
Ubidots ubidots(UBIDOTS_TOKEN);



bool powerState=true;        
int globalBrightness = 100;
unsigned long lastBtnPress = 0;  
CRGB leds[NUM_LEDS];
unsigned long last_update_time =0;

AsyncWebServer OTA_server(80);

bool onPowerState(const String &deviceId, bool &state) {
  powerState = state;
  if (state) {
    FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
  } else {
    FastLED.setBrightness(0);
  }
  FastLED.show();
  FastLED.show();
  return true; // request handled properly
}

bool onBrightness(const String &deviceId, int &brightness) {
  globalBrightness = brightness;
  FastLED.setBrightness(map(brightness, 0, 100, 0, 255));
  if(powerState){
    FastLED.show();
    FastLED.show();
  }
  
  return true;
}

bool onAdjustBrightness(const String &deviceId, int brightnessDelta) {
  globalBrightness += brightnessDelta;
  brightnessDelta = globalBrightness;
  FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
  if(powerState){
    FastLED.show();
    FastLED.show();
  }
  return true;
}

bool onColor(const String &deviceId, byte &r, byte &g, byte &b) {
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  if(powerState){
    FastLED.show();
    FastLED.show();
  }
  return true;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void sendUbidots(){
  ubidots.add("PowerState", powerState);
  //ubidots.add("Variable_Name_Three", value3);

  bool bufferSent = false;
  bufferSent = ubidots.ubidotsPublish(ubidots_device); 
  ubidots.loop();
}

void setupUbidots(){
    ubidots.wifiConnection(WIFI_SSID, WIFI_PASS);
    ubidots.begin(callback);
    // Serial.println(connected);
    sendUbidots();
    
}

void setupFastLED() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
  fill_solid(leds, NUM_LEDS, CRGB::White);
  last_update_time = millis();
  FastLED.show();
  FastLED.show();
}

void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", localIP.toString().c_str());
}

void setupAsyncOTA(){
  OTA_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am Dodo. The smart Bulb.");
  });
  AsyncElegantOTA.begin(&OTA_server);
  OTA_server.begin();
}

void setupSinricPro() {
  // get a new Light device from SinricPro
  SinricProLight &myLight = SinricPro[LIGHT_ID];

  // set callback function to device
  myLight.onPowerState(onPowerState);
  myLight.onBrightness(onBrightness);
  myLight.onAdjustBrightness(onAdjustBrightness);
  myLight.onColor(onColor);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  //SinricPro.restoreDeviceStates(true); // Uncomment to restore the last known state from the server.
  SinricPro.begin(APP_KEY, APP_SECRET);
}


void handleButtonPress() {
  unsigned long actualMillis = millis(); 
  SinricProLight &myLight = SinricPro[LIGHT_ID];
  if (digitalRead(BUTTON_PIN) == HIGH && actualMillis - lastBtnPress > 1000)  { 
    if (powerState) {     // flip powerState and update the FastLED content
      powerState = false;
      FastLED.setBrightness(0);
    } else {
      powerState = true;
      FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
     
    }
    FastLED.show(); // show the updates
    FastLED.show();
  
    myLight.sendPowerStateEvent(powerState); // send the new powerState to SinricPro server

    Serial.printf("Device %s turned %s (manually via pushbutton)\r\n", myLight.getDeviceId().c_str(), powerState?"on":"off");

    lastBtnPress = actualMillis;
  } 
}



// main setup function
void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  setupFastLED();
  setupWiFi();
  setupAsyncOTA();
  setupUbidots();
  setupSinricPro();
}

void loop() {
  handleButtonPress();
  if(millis()-last_update_time>10000){
    sendUbidots();
    last_update_time = millis();    
  }
  SinricPro.handle();
}
