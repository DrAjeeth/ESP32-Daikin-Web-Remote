// ======================================================
// SMART AC CONTROLLER
// ESP32 + BMP280 + DAIKIN + WEB UI
// ======================================================

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <time.h>

#include <Wire.h>
#include <Adafruit_BMP280.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Daikin.h>

// ======================================================
// WIFI
// ======================================================

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ======================================================
// NTP
// ======================================================

const char* ntpServer = "pool.ntp.org";

const long gmtOffset_sec =19800;

const int daylightOffset_sec = 0;

// ======================================================
// PINS
// ======================================================

#define IR_LED_PIN 4

// ======================================================
// OBJECTS
// ======================================================

AsyncWebServer server(80);

Adafruit_BMP280 bmp;

IRDaikinESP ac(IR_LED_PIN);

// ======================================================
// SENSOR DATA
// ======================================================

float temperature;
float pressure;

// ======================================================
// AC STATE
// ======================================================

bool acPower = false;

int setTemp = 28;

uint8_t fanSpeed = kDaikinFanAuto;

uint8_t acMode = kDaikinCool;

// ======================================================
// AUTO OFF TIMER
// ======================================================

bool autoOffEnabled = false;

unsigned long autoOffStart = 0;

unsigned long autoOffDuration = 0;

// ======================================================
// AUTO MODE
// ======================================================

bool autoMode = false;

float autoOnTemp = 30.0;

float autoOffTemp = 27.0;

// ======================================================
// ADVANCED SLEEP MODE
// ======================================================

bool sleepMode = false;

unsigned long sleepStart = 0;

unsigned long lastSleepRamp = 0;

int originalSleepTemp = 24;

// increase temp every X hours

int sleepRampHours = 1;

// max temp before OFF

int sleepMaxTemp = 31;

// ======================================================
// CYCLICAL TIMER
// ======================================================

bool cycleMode = false;

bool cycleStateOn = true;

unsigned long cycleStart = 0;

unsigned long cycleOnDuration = 30 * 60000UL;

unsigned long cycleOffDuration = 30 * 60000UL;

int totalCycles = 5;

int completedCycles = 0;

// ======================================================
// SCHEDULER
// ======================================================

bool schedulerEnabled = false;

int onHour = 22;
int onMinute = 0;

int offHour = 6;
int offMinute = 0;

// ======================================================
// SPECIAL FEATURES
// ======================================================

bool powerfulMode = false;

bool econoMode = false;

bool quietMode = false;

bool swingVertical = false;

bool swingHorizontal = false;

bool coandaMode = false;

// ======================================================
// SEND AC
// ======================================================

void sendAC() {

  ac.on();

  ac.setMode(acMode);

  ac.setTemp(setTemp);

  uint8_t effectiveFanSpeed =
  quietMode ? kDaikinFanQuiet : fanSpeed;

  ac.setFan(effectiveFanSpeed);

// ======================================================
// SPECIAL MODES
// ======================================================

// POWERFUL

ac.setPowerful(powerfulMode);

// ECONO

ac.setEcono(econoMode);

// QUIET

ac.setQuiet(quietMode);

// SWING VERTICAL

ac.setSwingVertical(swingVertical);

// SWING HORIZONTAL

ac.setSwingHorizontal(swingHorizontal);

// COANDA / COMFORT

ac.setComfort(coandaMode);

ac.send();

acPower = true;

Serial.println("AC COMMAND SENT");
}

// ======================================================
// TURN OFF AC
// ======================================================

void turnOffAC() {

  ac.off();

  ac.send();

  acPower = false;

Serial.println("AC OFF");
}

// ======================================================
// AUTO MODE CHECK
// ======================================================

void runAutoMode() {

  if(!autoMode){

    return;
  }

  if(!acPower && temperature >= autoOnTemp){

    sendAC();

    Serial.print("AUTO MODE AC ON, TEMP: ");
    Serial.println(temperature);
  }

  else if(acPower && temperature <= autoOffTemp){

    turnOffAC();

    Serial.print("AUTO MODE AC OFF, TEMP: ");
    Serial.println(temperature);
  }
}

