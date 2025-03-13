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
WiFiServer server(80);
TaskHandle_t ledTask;

bool smartModeInitialized = false;

void initializeDevice() {
  tally::settings::init();
  tally::led::init();
  tally::serial::init();
}

void updateTally() {
  if(switcher) {
    int srcId = tally::settings::query<int>("/tally/srcId").value();
    auto pvw = switcher->onPvw();
    auto pgm = switcher->onPgm();
    bool pgmOn = std::find(pgm.begin(), pgm.end(), srcId) != pgm.end();
    bool pvwOn = std::find(pvw.begin(), pvw.end(), srcId) != pvw.end();

    if(pgmOn) tally::settings::update("/state/tally", 2);
    else if(pvwOn && !pgmOn) tally::settings::update("/state/tally", 1);
    else tally::settings::update("/state/tally", 0);
  }
}

bool setUpWiFi(int maxTries) {
  auto ssid = tally::settings::query<std::string>("/network/wifi/ssid");
  auto pwd = tally::settings::query<std::string>("/network/wifi/pwd");
  auto useDHCP =  tally::settings::query<bool>("/network/wifi/useDHCP");

  if(!ssid || !pwd ||!useDHCP) return false;

  if(ssid.value().length() < 5) {
    tally::serial::Println(F("Warning: SSID not set, entering configuration mode"));
    tally::settings::update("/state/status", "configuration");
    return true;
  }

  tally::settings::update("/state/status", "searching");
  if(!useDHCP.value()) {
    // STRANGE CODE HERE 
    const auto tallyAddress = tally::settings::query<IPAddress>("/network/wifi/address").value();  
  }
  WiFi.mode(WIFI_STA);
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
    if(useDHCP)
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
    //WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 172, 1), IPAddress(192, 168, 172, 1), IPAddress(255, 255, 255, 0));
    if(WiFi.softAP("OneTally")) {
      tally::serial::Println("OneTally AP is ready. WebUi can be found at 192.168.172.1");
    }
    server.begin();
    tally::webui::init(server);
  } else {
    bool obsActive = tally::settings::query<bool>("/target/obs/active").value();
    bool gostreamActive = tally::settings::query<bool>("/target/gostream/active").value();
    if(gostreamActive)
      switcher = (target::Target*) new target::GoStream(tally::settings::query<IPAddress>("/target/gostream/address").value());
    else if(obsActive)
      switcher = (target::Target*) new target::OBS(tally::settings::query<IPAddress>("/target/obs/address").value(), tally::settings::query<int>("/target/obs/port").value());
    else
      tally::serial::Println("Error: multiple active targets, you can only have one target active at a time");
  }
}

void ledWorker(void *pvParameters) {
  while(1) {
    tally::led::show();
    delay(200);
  }
}

void serialWorker(void *pvParameters) {
  while(1) {
    tally::serial::read();
    delay(200);
  }
}

void setup() {
  Serial.begin(921600);
  initializeDevice();
  xTaskCreatePinnedToCore(
    ledWorker, "LED worker", 10000, NULL, 2, &ledTask, 0);
  xTaskCreatePinnedToCore(
    serialWorker, "Serial worker", 10000, NULL, 1, NULL, 0);
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
        if(tally::settings::query<bool>("/tally/smartMode").value() && !smartModeInitialized) {
          delay(500);  // Wait so that we reveice the pvw and pgm status
          switcher->receive();
          auto pvwId = switcher->onPvw();
          if(pvwId.size() > 1)
           tally::serial::Println(F("Error: failed to initialize smart mode, more than one source active on preview"));
          else {
            tally::settings::update("/tally/srcId", pvwId[0]);
            smartModeInitialized = true;
          }
        } 
      } else {
        tally::settings::update("/state/status", "connecting");
      }
    }  else {
      switcher->receive();
    }
  } else if (state == "configuration") {
    tally::webui::checkAndServeConnection();
  }

  updateTally();
}