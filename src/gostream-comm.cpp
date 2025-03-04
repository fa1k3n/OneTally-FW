#include "gostream-comm.hpp"
#include "tally-settings.hpp"
#include <CRC16.h>


namespace comm {
  WiFiClient* client_;
  stateT state; // = {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}, 0, false, false, false, 0};

  bool init(WiFiClient& c) {
    client_ = &c;
    return true;
  }

  bool connect(IPAddress address, uint16_t port) {
    tally::settings::update("/state/status", "connecting");
    Serial.print("Gostream IP: ");
    Serial.println(address.toString());

   while(!client_->connect(address, port)) {
        // TODO: Max number of tries then return false if not connected
      Serial.println("Failed to connect to GoStream");
      delay(1000);
    }

    Serial.println(F("Connected to GoStream"));
    tally::settings::update("/state/status", "connected");
    return true;
  }

  bool sendMessage(String message) {  
   JsonDocument json;
    uint8_t packet[128];
    json["id"] = message ;
    json["type"] = "get";
    memset(packet, 0, 128);  
    size_t size = serializeJson(json, &packet[5], 119);

    packet[0] = 0xeb;
    packet[1] = 0xa6;
    packet[2] = 0;
    packet[3] = size + 2;    // Ugly LE hardcode 
    packet[4] = 0; 

    CRC16 crc(CRC16_MODBUS_POLYNOME,
            CRC16_MODBUS_INITIAL,
            CRC16_MODBUS_XOR_OUT,
            CRC16_MODBUS_REV_IN,
            CRC16_MODBUS_REV_OUT);
      for(int i = 0; i < size + 5; i++) {
      uint8_t c = packet[i];
        crc.add(c);
      }
    uint16_t crcSum = crc.calc();
    packet[size + 5] = ((uint8_t)crcSum  & 0XFF);
    packet[size + 6] = ((uint8_t)(crcSum >> 8) & 0XFF);
    client_->write(packet, size + 7);
    return true;
  }

  bool receiveMessage(JsonDocument* doc) {
    // peek incoming bytes from the server 
    int c = client_->peek();

    if(c == -1) {
      return false;
    }
  
    while ((char)c != '{') {
      client_->read();
      c = client_->peek();
    }
    DeserializationError error = deserializeJson(*doc, *client_);

    // JSON misses last char and then read CRC 
    client_->read();
    client_->read();
    client_->read();

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }
    return true;
  }

  #define DEBUG_PRINT(val) { JsonVariant var; tally::settings::query("/debug", var); if(var.as<bool>()) { Serial.print(#val); Serial.print(": "); Serial.println(val); }} 
  void handleMessage(JsonDocument* doc) {
    String command = String((*doc)["id"].as<const char *>());
    JsonArray value =  (*doc)["value"].as<JsonArray>();

    if(command == String("pvwIndex")) {
      state.pvwId = value[0];
      DEBUG_PRINT(state.pvwId);
    } else if(command == String("pgmIndex")) {
      state.pgmId = value[0];
      DEBUG_PRINT(state.pgmId);
    } else if(command == String("autoTransition")) {
      state.transitionOngoing = value[0] == 1 ? true : false;
      DEBUG_PRINT(state.transitionOngoing);
    } else if(command == String("superSourceSource1")) {
      state.ssrcSrc1Id = value[0];
      DEBUG_PRINT(state.ssrcSrc1Id);
    } else if(command == String("superSourceSource2")) {
      state.ssrcSrc2Id = value[0];
      DEBUG_PRINT(state.ssrcSrc2Id);
    } else if(command == String("superSourceBackground")) {
      state.ssrcBkgId = value[0];
      DEBUG_PRINT(state.ssrcBkgId);
    } else if(command == String("upStreamKeyType")) {
      state.uskType = value[0];
      DEBUG_PRINT(state.uskType);
    } else if(command == String("pipSource")) {
      state.uskFillSrcId[PIP] = value[0];
      DEBUG_PRINT(state.pipSrcId);
    } else if(command == String("keyOnAir")) {
      state.uskActive = value[0] == 1 ? true : false;
      DEBUG_PRINT(state.uskActive);
    } else if(command == String("transitionSource")) {
      state.transitionSource = value[0];
      DEBUG_PRINT(state.transitionSource);
    } else if(command == String("upStreamKeyFillKeyType")) {
      int type = value[0].as<int>();
      state.uskFillSrcId[type] = value[1];
      if(type == LUMA)
        state.lumaKeySrcId = value[2];
      DEBUG_PRINT(state.transitionSource);
    } else {
      Serial.print("Unknown command "); Serial.print(command); Serial.print(": "); Serial.println(value[0].as<int>());
    }
  }

  stateT* getState() {
    return &state;
  }

  bool checkForUpdates() {
    bool stateUpdated = false;
    JsonDocument json;
    while(receiveMessage(&json)) {
      stateUpdated = true;
      handleMessage(&json);
    }
    return stateUpdated;
  }
}