# Alexa/ Google Home App controlled LED Strip
This project is a part of our academic course Internet of Things (CSN-527).

## Problem Statement
Control an addressable LED Strip using Alexa/ Google Home App. We should be able to do the following:
- Turn ON/OFF the LEDs
- Change the Brightness
- Change the Color of LEDs

## Motivation

The motivation behind this project is to leverage the power of Internet of Things (IoT) technology and voice-controlled virtual assistants to create a smart and convenient lighting solution for homes. It aims to provide a seamless way for users to control their LED strip lights using voice commands through voice assistants like Alexa and Google Home app thus reducing the need for manual control. 

This project seeks to improve the user experience by making lighting control more efficient, convenient, and modern. With the integration of IoT technology, users can control their LED strip lights from anywhere in their homes using voice commands, without having to physically access the switch or even use their smartphones. This can be especially beneficial for individuals with mobility issues, disabilities, or those who simply prefer a hands-free approach to home automation.


## Devices Used
- ESP8266
- Addressable LED Strip(WS2812B)
- Push Button
- Female Jumper Wires

Here the Push Button acts as a manual switch for the LED Strip.

## Software Components Used
- FastLED.h library
- Wifi.h library for wifi provisioning
- SinricPro, SinricProLight
- SinricPro / ESP Rainmaker web platform and apps
- AsyncElegantOTA.h, ArduinoOTA.h (for OTA functionality)
- UbidotsESPMQTT.h (for Ubidots integration)
- AsyncMqttClient
- Arduino IDE
- Alexa, Google home app 

Most of IoT devices use lightweight **MQTT** protocol to exchange messages with IoT clouds. On the other hand Alexa /Google Assistant use **REST APIs** services. In such scenario platforms like ESP Rainmaker, SinricPro provides users with interface that can understand REST as well as MQTT.  We are using Sinric Pro Library. 

*Sincric Pro* Library provides us with a higher level interface to connect to IoT Cloud without knowing specifics of MQTT protocol.

## Circuit Diagram
![Circuit diagram](/Circuit.png)

The WS2812B LED strip is powered using the ESP8266 5V, thus ESP8266 5V is connected to the VCC on the LED Strip. The ESP8266 GND is connected to the GND on the LED Strip. We connect the DATA of LED strip to the D0 pin on ESP8266.

The Push Button is powered using the ESP8266 3.3V, thus ESP8266 3.3V is connected to the VCC on the Push Button. The ESP8266 GND is connected to the GND on the Push Button. We connect the DATA of Push Button to the D1 pin on ESP8266.

## Functionalities Offered
- **Controlling LED Strip using Alexa/ Google Home app** using Sinric Pro Library
  - User can say "Hey Google! Decrease the brightness by 50% of Living Room LED", and the brightness will be decreased by 1/2.
- **Multiple Light modes available** for the Smart Bulb which can be controlled using Alexa app
  - Created Custom Device template using Sinric Pro library
  - Users can change from **Normal** to *blinking* **Party** mode in just a click
- **OTA functionality** added which allows user to upload a new sketch using wifi
  - Done using AsyncElegantOTA as well as ArduinoOTA library
  - Useful in case of when we don't have physical access to the Board and we want to make some updates
- **PowerSaver** Option available
  - If turned ON, it keeps reducing the brightness by 50% for every inactive interval of 60 seconds
- **Data Analysis using Ubidots integration**
  - These plots can be used to analyze the usage of the LEDs

## Code Setup

### Install ESP8266
- Include library <ESP8266WiFi.h> and install ESP8266 Board in Arduino IDE. For installation, go to Arduino IDE and follow the path File/preferences and open the preference tab. Paste the link http://arduino.esp8266.com/stable/package_esp8266com_index.json in the additional board manager URL box. 
- After this, go to Tool/ Board Tools/board/board manager and type ESP8266. 
- You will find a board of ESP8266 click on the install option to get the board installed.

