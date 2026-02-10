#include <SPI.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include "GoStream.hpp"
#include "OBS.hpp"

#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include "tally-led.hpp"
#include "tally-webui.hpp"
#include <SPIFFS.h>
#include <set>

WiFiClient client, webclient;
module::OneTallyModule* switcher = nullptr;

TaskHandle_t ledTask;

bool smartModeInitialized = false;

void initializeDevice() {
  if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
            return;
  }

  tally::settings::init();
  tally::led::init();
  tally::serial::init();
}

void updateTally() {
  if(switcher) {
    JsonArray triggers = tally::settings::query<JsonArray>("/triggers").value();
    bool isUpdated = false;
    auto moduleInfo = switcher->getInfo();
    for(auto trigger : triggers) { 
      auto srcId = trigger["srcId"].as<int>();
      auto event = trigger["event"].as<String>();
      auto colour = std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16);
      auto brightness = trigger["brightness"].as<uint8_t>();

      if(switcher->handleTrigger(trigger)) { 
        auto colour = std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16);
        auto brightness = trigger["brightness"].as<uint8_t>();
        auto pifs = trigger["peripherals"].as<JsonArray>();
        tally::led::show(0, colour, brightness);
        isUpdated = true;
        break;
      }
      if(isUpdated) break;
    }

    if(!isUpdated) tally::led::clear();
  }
      //if(isUpdated) break;

   /* JsonArray allPifs = tally::settings::query<JsonArray>("/peripherals").value();        
    for(auto pifId : allPifs) {
      for(auto trigger : triggers) { 
        auto pifs = trigger["peripheral"].as<JsonArray>();
        if(std::find(pifs.begin(), pifs.end(), pifId) == pifs.end()) {
          // Not active pif
          tally::led::clear(pifId);
          break;
        }
        auto srcId = trigger["srcId"].as<int>();
        auto event = trigger["event"].as<String>();
        auto colour = std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16);
        auto brightness = trigger["brightness"].as<uint8_t>();
        tally::led::show(pifId, colour, brightness);

      }
    }
  }*/
}
   // JsonArray pifs = tally::settings::query<JsonArray>("/peripherals").value();
   // for(auto pif : pifs) {
   //   auto pifId = pif["id"].as<int>();
   //   if(pifsInUse.find(pifId) == pifsInUse.end())
   //     tally::led::clear(pifId);
   // }
  //}

bool setUpWiFi(int maxTries) {
  auto ssid = tally::settings::query<std::string>("/network/wifi/ssid");
  auto pwd = tally::settings::query<std::string>("/network/wifi/pwd");
  auto manualCfg =  tally::settings::query<bool>("/network/wifi/manualCfg");

  if(!ssid || !pwd || !manualCfg) return false;

  if(ssid.value().length() < 5) {
    tally::serial::Println(F("Warning: SSID not set, entering configuration mode"));
    tally::settings::update("/state/status", "configuration");
    return true;
  }

  tally::settings::update("/state/status", "searching");
  if(manualCfg.value()) {
    const auto tallyAddress = tally::settings::query<IPAddress>("/network/wifi/address").value();  
  }
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.begin(ssid.value().c_str(), pwd.value().c_str());

  tally::serial::Print(F("\nConnecting to WiFi AP."));
  int numTries = 0;
  while(WiFi.status() != WL_CONNECTED && numTries ++ < maxTries){
      tally::serial::Print(".");
      delay(1000);
  }
  if(WiFi.status() == WL_CONNECTED) {
    tally::serial::Println(F("OK"));
    tally::settings::update("/state/status", "connecting");
    if(!manualCfg.value())
      tally::settings::update("/state/dhcpAddress", WiFi.localIP().toString().c_str());
  } else {
    tally::serial::Println(F("ERROR"));
    return false;
  }

  return true;
}

void connect() {
  setUpWiFi(5);  
  if(WiFi.status() != WL_CONNECTED) {
    // Setup AP
    tally::settings::update("/state/status", "configuration");
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.softAPConfig(IPAddress(192, 168, 172, 1), IPAddress(192, 168, 172, 1), IPAddress(255, 255, 255, 0));
    //WiFi.setSleep()
   // WiFi.begin();
    if(WiFi.softAP("OneTally")) {
      tally::serial::Print("OneTally AP is ready. WebUi can be found at ");
      tally::serial::Println(WiFi.softAPIP().toString().c_str());
    }
   
  } else {
    switcher = (module::OneTallyModule*) new module::GoStream(tally::settings::query<IPAddress>("/network/targetAddress").value());
  }
   tally::webui::init();
}