String currentTime(){

  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){

    return "TIME ERROR";
  }

  char timeString[20];

  strftime(timeString, sizeof(timeString), "%I:%M %p", &timeinfo);

  return String(timeString);
}

// ======================================================
// JSON DATA
// ======================================================

String jsonData() {

  String json = "{";

  json += "\"temperature\":";
  json += String(temperature,1);
  json += ",";

  json += "\"pressure\":";
  json += String(pressure,1);
  json += ",";

  json += "\"setTemp\":";
  json += String(setTemp);
  json += ",";

  json += "\"fanSpeed\":";
  json += String(fanSpeed);
  json += ",";

  json += "\"acMode\":";
  json += String(acMode);
  json += ",";

  json += "\"acPower\":";
  json += acPower ? "true" : "false";

  json += ",\"sleepMode\":";
  json += sleepMode ? "true" : "false";

  json += ",\"sleepRampHours\":";
  json += String(sleepRampHours);

  json += ",\"sleepMaxTemp\":";
  json += String(sleepMaxTemp);

  json += ",\"autoOffEnabled\":";
  json += autoOffEnabled ? "true" : "false";

  json += ",\"autoMode\":";
  json += autoMode ? "true" : "false";

  json += ",\"autoOnTemp\":";
  json += String(autoOnTemp,1);

  json += ",\"autoOffTemp\":";
  json += String(autoOffTemp,1);

  json += ",\"cycleMode\":";
  json += cycleMode ? "true" : "false";

  json += ",\"completedCycles\":";
  json += String(completedCycles);

  json += ",\"totalCycles\":";
  json += String(totalCycles);

  json += ",\"powerfulMode\":";
  json += powerfulMode ? "true" : "false";

  json += ",\"econoMode\":";
  json += econoMode ? "true" : "false";

  json += ",\"quietMode\":";
  json += quietMode ? "true" : "false";

  json += ",\"swingVertical\":";
  json += swingVertical ? "true" : "false";

  json += ",\"swingHorizontal\":";
  json += swingHorizontal ? "true" : "false";

  json += ",\"coandaMode\":";
  json += coandaMode ? "true" : "false";

  json += ",\"time\":\"";
  json += currentTime();
  json += "\"";

  json += "}";

  return json;
}

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  // ======================================================
  // LITTLEFS
  // ======================================================

  if(!LittleFS.begin(true)) {

    Serial.println("LittleFS Error");

    return;
  }

  // DEBUG FILES

  File root = LittleFS.open("/");

  File file = root.openNextFile();

  while(file){

    Serial.print("FILE: ");

    Serial.println(file.name());

    file = root.openNextFile();
  }

  // ======================================================
  // BMP280
  // ======================================================

  Wire.begin(8, 9);

  if (!bmp.begin(0x76)) {

    Serial.println("BMP280 ERROR");

    while(1);
  }

  // ======================================================
  // IR
  // ======================================================

  ac.begin();

  // ======================================================
  // WIFI
  // ======================================================

  WiFi.begin(ssid,password);

  Serial.print("Connecting");

  while(WiFi.status()!=WL_CONNECTED){

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(WiFi.localIP());

// ======================================================
// NTP TIME
// ======================================================

configTime(
gmtOffset_sec,
daylightOffset_sec,
ntpServer);

Serial.println("NTP TIME STARTED");

  // ======================================================
  // STATIC FILES
  // ======================================================

  server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html");

  // ======================================================
  // DATA
  // ======================================================

  server.on("/data", HTTP_GET,
  [](AsyncWebServerRequest *request){

    request->send(200,
    "application/json",
    jsonData());
  });

  // ======================================================
  // AC ON
  // ======================================================

  server.on("/on", HTTP_GET,
  [](AsyncWebServerRequest *request){

    sendAC();

    request->send(200,
    "text/plain",
    "ON");
  });

  // ======================================================
  // AC OFF
  // ======================================================

  server.on("/off", HTTP_GET,
  [](AsyncWebServerRequest *request){

    turnOffAC();

    request->send(200,
    "text/plain",
    "OFF");
  });

  // ======================================================
  // TEMP UP
  // ======================================================

  server.on("/tempup", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(setTemp < 30){

      setTemp++;

      sendAC();
    }

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // TEMP DOWN
  // ======================================================

  server.on("/tempdown", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(setTemp > 16){

      setTemp--;

      sendAC();
    }

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // FAN
  // ======================================================

  server.on("/fan", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(request->hasParam("speed")){

      String s =
      request->getParam("speed")->value();

      if(s == "auto"){        fanSpeed = kDaikinFanAuto;
      }

      else if(s == "1"){        fanSpeed = kDaikinFanMin;
      }

      else if(s == "2"){        fanSpeed = 2;
      }

      else if(s == "3"){        fanSpeed = 3;
      }

      else if(s == "4"){        fanSpeed = 4;
      }

      else if(s == "5"){        fanSpeed = kDaikinFanMax;
      }

      quietMode = false;

      sendAC();
    }

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // MODE
  // ======================================================

  server.on("/mode", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(request->hasParam("type")){

      String mode =
      request->getParam("type")->value();

      if(mode == "cool"){

        acMode = kDaikinCool;
      }

      else if(mode == "fan"){

        acMode = kDaikinFan;
      }

      else if(mode == "dry"){

        acMode = kDaikinDry;
      }

      sendAC();
    }

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // AUTO OFF TIMER
  // ======================================================

  server.on("/autoff", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(request->hasParam("minutes")){

      int mins =
      request->getParam("minutes")->value().toInt();

      autoOffEnabled = true;

      autoOffStart = millis();

      autoOffDuration =
      mins * 60000UL;

      Serial.println("AUTO OFF STARTED");
    }

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // AUTO MODE
  // ======================================================

  server.on("/automode", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(request->hasParam("enable")){

      String en =
      request->getParam("enable")->value();

      autoMode = (en == "1");

      if(autoMode){

        cycleMode = false;

        autoOffEnabled = false;

        Serial.println("AUTO MODE ON");

      } else {

        Serial.println("AUTO MODE OFF");
      }
    }

    if(request->hasParam("on")){

      autoOnTemp =
      request->getParam("on")->value().toFloat();
    }

    if(request->hasParam("off")){

      autoOffTemp =
      request->getParam("off")->value().toFloat();
    }

    if(autoOffTemp >= autoOnTemp){

      autoOffTemp = autoOnTemp - 1.0;
    }

    temperature = bmp.readTemperature();

    runAutoMode();

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // ADVANCED SLEEP MODE
  // ======================================================

  server.on("/sleep", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(request->hasParam("enable")){

      String en =
      request->getParam("enable")->value();

      sleepMode = (en == "1");

      if(sleepMode){

        sleepStart = millis();

        lastSleepRamp = millis();

        originalSleepTemp = setTemp;

        Serial.println("SLEEP MODE ON");

      } else {

        Serial.println("SLEEP MODE OFF");
      }
    }

    if(request->hasParam("hours")){

      sleepRampHours =
      request->getParam("hours")->value().toInt();
    }

    if(request->hasParam("max")){

      sleepMaxTemp =
      request->getParam("max")->value().toInt();
    }

    request->send(200,
    "text/plain",
    "OK");
  });

  // ======================================================
  // CYCLICAL TIMER
  // ======================================================

  server.on("/cycle", HTTP_GET,
  [](AsyncWebServerRequest *request){

    if(request->hasParam("enable")){

      String en =
      request->getParam("enable")->value();

      cycleMode = (en == "1");

      cycleStart = millis();

      completedCycles = 0;

      cycleStateOn = acPower;

      Serial.println("CYCLE MODE CHANGED");
    }

    if(request->hasParam("on")){

      int mins =
      request->getParam("on")->value().toInt();

      cycleOnDuration =
      mins * 60000UL;
    }

    if(request->hasParam("off")){

      int mins =
      request->getParam("off")->value().toInt();

      cycleOffDuration =
      mins * 60000UL;
    }

    if(request->hasParam("count")){

      totalCycles =
      request->getParam("count")->value().toInt();
    }

    request->send(200,
    "text/plain",
    "OK");
  });

// ======================================================
// POWERFUL
// ======================================================

server.on("/powerful", HTTP_GET,
[](AsyncWebServerRequest *request){

  powerfulMode = !powerfulMode;

  if(powerfulMode){

    econoMode = false;

    coandaMode = false;

    swingVertical = true;
  }

  sendAC();

  request->send(200,"text/plain","OK");
});

// ======================================================
// ECONO
// ======================================================

server.on("/econo", HTTP_GET,
[](AsyncWebServerRequest *request){

  econoMode = !econoMode;

  if(econoMode){

    powerfulMode = false;
  }

  sendAC();

  request->send(200,"text/plain","OK");
});

// ======================================================
// QUIET
// ======================================================

server.on("/quiet", HTTP_GET,
[](AsyncWebServerRequest *request){

  quietMode = !quietMode;

  sendAC();

  request->send(200,"text/plain","OK");
});

// ======================================================
// SWING VERTICAL
// ======================================================

server.on("/swingv", HTTP_GET,
[](AsyncWebServerRequest *request){

  swingVertical = !swingVertical;

  if(swingVertical){

    coandaMode = false;
  }

  sendAC();

  request->send(200,"text/plain","OK");
});

// ======================================================
// SWING HORIZONTAL
// ======================================================

server.on("/swingh", HTTP_GET,
[](AsyncWebServerRequest *request){

  swingHorizontal = !swingHorizontal;

  sendAC();

  request->send(200,"text/plain","OK");
});

// ======================================================
// COANDA / COMFORT
// ======================================================

server.on("/coanda", HTTP_GET,
[](AsyncWebServerRequest *request){

  coandaMode = !coandaMode;

  if(coandaMode){

    swingVertical = false;
  }

  sendAC();

  request->send(200,"text/plain","OK");
});

  server.begin();
}

// ======================================================
// LOOP
// ======================================================

void loop() {

  temperature = bmp.readTemperature();

  pressure = bmp.readPressure()/100.0;

  // ======================================================
  // AUTO MODE LOGIC
  // ======================================================

  runAutoMode();

  // ======================================================
  // AUTO OFF TIMER
  // ======================================================

  if(autoOffEnabled){

    if(millis() - autoOffStart
    >= autoOffDuration){

      turnOffAC();

      autoOffEnabled = false;

      Serial.println("AUTO OFF EXECUTED");
    }
  }

  // ======================================================
  // ADVANCED SLEEP MODE
  // ======================================================

  if(sleepMode){

    if(millis() - lastSleepRamp >=
       sleepRampHours * 3600000UL){

      lastSleepRamp = millis();

      setTemp++;

      Serial.print("SLEEP TEMP -> ");
      Serial.println(setTemp);

      if(setTemp >= sleepMaxTemp){

        turnOffAC();

        sleepMode = false;

        Serial.println("SLEEP COMPLETE");

      } else {

        sendAC();
      }
    }
  }

  // ======================================================
  // CYCLICAL TIMER LOGIC
  // ======================================================

  if(cycleMode){

    unsigned long elapsed =
    millis() - cycleStart;

    // CURRENTLY ON

    if(cycleStateOn){

      if(elapsed >= cycleOnDuration){

        turnOffAC();

        cycleStateOn = false;

        cycleStart = millis();

        completedCycles++;

        Serial.println("CYCLE OFF");
      }
    }

    // CURRENTLY OFF

    else {

      if(elapsed >= cycleOffDuration){

        sendAC();

        cycleStateOn = true;

        cycleStart = millis();

        Serial.println("CYCLE ON");
      }
    }

    // STOP AFTER TARGET CYCLES

    if(completedCycles >= totalCycles){

      cycleMode = false;

      Serial.println("CYCLE COMPLETE");
    }
  }

  delay(1000);
}