### For Smart Bulb
1. Sign up in Sinric Pro
    - Click on Devices, then click on Add Device
    - Add the Device Name(call it Smart Bulb), description and Device Type(Smart Light Bulb)
    - Sinric Pro associates every Device with a DeviceID(let's call it *Light_ID*)
2. Now update the *Light_ID*, your App key and App secret and the Wifi credentials in the code(`Arduino_code/substitute/substitute.ino`).

### For Smart LED
1. Sign up in Sinric Pro
    - Click on Device Templates, then click on Add Device Template
    - Add the Template Name(eg: LEDStrip), description and Device Type(Smart Light Bulb)
    - Select capabilities by drag and drop. We can only choose 3 in non-premimum mode.
      - Select Power, Mode and Brightness
      - Configure Mode, add Mode name as *Theme* and add new mode values
      - To replicate our code, add mode values as Party, Power Saver, Normal, Cloudy
    - Now click on Devices, then click on Add Device
    - Add the Device Name(call it Smart LED), description and Device Type
      - Select the Device Type as the Device Template you just created(LEDStrip).
    - Sinric Pro associates every Device with a DeviceID(let's call it *Light_ID*)
2. Now update the *Light_ID*, your App key and App secret and the Wifi credentials in the code(`Arduino_code/main/main.ino`).


## Working
This section shows the workflow of the project.
### Smart Bulb
| ![Arduino Script Design for Smart Bulb](/Arduino-design-2.png) |
|:--:| 
| *Arduino Script Design for Smart Bulb* |
1. The ESP8266 first connects to the WIFI using the specified credentials.
2. We have created a new Light Device using Sinric Pro library. Like:  `SinricProLight &myLight = SinricPro[*Light_ID*]`
    - This is the Light Device associated with the Smart Bulb we created on the Sinric Pro library.
3. This library has callback functions that are called every time a variable is changed. We define the callback function for this light device.
4. *onPowerState* callback function will be called everytime *PowerState* of the *myLight* will change on the Cloud.

`bool onPowerState(const String &deviceId, bool &state) {
  powerState = state;
  if (state) {
    FastLED.setBrightness(map(globalBrightness, 0, 100, 0, 255)); // Turn ON the LEDs
  } else {
    FastLED.setBrightness(0); // Turn OFF the LEDs
  }
  FastLED.show();
  return true;
}`

6. *onBrightness*, *onAdjustBrightness* and *onColor" callback functions are also defined.
7. We have also integrated Ubidots where we send the `PowerState` variable's values to the cloud.
8. Now the following code runs in loop
    - The function `handleButtonPress` which handles the push button. It checks if the push button is being pressed or not, if yes then it toggles the Power state of the LEDs.
      - To update the *PowerState* back to the Cloud, we use the function `sendPowerStateEvent()` provided by the Sinric Pro library with the newly updated value of *PowerState*.
    - Then it checks if its been more than 10 seconds that we sent an update to *Ubidots*, if yes then it sents the updated *PowerState* to Ubidots.
    - Then it handles any updates from the Sinric Pro platform which includes updates from Alexa/ Google Home aap and the Sinric Pro app.
      - If user turns OFF the Smart Bulb using any of the above specified platform
      - The callback function corresponding to *PowerState* i.e *onPowerState* is called and the FastLED finally turns OFF the LED.
      - If user changed the color of the Smart Bulb using any of the above specified platform
      - The callback function corresponding to *color* i.e *onColor* is called and the FastLED finally sets the color to the specified one.


### Smart LED
|![Arduino Script Design for Smart LED](/Arduino-design-1.png)|
|:--:| 
| *Arduino Script Design for Smart LED* |

## Extensions
### Power Saver
It is an option which we can enable. If this option is enabled, it reduces the brightness by 50% for every inactive interval of 60 seconds. 
For this we have created a C server `analyser.c` which makes use of `MQTTClient.h` library. 

### OTA functionality
We have added OTA feature so that user can upload a new sketch whenever required without the necessity of having the serial connection with the ESP8266 board.
We have used AsyncElegantOTA library in out **Smart Bulb** code. For this we included the `AsyncElegantOTA.h` library. We have created a AsyncWebServer on the port 80 namely *OTA_server*. Then we setup the AsyncOTA and begin this *OTA_server*. To upload a new sketch go to `localhost/update`. The below image is the update page.
![OTA update page](/ElegantOTAUpdate.png)

### Data Integration with Ubidots
We have integrated Ubidots using the MQTT protocol in the *Smart Bulb* code. It sends the updated value of the *PowerState* variable to the Ubidots after every 10 seconds. This data can be analyzed to better understand the usage of the bulb and how it is related to time for a individual person. This can be later used to set Night Modes, Power Saver mode at some scheduled time that statifies the user's light pattern.
| ![Power State Data Plot using Ubidots](/Power.png) |
|:--:| 
| *Power State Data Plot using Ubidots* |

| ![Power State Data Plot using Ubidots](/Power-Plot-CSV.png) |
|:--:| 
| *Power State Data Plot using Ubidots* |
