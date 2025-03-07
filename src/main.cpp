#include <SPI.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include "GoStream.hpp"

#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include "tally-led.hpp"
#include "tally-webui.hpp"

WiFiClient client, webclient;
target::GoStream switcher = target::GoStream(client);
WiFiServer server(80);

bool smartModeInitialized = false;

void initializeDevice() {
  tally::settings::init();
  tally::led::init();
  tally::serial::init();
}

void updateTally() {
  int srcId = tally::settings::query<int>("/srcId").value();
  auto pvw = switcher.onPvw();
  auto pgm = switcher.onPgm();
  bool pgmOn = std::find(pgm.begin(), pgm.end(), srcId) != pgm.end();
  bool pvwOn = std::find(pvw.begin(), pvw.end(), srcId) != pvw.end();

  if(pgmOn) tally::settings::update("/state/tally", 2);
  else if(pvwOn && !pgmOn) tally::settings::update("/state/tally", 1);
  else tally::settings::update("/state/tally", 0);
}

bool setUpWiFi(int maxTries) {
  auto ssid = tally::settings::query<std::string>("/tally/wifi/ssid").value();
  auto pwd = tally::settings::query<std::string>("/tally/wifi/pwd").value();
  auto useDHCP =  tally::settings::query<bool>("/tally/wifi/useDHCP").value();

  if(ssid.length() < 5) {
    Serial.println(F("Warning: SSID not set, entering configurtaion mode"));
    tally::settings::update("/state/status", "configuration");
    return true;
  }

  tally::settings::update("/state/status", "searching");
  if(!useDHCP) {
    const auto tallyAddress = tally::settings::query<IPAddress>("/tally/wifi/address").value();
    const auto tallyGateway = tally::settings::query<IPAddress>("/tally/wifi/gateway").value();
    const auto tallyNetmask = tally::settings::query<IPAddress>("/tally/wifi/netmask").value();
    WiFi.config(tallyAddress, tallyGateway, tallyNetmask);
  }
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pwd.c_str());

  Serial.print(F("Connecting to WiFi AP."));
  int numTries = 0;
  while(WiFi.status() != WL_CONNECTED && numTries ++ < maxTries){
      Serial.print(".");
      delay(1000);
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println(F("OK"));
    tally::settings::update("/state/status", "connecting");
    if(useDHCP)
      tally::settings::update("/state/dhcpAddress", WiFi.localIP().toString().c_str());
  } else {
    Serial.println(F("ERROR"));
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
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("GoTally");
  } 
  server.begin();
  tally::webui::init(server);
}

void ledWorker(void *pvParameters) {
  tally::led::init();
  while(1) {
    tally::led::show();
  }
}
TaskHandle_t ledTask;

void setup() {
  Serial.begin(921600);
  initializeDevice();
  xTaskCreatePinnedToCore(
    ledWorker, "LED worker", 10000, NULL, 1, &ledTask, 0);
  connect();
}

void loop() { 
  auto state = tally::settings::query<std::string>("/state/status").value();
  // Check that we are connected 
  if(!switcher.connected() && state == "connecting") {
    if(switcher.connect(tally::settings::query<IPAddress>("/gostream/address").value())) {
      tally::settings::update("/state/status", "connected");
      if(tally::settings::query<bool>("/smartMode") && !smartModeInitialized) {
        switcher.receiveAndHandleMessages();
        auto pvwId = switcher.onPvw();
        if(pvwId.size() > 1)
          Serial.println(F("Error initializing smart mode, more than one source active on preview"));
        else {
          tally::settings::update("/srcId", pvwId[0]);
          smartModeInitialized = true;
        }
      } 
    } else {
      tally::settings::update("/state/status", "connecting");
    }
  } else if (state == "configuration") {
      tally::webui::checkAndServeConnection();
  } else {
    switcher.receiveAndHandleMessages();
  }

  tally::serial::read();
  updateTally();
}
