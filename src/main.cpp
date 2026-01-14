#include <SPI.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include "GoStream.hpp"
#include "OBS.hpp"

#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include "tally-led.hpp"
#include "tally-webui.hpp"

WiFiClient client, webclient;
target::Target* switcher = nullptr;

TaskHandle_t ledTask;

bool smartModeInitialized = false;

void initializeDevice() {
  tally::settings::init();
  tally::led::init();
  tally::serial::init();
}

void updateTally() {
  if(switcher) {
    JsonArray triggers = tally::settings::query<JsonArray>("/triggers").value();
    JsonArray pifs = tally::settings::query<JsonArray>("/peripherals").value();
    auto state = tally::settings::query<std::string>("/state/status").value();
    bool isUpdated = false;
    for(auto pif : pifs) {
      for(auto trigger : triggers) { 
        auto srcId = trigger["srcId"].as<int>();
        auto event = trigger["event"].as<String>();
        auto pifId = trigger["peripheral"].as<int>();
        if(pifId != pif["id"].as<int>()) continue;  // This trigger is not for current pif

        auto colour = std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16);
        auto brightness = trigger["brightness"].as<uint8_t>();

        if(switcher->handleTrigger(trigger)) {
          isUpdated = true;
        } else if(event == String(state.c_str())) {
            tally::led::show(pifId, colour, brightness);
            isUpdated = true;
            break;
        } 
      }
    }
    if(!isUpdated)
      tally::led::clear();
  }
}

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
    switcher = (target::Target*) new target::GoStream(tally::settings::query<IPAddress>("/network/targetAddress").value());
    /*bool obsActive = tally::settings::query<bool>("/target/obs/active").value();
    bool gostreamActive = tally::settings::query<bool>("/target/gostream/active").value();
    if(gostreamActive)
      switcher = (target::Target*) new target::GoStream(tally::settings::query<IPAddress>("/target/gostream/address").value());
    else if(obsActive)
      switcher = (target::Target*) new target::OBS(tally::settings::query<IPAddress>("/target/obs/address").value(), tally::settings::query<int>("/target/obs/port").value());
    else
      tally::serial::Println("Error: multiple active targets, you can only have one target active at a time");*/
  }
   tally::webui::init();
}

void ledWorker(void *pvParameters) {
  while(1) {
    //tally::led::show();
    updateTally();
    delay(2000);
  }
}

void serialWorker(void *pvParameters) {
  while(1) {
    tally::serial::read();
    delay(200);
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
  Serial.begin(921600);
  initializeDevice();
  xTaskCreatePinnedToCore(
    ledWorker, "LED worker", 10000, NULL, 2, &ledTask, 0);
  xTaskCreatePinnedToCore(
    serialWorker, "Serial worker", 10000, NULL, 1, NULL, 0);
  //xTaskCreatePinnedToCore(
  //  batteryWorker, "Battery worker", 10000, NULL, 1, NULL, 0);
  connect();

}

void restart() {
  // Notify pending shutdown
  tally::serial::Println("Info: tally is preparing to restart");

  // Disconnect from switcher
  if(switcher && switcher->connected()) switcher->disconnect();

  // clear leds
  tally::led::clear();

  delay(300);
  tally::serial::Println("Info: tally is restarting");

  ESP.restart();
}

void loop() { 

  if(!tally::settings::query<std::string>("/state/status")) return;

  auto state = tally::settings::query<std::string>("/state/status").value();
  // Check that we are connected 
  if(switcher) {  
    if(!switcher->connected() && state != "configuration") {
      if(switcher->connect(&client)) {
        tally::settings::update("/state/status", "connected");
      } else {
        tally::settings::update("/state/status", "connecting");
      } 
    } else {
      if(switcher->receive())
          updateTally();
    }
  }
}