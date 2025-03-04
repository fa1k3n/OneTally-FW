#include <SPI.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Adafruit_NeoPixel.h>
#include "gostream-comm.hpp"
#include "tally-serial.hpp"
#include "tally-settings.hpp"

// Board specific pins
#define LED_PIN 18
Adafruit_NeoPixel leds(1, LED_PIN, NEO_GRB + NEO_KHZ800);

#define IN1_ID 0
#define IN2_ID 1
#define IN3_ID 2
#define IN4_ID 3
#define AUX_ID 4
#define SSRC_ID 5

WiFiClient client;

void initializeDevice() {
  tally::settings::init();

  // Led 5V
  pinMode(19, OUTPUT);
  digitalWrite(19, HIGH);
  delay(100);
  leds.begin();
  leds.clear();
  leds.show();

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
  leds.clear();
  
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
  if(pgmOn) leds.setPixelColor(0, leds.Color(0, 255 * brightness / 100, 0));
  if(pvwOn && !pgmOn) leds.setPixelColor(0, leds.Color(255 * brightness / 100, 0, 0));

  leds.show();
}

void connect() {
  leds.setPixelColor(0, leds.Color(0, 0, 255));
  leds.show();
  JsonVariant var;

  tally::settings::query("/tally/wifi/ssid", var);
  std::string ssid = var.as<std::string>();

  tally::settings::query("/tally/wifi/pwd", var);
  std::string pwd = var.as<std::string>();

  tally::settings::query("/tally/wifi/useDHCP", var);
  bool useDHCP = var.as<bool>();

  tally::settings::update("/state/status", "searching");
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
  WiFi.begin(ssid.c_str(), pwd.c_str());

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

  if(useDHCP)
    tally::settings::update("/state/dhcpAddress", WiFi.localIP().toString().c_str());

  comm::init(client);   
  IPAddress address;

  tally::settings::query("/gostream/address", var);
  std::string ip = var.as<std::string>();
  tally::settings::query("/gostream/port", var);
  uint16_t port = var.as<uint16_t>();

  address.fromString(ip.c_str());
  comm::connect(address, port);

  leds.clear();
  leds.show();
}

void setup() {
  Serial.begin(921600);
  initializeDevice();
  connect();
  syncState();
  updateTally();
}

void loop() { 
  if (client.available()) {
    if(comm::checkForUpdates()) {
      updateTally();
    }
  }

  tally::serial::read();

/*
  EthernetClient client2 = server.available();
  if (client2) {
    Serial.println("Client connected");
    // read bytes from the incoming client and write them back
    // to any clients connected to the server:
    if (client2.connected()) {
      //while (client2.available()) {
      //  char c = client2.read();
      //  Serial.write(c);
      //}
          client2.println(F("HTTP/1.1 200 OK\nContent-Type: text/html\nConnection: close\n\n"));
         //client2.println("Content-Type: text/html");
         //client2.println("Connection: close");  // the connection will be closed after completion of the response
         // client2.println("Refresh: 5");  // refresh the page automatically every 5 sec
         // client2.println();
       //   client2.println("<!DOCTYPE HTML>");
      //    client2.println("<html>");
          // output the value of each analog input pin
          
          
          //client2.println(httpHeader);
          //String httpHeader;
          //EEPROM.get(STR_BASE, httpHeader); 
         // client2.println(httpHeader);
            client2.println(F("\n<!DOCTYPE HTML>\n<html>"
            "<h1>GoTally WiFi configuration</h1><br>"
            "<form>"
              "<h2>Network information</h2><br>"
              "<label for=\"fname\">GoStream IP:</label>&emsp;<input type=\"text\" id=\"fname\" name=\"fname\" value=\"192.168.255.11\"><br>"
              "<label for=\"wifiname\">WiFi SSID:</label>&emsp;<input type=\"text\" id=\"wififname\" name=\"wifiname\"><br>"
              "<label for=\"wifipwd\">WiFi password:</label>&emsp;<input type=\"password\" id=\"wifipwd\" name=\"wifipwd\"><br>"
              "<h2>Tally information</h2><br>"
              "<label for=\"srcId\">Src Id [1:5]</label>&emsp;<input type=\"number\" id=\"srcId\" name=\"srcId\" min=\"1\" max=\"5\"><br>"
              "<label for=\"dhcp\">Manual configuration</label>&emsp;<input type=\"checkbox\" id=\"dhcp\" name=\"dhcp\" onclick=\"document.getElementById('dname').disabled = !this.checked, document.getElementById('maskname').disabled = !this.checked\"/><br>"
              "<label for=\"dname\">Device IP:</label>&emsp;<input type=\"text\" id=\"dname\" name=\"dname\" onload=\"this.disabled = !document.getElementById('dhcp').checked\"><br>"
              "<label for=\"maskname\">Netmask:</label>&emsp;<input type=\"text\" id=\"maskname\" name=\"maskname\"><br>"
              "<input type=\"submit\" value=\"Update config\"> "
            "</form>"
            "</html>"));
          //client2.println("</html>");
      client2.stop();
    }
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnected.");
    connect();
    //client.stop();
    //while(true);
  }
  */

}
