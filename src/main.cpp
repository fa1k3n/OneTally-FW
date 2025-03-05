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
}

void updateTally() {
  tally::led::clear();
  comm::stateT* newState = comm::getState();
  const auto srcId = tally::settings::query<uint8_t>("/srcId");

  bool pgmOn = false;
  bool pvwOn = false; 

  if(newState->pgmId == srcId) pgmOn = true;
  if(newState->pvwId == srcId) pvwOn = true;
  if((newState->ssrcSrc1Id == srcId || newState->ssrcSrc2Id == srcId || newState->ssrcBkgId == srcId) && newState->pvwId == SSRC_ID) pvwOn = true;
  if((newState->ssrcSrc1Id == srcId || newState->ssrcSrc2Id == srcId || newState->ssrcBkgId == srcId) && newState->pgmId == SSRC_ID) pgmOn = true;
  if(newState->uskActive && (newState->uskFillSrcId[newState->uskType] == srcId || newState->lumaKeySrcId == srcId)) pgmOn = true;
  if((newState->transitionSource & USK) && (newState->uskFillSrcId[newState->uskType] == srcId || newState->lumaKeySrcId == srcId)) pvwOn = true; 
  if(newState->transitionOngoing && (newState->pvwId == srcId ||newState->pgmId == srcId))   pgmOn = true;

  // Light the candles, if both PGM and PWV are active then only light PGM  
  const auto brightness = tally::settings::query<uint8_t>("/tally/led/brightness");
  if(pgmOn) tally::led::setPixelColor(0, 255, 0);
  if(pvwOn && !pgmOn) tally::led::setPixelColor(255, 0, 0);
  tally::led::show();
}

bool setUpWiFi(int maxTries) {
  auto ssid = tally::settings::query<std::string>("/tally/wifi/ssid").value();
  auto pwd = tally::settings::query<std::string>("/tally/wifi/pwd").value();
  auto useDHCP =  tally::settings::query<bool>("/tally/wifi/useDHCP").value();

  tally::settings::update("/state/status", "searching");
  tally::led::show();

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
    tally::led::show();
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
  tally::led::show();
}

void connect() {
  // CHANGE THESE TO PREDEFINED COLORS
  tally::led::setPixelColor(0, 0, 255);
  tally::led::show();
  
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
  tally::led::clear();
  tally::led::show();
}

void setup() {
  Serial.begin(921600);
  initializeDevice();
  connect();
  if(tally::settings::query<std::string>("/state/status").value().compare("configuration")) {
    syncState();
    if(tally::settings::query<bool>("/smartMode")) {
      // Smart mode enabled, wait for state to be received and then update srcId
      delay(200);
      comm::receiveAndHandleMessages();
      comm::stateT* newState = comm::getState();
      tally::settings::update("/srcId", newState->pvwId);
    }
    updateTally();
  }
}

void loop() { 
  if (tally::settings::query<std::string>("/state/status").value().compare("configuration") && client.available()) {
    comm::receiveAndHandleMessages();
    JsonVariant var;
    tally::led::setBrigtness(tally::settings::query<uint8_t>("/tally/led/brightness").value());
  }
  tally::serial::read();
  tally::webui::checkAndServeConnection();
  updateTally();

  // Felsök connection avbrott
}
