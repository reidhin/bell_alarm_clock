# bell_alarm_clock

This repository contains the code for an alarm clock with bell and display.

## Ingredients

- [ESP8266](https://en.wikipedia.org/wiki/ESP8266) - the brain of the clock;
- [28BYJ-48](https://en.wikipedia.org/wiki/Stepper_motor#/media/File:28BYJ-48_unipolar_stepper_motor_with_ULN2003_driver.jpg) stepper motor to drive the bell;
- [ULN2003A](https://en.wikipedia.org/wiki/ULN2003A) to drive the stepper motor;
- [SSD1306 0.96 inch I2C OLED display](https://wiki.seeedstudio.com/Grove-OLED-Display-0.66-SSD1306_v1.0/) to display the date and time;
- Brass bell;
- Mechano construction to hold the bell;

## Wiring

![schema](https://github.com/user-attachments/assets/d0077fc9-9654-46ff-9b7e-0c5ffb9738fb)

## Instructions

The codes tries to connect to the local Wifi network using the credentials listed in `arduino_secrets.h`. 
This file is not added to the repository; an example on how to structure this file can be seen in `arduino_secrets_example.h`.
The Wifi connection is needed for two reasons:
1. To obtain the internet time;
2. To set different parameters of the alarm clock using a web interface such as the alarm time and turn on/off the hourly strike.

## Code

The code has been split into many (small) functions and classes, inspired by https://m1cr0lab-esp32.github.io/remote-control-with-websocket/.

The files needed for the web interface are stored in a separe folder: `data`. This folder contains a `html`, `css`, and javascript file.
The folder is uploaded to ESP8266 using LittleFS. Use `[Ctrl] + [Shift] + [P]`, then "Upload LittleFS to Pico/ESP8266/ESP32" to upload the folder.
Please note that the serial monitor should be closed during upload- try to restart the Arduino IDE in case of errors.
Details on the use of LittleFS can be found here: https://randomnerdtutorials.com/install-esp8266-nodemcu-littlefs-arduino/

The code uses `AsyncWebServer` which allows serving multiple clients at the same time.
The clients are kept upto date on the states of the clock using websockets. 
The setup of the websockets is heavily inspired by https://m1cr0lab-esp32.github.io/remote-control-with-websocket/.
The creation and reading of json is updated to the more modern `(de)serializeJson()'.

The onboard led is used for a bit of signaling when the LittleFS file system is not mounted correctly at start-up.
During normal runtime, the onboard led turns on/off every 1 second (0.5Hz).

The initialization and usage of the OLED display is based upon https://randomnerdtutorials.com/esp8266-0-96-inch-oled-display-with-arduino-ide/.

The initialization and usage of the stepper motor is based upon https://randomnerdtutorials.com/esp8266-nodemcu-stepper-motor-28byj-48-uln2003/.

## To do

I may want to implement:
- showing the motor status as text on the webinterface;
- provide a (large) turn-off button when the alarm bell is ringing;
- provide a picture/video showing the clock;

*Question:*
A processor function is used to process the index.html file when it is sent to the client. 
In this way a new client gets directly the latest updated status of the led in the html-file.

In this tutorial, https://randomnerdtutorials.com/stepper-motor-esp8266-websocket/, they seem to take a different approach. 
When a new client connects, they initiate a notifyClients command to notify (all) clients of the latest state. 
See code snippet below:

```
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //Notify client of motor current state when it first connects
      notifyClients(direction);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
```

I would say that the advantage of the notifyClients approach is that you do not have to write a (possibly lengthy) processor function. 
The downside is that you also notify clients that are already connected; so in this approach you are creating unnecessary network traffic.

What are your thoughts on the notifyClients approach?


## References

- Tutorial on websockets: https://m1cr0lab-esp32.github.io/remote-control-with-websocket/
- Countless tutorials on: https://randomnerdtutorials.com/projects-esp8266/

## Project structure

```
├── arduino_secrets_example.h     <- example on how to structure arduino_secrets.h
├── arduino_secrets.h             <- file containin Wifi credentials
├── bell_alarm_clock.ino          <- Main arduino code
├── data
│   ├── favicon.png               <- image with favicon
│   ├── index.css                 <- css file to style the html webinterface
│   ├── index.html                <- html file for the webinterface
│   └── index.js                  <- javascript handling the button clicks 
├── LICENSE.md                    <- License
└── README.md                     <- readme file with explanation on this package
```

## License

<a rel="license" href="https://creativecommons.org/publicdomain/zero/1.0/">
<img alt="Creative Commons-Licentie" style="border-width:0" src="https://licensebuttons.net/l/publicdomain/88x31.png" />
</a>
<br />This work is subject to <a rel="license" href="https://creativecommons.org/publicdomain/zero/1.0/">Creative Commons CC0 1.0 Universal</a>.
