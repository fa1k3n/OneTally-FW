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
JsonVariant connectionStatus;

void initializeDevice() {
  tally::settings::init();
  tally::led::init();
  tally::serial::init();

  tally::settings::query("/state/status", connectionStatus);
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
  JsonVariant srcIdVar;
  tally::settings::query("/srcId", srcIdVar);
  int8_t srcId = srcIdVar.as<int8_t>();

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
  JsonVariant brightnessVar;
  tally::settings::query("/tally/led/brightness", brightnessVar);
  int8_t brightness = brightnessVar.as<int8_t>();
  if(pgmOn) tally::led::setPixelColor(0, 255, 0);
  if(pvwOn && !pgmOn) tally::led::setPixelColor(255, 0, 0);
  tally::led::show();
}

bool setUpWiFi(int maxTries) {
  JsonVariant var;
  tally::settings::query("/tally/wifi/ssid", var);
  std::string ssid = var.as<std::string>();

  tally::settings::query("/tally/wifi/pwd", var);
  std::string pwd = var.as<std::string>();

  tally::settings::query("/tally/wifi/useDHCP", var);
  bool useDHCP = var.as<bool>();

  tally::settings::update("/state/status", "searching");
  tally::led::show();
  if(!useDHCP) {
    tally::settings::query("/tally/wifi/address", var);
    IPAddress tallyAddress;
    tallyAddress.fromString(var.as<std::string>().c_str());
    tally::settings::query("/tally/wifi/gateway", var);
    IPAddress tallyGateway;
    tallyGateway.fromString(var.as<std::string>().c_str());
    tally::settings::query("/tally/wifi/netmask", var);
    IPAddress tallyNetmask;
    tallyNetmask.fromString(var.as<std::string>().c_str());
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

  JsonVariant var;
  tally::settings::query("/gostream/address", var);
  std::string ip = var.as<std::string>();
  tally::settings::query("/gostream/port", var);
  uint16_t port = var.as<uint16_t>();

  IPAddress address;
  address.fromString(ip.c_str());
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
  if(connectionStatus.as<std::string>().compare("configuration")) {
    syncState();
    JsonVariant smartMode;
    tally::settings::query("/smartMode", smartMode);
    if(smartMode.as<bool>()) {
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
  if (connectionStatus.as<std::string>().compare("configuration") && client.available()) {
    comm::receiveAndHandleMessages();
    JsonVariant var;
    tally::settings::query("/tally/led/brightness", var);
    tally::led::setBrigtness(var.as<uint8_t>());
  }
  tally::serial::read();
  tally::webui::checkAndServeConnection();
  updateTally();

  // Felsök connection avbrott
}
