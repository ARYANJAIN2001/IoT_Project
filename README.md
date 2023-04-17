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
