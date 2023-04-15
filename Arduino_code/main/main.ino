/*
 * Example
 *
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

 // Custom devices requires SinricPro ESP8266/ESP32 SDK 2.9.6 or later

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG


#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <FastLED.h>



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

#include <SinricPro.h>
#include "LedStrip.h"

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>


#define APP_KEY    "7e8bb5c8-ccf2-4435-8761-ad360a39f065"
#define APP_SECRET "dd111aee-6392-496f-be2d-b071be6aa770-c5c6dd55-86c4-487a-903a-dc34d655f8c9"
#define DEVICE_ID  "6432d224918a3c911c7963f2"

#define SSID       "Aryan"
#define PASS       "12345678"

#define BAUD_RATE  9600

#define NUM_LEDS          7                   // how much LEDs are on the stripe
#define LED_PIN           D0                   // LED stripe is connected to PIN 3
#define BUTTON_PIN        D1
#define UPDATES_PER_SECOND 100


#define MQTT_HOST IPAddress(192, 168, 43,42)
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
char payload[2] = "\0";
bool enabled = false;


LedStrip &ledStrip = SinricPro[DEVICE_ID];

/*************
 * Variables *
 ***********************************************
 * Global variables to store the device states *
 ***********************************************/

// PowerStateController
bool globalPowerState=true;

// ModeController
std::map<String, String> globalModes;

// BrightnessController
int globalState = 1;
unsigned long last_update_time = 0;
unsigned long last_update_time_blink = 0;


// global variables 
unsigned long lastBtnPress = 0;        
int globalBrightness = 100;
CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

/*************
 * Callbacks *
 *************/

// PowerStateController
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("[Device: %s]: Powerstate changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
  globalPowerState = state;
  
  if (state) {
    FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
    globalState = ((globalState%2)==1)?globalState:globalState+1;
  } else {
    FastLED.setBrightness(0);
    globalState = ((globalState%2)==0)?globalState:(globalState-1);
  }
  FastLED.show();
  FastLED.show();
  return true; // request handled properly
}

// ModeController
bool onSetMode(const String& deviceId, const String& instance, String &mode) {
  Serial.printf("[Device: %s]: Modesetting for \"%s\" set to mode %s\r\n", deviceId.c_str(), instance.c_str(), mode.c_str());
  globalModes[instance] = mode;
  if(instance == "modeInstance1"){
    onThemeSetMode(deviceId, mode);
  }
  return true;
}

void onThemeSetMode(const String& deviceId, String &mode){
  if(mode == "Party"){
    Serial.printf("Inside if for party");
    currentPalette = PartyColors_p;           
    currentBlending = LINEARBLEND;
    FillLEDsFromPaletteColors(0);
    // fill_solid(leds, NUM_LEDS, CRGB::Green);
    globalState = 3;
  }
  else if(mode == "Rainbow"){
    currentPalette = RainbowColors_p;        
    currentBlending = LINEARBLEND;
    FillLEDsFromPaletteColors(0);
    // fill_solid(leds, NUM_LEDS, CRGB::Red);
    globalState = 5;
  }
  else if(mode == "Cloudy"){
    currentPalette = CloudColors_p;           
    currentBlending = LINEARBLEND;
    FillLEDsFromPaletteColors(0);
    // fill_solid(leds, NUM_LEDS, CRGB::Yellow);
    globalState =7;
  }
  else if(mode == "Normal"){
    // default, set to black and white stripes
    // SetupBlackAndWhiteStripedPalette();       
    // currentBlending = NOBLEND;
    fill_solid(leds, NUM_LEDS, CRGB::White);
     globalState = 1;
  }else if(mode == "Power Saver"){
     enabled = !enabled;    
  }
  // FillLEDsFromPaletteColors(0);
   if((globalState%2)!=globalPowerState){
          globalState = globalPowerState ? globalState : globalState - 1;
   }
   if(globalPowerState){
    FastLED.show();
    FastLED.show();  
   } 
}

// BrightnessController
bool onBrightness(const String &deviceId, int &brightness) {
  Serial.printf("[Device: %s]: Brightness set to %d\r\n", deviceId.c_str(), brightness);
  globalBrightness = brightness;
  FastLED.setBrightness(map(brightness, 0, 100, 0, 255)); 
  if(globalPowerState){
    FastLED.show();
    FastLED.show();  
  } 
  
  return true; // request handled properly
}

bool onAdjustBrightness(const String &deviceId, int &brightnessDelta) {
  globalBrightness += brightnessDelta; // calculate absolute brigthness
  Serial.printf("[Device: %s]: Brightness changed about %i to %d\r\n", deviceId.c_str(), brightnessDelta, globalBrightness);
  brightnessDelta = globalBrightness; // return absolute brightness
  FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
  // FastLED.setTemperature()
  if(globalPowerState){
    FastLED.show();
    FastLED.show();  
  } 
  return true; // request handled properly
}

