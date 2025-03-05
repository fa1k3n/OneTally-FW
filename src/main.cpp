#include <SPI.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include "gostream-comm.hpp"
#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include "tally-led.hpp"
#include "tally-webui.hpp"

#define SSRC_ID 5

WiFiClient client, webclient;
WiFiServer server(80);

void initializeDevice() {
  tally::settings::init();
  tally::led::init();
  tally::serial::init();
}

void syncState() {
  comm::sendMessage("pvwIndex");
  comm::sendMessage("keyOnAir");
  comm::sendMessage("pgmIndex");
  comm::sendMessage("autoTransition");
  comm::sendMessage("superSourceSource1");
  comm::sendMessage("superSourceSource2");
  comm::sendMessage("superSourceBackground");
  comm::sendMessage("pipSource");
  comm::sendMessage("upStreamKeyType");
  comm::sendMessage("transitionSource");
  delay(200);
}

void updateTally() {
  comm::stateT* newState = comm::getState();
  int srcId = tally::settings::query<int>("/srcId").value();

  bool pgmOn = false;
  bool pvwOn = false; 

  if(newState->pgmId == srcId) pgmOn = true;
  if(newState->pvwId == srcId) pvwOn = true;
  if((newState->ssrcSrc1Id == srcId || newState->ssrcSrc2Id == srcId || newState->ssrcBkgId == srcId) && newState->pvwId == SSRC_ID) pvwOn = true;
  if((newState->ssrcSrc1Id == srcId || newState->ssrcSrc2Id == srcId || newState->ssrcBkgId == srcId) && newState->pgmId == SSRC_ID) pgmOn = true;
  if(newState->uskActive && (newState->uskFillSrcId[newState->uskType] == srcId || newState->lumaKeySrcId == srcId)) pgmOn = true;
  if((newState->transitionSource & USK) && (newState->uskFillSrcId[newState->uskType] == srcId || newState->lumaKeySrcId == srcId)) pvwOn = true; 
  if(newState->transitionOngoing && (newState->pvwId == srcId ||newState->pgmId == srcId))   pgmOn = true;

  if(pgmOn) tally::settings::update("/state/tally", 2);
  else if(pvwOn && !pgmOn) tally::settings::update("/state/tally", 1);
  else tally::settings::update("/state/tally", 0);
}

bool setUpWiFi(int maxTries) {
  auto ssid = tally::settings::query<std::string>("/tally/wifi/ssid").value();
  auto pwd = tally::settings::query<std::string>("/tally/wifi/pwd").value();
  auto useDHCP =  tally::settings::query<bool>("/tally/wifi/useDHCP").value();

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
    tally::settings::update("/state/status", "attached");
    if(useDHCP)
      tally::settings::update("/state/dhcpAddress", WiFi.localIP().toString().c_str());
  } else {
    Serial.println(F("ERROR"));
    return false;
  }

  return true;
}

void connectToDevice() {
  comm::init(client);
  auto address = tally::settings::query<IPAddress>("/gostream/address").value();
  auto port = tally::settings::query<uint16_t>("/gostream/port").value();
  comm::connect(address, port, 5);
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
  } else {
    connectToDevice();
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
  if(tally::settings::query<std::string>("/state/status").value().compare("configuration")) {
    syncState();
    if(tally::settings::query<bool>("/smartMode")) {
      comm::receiveAndHandleMessages();
      comm::stateT* newState = comm::getState();
      tally::settings::update("/srcId", newState->pvwId);
    }
    updateTally();
  }
}

void loop() { 
  if (tally::settings::query<std::string>("/state/status") != "configuration" && client.available()) {
    comm::receiveAndHandleMessages();
  }
  tally::serial::read();
  tally::webui::checkAndServeConnection();
  updateTally();

  // Felsök connection avbrott
}
