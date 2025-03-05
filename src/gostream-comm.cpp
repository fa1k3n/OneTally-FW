#include "gostream-comm.hpp"
#include "tally-settings.hpp"
#include <CRC16.h>
#include <queue>

namespace comm {
  Client* client_;
  stateT state; 
  std::queue<JsonDocument*> messageQueue;

  bool init(Client& c) {
    client_ = &c;
    return true;
  }

  bool connect(IPAddress address, uint16_t port, uint8_t maxTries) {
    tally::settings::update("/state/status", "connecting");
    Serial.print("Connecting to GoStream.");
    uint8_t i = 0;
    while(!client_->connect(address, port) && i++ < maxTries) {
      Serial.print(".");
    }
    if(client_->connected()) {
      Serial.println(F("OK"));
      tally::settings::update("/state/status", "connected");
    } else {
      Serial.println(F("ERROR"));
      return false;
    }
    
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

  bool receiveMessages() {
    while(client_->available() > 0) {
      int c = client_->peek();
      
      if((char)c != '{') {
        client_->read();
      } else {
        auto* msg = new JsonDocument();
        DeserializationError error = deserializeJson(*msg, *client_);
        messageQueue.push(msg);

        // JSON misses last char and then read CRC 
        client_->read();
        client_->read();
        client_->read();

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return false;
        }
      }
    }
  
    return true;
  }

  #define DEBUG_PRINT(val) {}; //{ if(tally::settings::query<bool>("/debug")) { Serial.print(#val); Serial.print(": "); Serial.println(val); }} 
  void handleMessage(JsonDocument& doc) {
    String command = String((doc)["id"].as<const char *>());
    JsonArray value =  (doc)["value"].as<JsonArray>();

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

  bool receiveAndHandleMessages() {
    receiveMessages();
    int nofMessages = messageQueue.size();
    while(messageQueue.size() > 0) {
      JsonDocument* tmp = messageQueue.front();
      messageQueue.pop();
      handleMessage(*tmp);
      delete tmp;
    }

    return nofMessages > 0;
  }
}