void tallyWorker(void *pvParameters) {
  while(1) {
    updateTally();
    //taskYIELD();

    vTaskDelay(200 / portTICK_RATE_MS);//delay(2000);
  }
}

void serialWorker(void *pvParameters) {
  while(1) {
    tally::serial::read();
    taskYIELD();
    //vTaskDelay(200 / portTICK_RATE_MS);//delay(200);
  }
}

void batteryWorker(void *pvParameters) {
  while(1) {
    int digitalValue = analogRead(GPIO_NUM_34);// read the value from the analog channel
    float Aout = digitalValue * (3.20 / (4096.00 - 1.00));
    Serial.println(digitalValue);
    delay(1000);
  }
}
void setup() {
  Serial.begin(115200);
  initializeDevice();
  vTaskSuspendAll();
  xTaskCreatePinnedToCore(
    tallyWorker, "Tally worker", 10000, NULL, 1, &ledTask, 1);
  xTaskCreatePinnedToCore(
    serialWorker, "Serial worker", 10000, NULL, 1, NULL, 1);
  xTaskResumeAll();
  //xTaskCreatePinnedToCore(
  //  batteryWorker, "Battery worker", 10000, NULL, 1, NULL, 0);
  connect();

}

void restart() {
  // Notify pending shutdown
  tally::serial::Println("Info: tally is preparing to restart");

  // Stop switcher
  if(switcher && switcher->started()) switcher->stop();

  // clear leds
  tally::led::clear();

  delay(300);
  tally::serial::Println("Info: tally is restarting");

  ESP.restart();
}

// the setup function runs once when you press reset or power the board
#ifdef RGB_BUILTIN
#undef RGB_BUILTIN
#endif
#define RGB_BUILTIN 21

void loop() { 

//  if(!tally::settings::query<std::string>("/state/status")) return;

 // auto state = tally::settings::query<std::string>("/state/status").value();
  // Check that we are connected 
  if(switcher) {  
    if(!switcher->started()) { //} && state != "configuration") {
      if(switcher->start(&client)) {
        tally::settings::update("/state/status", "connected");
      } else {
        tally::settings::update("/state/status", "connecting");
      } 
    } else {
      //updateTally();
      //vTaskDelay(200 / portTICK_RATE_MS);
      //GP
    }
  }
  vTaskDelay(200 / portTICK_RATE_MS);
}

/*
  BlinkRGB

  Demonstrates usage of onboard RGB LED on some ESP dev boards.

  Calling digitalWrite(RGB_BUILTIN, HIGH) will use hidden RGB driver.
    
  RGBLedWrite demonstrates controll of each channel:
  void neopixelWrite(uint8_t pin, uint8_t red_val, uint8_t green_val, uint8_t blue_val)

  WARNING: After using digitalWrite to drive RGB LED it will be impossible to drive the same pin
    with normal HIGH/LOW level
*/
//#include <Adafruit_NeoPixel.h>
/*#include <esp32-hal-rgb-led.h>
#include <esp32-hal.h>

//#define RGB_BRIGHTNESS 10 // Change white brightness (max 255)

// the setup function runs once when you press reset or power the board
#ifdef RGB_BUILTIN
#undef RGB_BUILTIN
#endif
#define RGB_BUILTIN 21

void setup() {
  // No need to initialize the RGB LED
}

// the loop function runs over and over again forever
void loop() {
#ifdef RGB_BUILTIN
  // digitalWrite(RGB_BUILTIN, HIGH);   // Turn the RGB LED white
  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,RGB_BRIGHTNESS,RGB_BRIGHTNESS); // Red
  delay(1000);
  // digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off
  neopixelWrite(RGB_BUILTIN,0,0,0); // Red
  delay(1000);

  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0,0); // Red
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,0); // Green
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,0,RGB_BRIGHTNESS); // Blue
  delay(1000);
  neopixelWrite(RGB_BUILTIN,0,0,0); // Off / black
  delay(1000);
#endif
}
*/