/**********
 * Events *
 *************************************************
 * Examples how to update the server status when *
 * you physically interact with your device or a *
 * sensor reading changes.                       *
 *************************************************/

// PowerStateController
void updatePowerState(bool state) {
  ledStrip.sendPowerStateEvent(state);
}

// ModeController
void updateMode(String instance, String mode) {
  ledStrip.sendModeEvent(instance, mode, "PHYSICAL_INTERACTION");
}

// BrightnessController
void updateBrightness(int brightness) {
  ledStrip.sendBrightnessEvent(brightness);
}

/********* 
 * Setup *
 *********/

void setupSinricPro() {

  // PowerStateController
  ledStrip.onPowerState(onPowerState);

  // ModeController
  ledStrip.onSetMode("modeInstance1", onSetMode);


  // BrightnessController
  ledStrip.onBrightness(onBrightness);
  ledStrip.onAdjustBrightness(onAdjustBrightness);

  SinricPro.onConnected([]{ Serial.printf("[SinricPro]: Connected\r\n"); });
  SinricPro.onDisconnected([]{ Serial.printf("[SinricPro]: Disconnected\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
};


void setupFastLED() {
  FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
  fill_solid(leds, NUM_LEDS, CRGB::White);
  last_update_time = millis();
  last_update_time_blink = millis();
  globalState = 1;
  FastLED.show();
  FastLED.show();
}




void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe("powerSave/alert", 2);
  // payload[0]=(char)9;  
  // uint16_t packetIdPub = mqttClient.publish("powerSave/analysis", 2, true,payload);

  
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    //   Serial.println("buzz1");
    //   char* payloadptr=payload;
    //   for(int i=0; i<len; i++)
    // {
    //     putchar(*payloadptr++);
    // }
    // putchar('\n');      
       
     if(globalPowerState && (strncmp(payload,"inactive",strlen("inactive"))==0)){ 
        // Serial.println("buzz");               
        globalBrightness = globalBrightness/2;
        FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
        if(globalBrightness==0){
            globalPowerState = 0;
            globalState = (globalState%2)==0 ? globalState : globalState -1;
            ledStrip.sendPowerStateEvent(globalPowerState);
        }
        updateBrightness(globalBrightness); 
        FastLED.show();
        FastLED.show();        

     }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setupMqtt(){
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}

void setupWiFi() {
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected\r\n");
  connectToMqtt();
}

void setupArduinoOTA(){
  ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}




void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    
    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, map(globalBrightness, 0, 100, 0, 255), currentBlending);
        colorIndex += 3;
    }
}


void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};

void blinkColor(){
    ChangePalettePeriodically();
    
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors(startIndex);
    
    FastLED.show();
    FastLED.show();
}




void handleButtonPress() {
  unsigned long actualMillis = millis(); 
  if (digitalRead(BUTTON_PIN) == HIGH && actualMillis - lastBtnPress > 1000)  { 
    if (globalPowerState) {     // flip powerState and update the FastLED content
      globalPowerState = false;
      FastLED.setBrightness(0);
      globalState = ((globalState%2)==1)?(globalState-1):globalState;
    } else {
      globalPowerState = true;
      FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255));
      globalState = ((globalState%2)==1)?globalState:globalState+1;
    }
    FastLED.show(); // show the updates
    FastLED.show();
  
    ledStrip.sendPowerStateEvent(globalPowerState); // send the new powerState to SinricPro server

    Serial.printf("Device %s turned %s (manually via pushbutton)\r\n", ledStrip.getDeviceId().c_str(), globalPowerState?"on":"off");

    lastBtnPress = actualMillis;
  } 
}



void setup() {
  Serial.begin(BAUD_RATE);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  setupFastLED();
   
  //setupUbidots();
  setupMqtt();
  setupArduinoOTA();
  setupWiFi();
  setupSinricPro();
}

/********
 * Loop *
 ********/

void loop() {
   handleButtonPress();
   
  unsigned long actual_time = millis();
  if(actual_time-last_update_time>10000){
      //getUbidots();
      if(enabled){      
         Serial.println(globalState);
         payload[0]=(char)globalState;
      }else if(globalPowerState)
         payload[0]=(char)9;  
      else
         payload[0]=(char)8;        
     // sendUbidots();   
      //Serial.println("Publishing Packet ...");    
      uint16_t packetIdPub = mqttClient.publish("powerSave/analysis", 2, true,payload);
      last_update_time = millis();
  }
  if(actual_time-last_update_time_blink>1000/ UPDATES_PER_SECOND ){
    if(globalState==3){
      blinkColor();
    }
    last_update_time_blink = millis();
  }

  ArduinoOTA.handle();
  SinricPro.handle();
}