# TeslaDoorButton

UPDATE: APRIL 2021- Tesla has changed their security method and this will no longer work. The new v3 Authorization is much more complex and very difficult on an ESP8266.

Arduino ESP8266 Project to Lock or Unlock a Tesla via the API

This project uses an ESP8266 with OLED from HELTEC (https://heltec.org/project/wifi-kit-8/) which can be found at many retailers.

The purpose is simple- unlock a Tesla via the API with a single press of the built in PROG button, or lock it if the button is held.

This code also emulates a Philips HUE bulb, which allows you to use Alexa or other voice assistants to lock or unlock.

Enter your WiFi and Tesla credentials at the beginning of the code block. Supports username and password or token, but if you use a
token you'll need to re-compile every 45 days

Implements some messy JSON parsing. Uses ArduinoJSON, U8G2Lib, and FauxmoESP libraries.

![Demo Video](https://github.com/ijord/TeslaDoorButton/blob/master/Demo%20Video.gif)

3D printed Case found here: https://www.thingiverse.com/thing:4153726
