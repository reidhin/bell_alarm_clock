/**
 * ----------------------------------------------------------------------------
 * Bell Alarm Clock
 * ----------------------------------------------------------------------------
 * © 2024 Hans Weda
 * ----------------------------------------------------------------------------
 * NotifyClients when connecting instead of processor with variables.
 * https://www.instructables.com/ESP8266-Two-Wheel-Robot-NodeMCU-and-Stepper-Motor/
 */

// include secrets to connect to wifi 
#include "arduino_secrets.h"

// File system
#include <LittleFS.h>

// WiFi and webserver stuff
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// motor control
#include <AccelStepper.h>

// screen control
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif24pt7b.h>

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

// Port for webserver
#define HTTP_PORT 80

// OLED display width and height
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

/* Configuration of NTP */
#define MY_NTP_SERVER "pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"  

// ULN2003 Motor Driver Pins
#define IN1 0
#define IN2 14
#define IN3 12
#define IN4 13

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
enum status{STOPPED, START_RUNNING, RUNNING, START_STRIKING, STRIKING, STOPPING}; // status of the motor driving the bell

// ----------------------------------------------------------------------------
// Definition of the Led component
// ----------------------------------------------------------------------------

struct Led {
  // state variables
  uint8_t pin;
  bool    on;

  // methods
  void update() {
    digitalWrite(pin, on ? HIGH : LOW);
  }
};

// ----------------------------------------------------------------------------
// Definition of the Alarm component
// ----------------------------------------------------------------------------

struct Alarm {
  // state variables
  bool alarmButtonOn;
  String alarmTime;
};


// ----------------------------------------------------------------------------
// Definition of the Motor component
// ----------------------------------------------------------------------------

struct Motor {
  // state variables
  status motorStatus;
  AccelStepper stepper;

  // methods
  void initialize() {
    // set the speed and acceleration
    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(2000);
    
    // set neutral position
    stepper.moveTo(0);
    stepper.runToPosition();

    Serial.println("Motor initialized");
  }

  void update() {
    // update of status
    if (motorStatus == START_RUNNING) {
      stepper.moveTo(200);
      motorStatus = RUNNING;
      Serial.println("Start running");
    }
    if (motorStatus == START_STRIKING) {
      stepper.moveTo(250);
      motorStatus = STRIKING;
      Serial.println("Start striking");
    }
    if (motorStatus == STOPPING) {
      if (stepper.targetPosition() != 0) {
        stepper.moveTo(0);
        Serial.println("Start stopping");
      }
    }

    if (motorStatus != STOPPED) {
      if (stepper.distanceToGo() == 0) {
        if (motorStatus == STOPPING) {
          motorStatus = STOPPED;
          Serial.println("Stopped");
          digitalWrite(IN1, LOW);  // Turn off led's on ULN2003 Stepper Motor Driver
        } 
        if (motorStatus == RUNNING) {
          stepper.moveTo(-stepper.currentPosition());
          Serial.println("Changing direction");
        }
        if (motorStatus == STRIKING) {
          motorStatus = STOPPING;
          Serial.println("Striked");
        }
      }
      stepper.run();
    }
  }
};


// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led onboard_led = { LED_BUILTIN, false };  // LED_BUILTIN is a variable arduino

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// initialize time and alarm
time_t now;                         // this are the seconds since Epoch (1970) - UTC
struct tm tm;                       // the structure tm holds time information in a more convenient way

Alarm alarm = { false, "" };

// initialize the stepper library
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

// initialize motor
Motor bell_motor = {STOPPED, stepper};

status current_motor_status = STOPPED;

// initialize hourly strike
bool hourlyStrikeButtonOn = true;

// ----------------------------------------------------------------------------
// LittleFS initialization
// ----------------------------------------------------------------------------

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("Cannot mount LittleFS volume...");
    while (1) {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
  }
}


// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
  
  //The ESP8266 tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}


// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
  if (var == "MotorCheckboxPlaceholder") {
    String html_out = "";
    if (bell_motor.motorStatus == STOPPED || bell_motor.motorStatus == STOPPING) {
      html_out += "<label class='switch'><input type='checkbox' id='motor'><span class='slider round'></span></label>";
    } else {
      html_out += "<label class='switch'><input type='checkbox' id='motor' checked><span class='slider round'></span></label>";
    }
    return html_out;
  };
  if (var == "UurslagCheckboxPlaceholder") {
    String html_out = "";
    if (hourlyStrikeButtonOn) {
      html_out += "<label class='switch'><input type='checkbox' id='hourly_strike' checked><span class='slider round'></span></label>";
    } else {
      html_out += "<label class='switch'><input type='checkbox' id='hourly_strike'><span class='slider round'></span></label>";
    }
    return html_out;
  };
  if (var == "AlarmCheckboxPlaceholder") {
    String html_out = "";
    if (alarm.alarmButtonOn) {
      html_out += "<label class='switch'><input type='checkbox' id='alarm' checked><span class='slider round'></span></label>";
    } else {
      html_out += "<label class='switch'><input type='checkbox' id='alarm'><span class='slider round'></span></label>";
    }
    return html_out;
  };
  if (var == "AlarmTime") {
    return alarm.alarmTime;
  };
  if (var == "MotorStatus") {
    switch (bell_motor.motorStatus) {
      case RUNNING: return "<i class='fa fa-bell running_bell' id='bell'></i>"; break;
      case STOPPING: return "<i class='fa fa-bell stopping_bell' id='bell'></i>"; break;
      case STOPPED: return "<i class='fa fa-bell stopped_bell' id='bell'></i>"; break;
      case STRIKING: return "<i class='fa fa-bell striking_bell' id='bell'></i>"; break;
    }
  }
  return String();
}

void onRootRequest(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html", false, processor);
}

void initWebServer() {
  server.on("/", onRootRequest);
  server.serveStatic("/", LittleFS, "/");
  server.begin();
  Serial.println("HTTP Server Started");
}


// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients() {
  JsonDocument json;
  json["motorStatus"] = bell_motor.motorStatus;
  json["hourlyStrikeButtonOn"] = hourlyStrikeButtonOn;
  json["alarmButtonOn"] = alarm.alarmButtonOn;
  json["alarmTime"] = alarm.alarmTime.c_str();
  
  char data[100];
  size_t len = serializeJson(json, data);
  ws.textAll(data, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

    JsonDocument json;
    DeserializationError err = deserializeJson(json, data);
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
      return;
    }

    bell_motor.motorStatus = json["motorStatus"];
    alarm.alarmButtonOn = json["alarmButtonOn"];
    hourlyStrikeButtonOn = json["hourlyStrikeButtonOn"];
    alarm.alarmTime = json["alarmTime"].as<String>();

    notifyClients();
  }
}

void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


// ----------------------------------------------------------------------------
// OLED display initialization
// ----------------------------------------------------------------------------

void initDisplay() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  display.setRotation(2);

  Serial.println("Display initialized");
}


// ----------------------------------------------------------------------------
// OLED display update
// ----------------------------------------------------------------------------

void updateDisplay(struct tm &tm) {
  char date_char[100];
  char time_char[10];
  
  strftime(date_char, 100, "%a, %e %b %Y", &tm);
  strftime(time_char, 10, "%H:%M", &tm);
  //Serial.println(now);
  //Serial.println(date_char);
  //Serial.println(time_char);
  //Serial.println();

  display.dim((tm.tm_hour > 20 || tm.tm_hour < 7)); 

  // Display static text
  display.clearDisplay();
  display.setCursor(0, 6);
  // display.setFont(&FreeSerif9pt7b);
  display.setFont();
  display.setTextSize(1);
  display.println(date_char);
  display.setCursor(0, 56);
  display.setFont(&FreeSerif24pt7b);
  // display.setTextSize(4);
  display.println(time_char);
  display.display();
}


// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  pinMode(onboard_led.pin, OUTPUT);

  Serial.begin(115200); delay(500);

  initLittleFS();
  initWiFi();
  initWebServer();
  initWebSocket();
  initDisplay();
  bell_motor.initialize();

  configTime(MY_TZ, MY_NTP_SERVER); // --> Here is the IMPORTANT ONE LINER needed in your sketch!
}


// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop() {
  // put your main code here, to run repeatedly:
  ws.cleanupClients();
  
  // flash the onboard led
  onboard_led.on = millis() % 2000 < 1000;
  onboard_led.update();

  // update time
  if (!(millis() % 2000)) {
    time(&now);                       // read the current time
    localtime_r(&now, &tm);
  }

  // update display every 2 seconds
  if (!(millis() % 2000)) updateDisplay(tm);

  // update motor
  bell_motor.update();

  // notify clients if the motor status has changed
  if (bell_motor.motorStatus != current_motor_status) {
    notifyClients();
    current_motor_status = bell_motor.motorStatus;
  }

  // check if alarm is triggered every 1 seconds
  if (alarm.alarmButtonOn && !(millis() % 1000)) {
    // alarm-button is on
    if (alarm.alarmTime.substring(0, 2).toInt() == tm.tm_hour) {
      // same hour
      if (alarm.alarmTime.substring(3).toInt() == tm.tm_min) {
        // same minute
        if (tm.tm_sec < 3) {
          // turn motor on (only in first three seconds)
          if (bell_motor.motorStatus != RUNNING) {
            bell_motor.motorStatus = START_RUNNING;
          }
        }
      }
    }
  }
  
  // check if hourly strike is triggered every 1 seconds
  if (hourlyStrikeButtonOn && !(millis() % 1000)) {
    // hourly strike button is on
    if (tm.tm_sec < 1) {
      // every 30 minutes
      if (bell_motor.motorStatus != STRIKING) {
        bell_motor.motorStatus = START_STRIKING;
      }
    }
  }
}
