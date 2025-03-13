#include "GoStream.hpp"
#include <CRC16.h>
#include <algorithm>

#define SSRC_ID 5

// Transition sources
#define USK 1
#define DSK 2
#define BKG 4

// USK stuff
#define LUMA 0
#define CHROMA 1
#define KEYPATTERN 2
#define PIP 3

namespace target {
    GoStream::GoStream(IPAddress address, uint16_t port) : Target(address, port) {
    }
    
    bool GoStream::connect(Client* client, int numRetries) {
        Target::connect(client, numRetries);        
        if(client_->connected()) {
            sendMessage_("pvwIndex");
            sendMessage_("keyOnAir");
            sendMessage_("pgmIndex");
            sendMessage_("autoTransition");
            sendMessage_("superSourceSource1");
            sendMessage_("superSourceSource2");
            sendMessage_("superSourceBackground");
            sendMessage_("pipSource");
            sendMessage_("upStreamKeyType");
            sendMessage_("transitionSource");
            //sendMessage_("pgmTally");
            //sendMessage_("pvwTally");
        }
        return client_->connected();
    }

    std::vector<uint8_t> GoStream::onPgm() {
        std::vector<uint8_t> srcs = { state_.pgmId };

        // Supersource 
        if(state_.pgmId == SSRC_ID) {
            // TODO: Check what ssrc sources are visible and enabled before this
            srcs.push_back(state_.ssrcSrc1Id);
            srcs.push_back(state_.ssrcSrc2Id);
            srcs.push_back(state_.ssrcBkgId);
        }

        // USK
        if(state_.uskActive) {
            srcs.push_back(state_.uskFillSrcId[state_.uskType]);
        }

        // Transition, both sources "live" on pgm
        if(state_.transitionOngoing) {
            srcs.push_back(state_.pvwId);
        }

        return srcs;
    }

    std::vector<uint8_t> GoStream::onPvw() {
        std::vector<uint8_t> srcs = { state_.pvwId };
         // Supersource 
        if(state_.pvwId == SSRC_ID) {
            // TODO: Check what ssrc sources are visible and enabled before this
            srcs.push_back(state_.ssrcSrc1Id);
            srcs.push_back(state_.ssrcSrc2Id);
            srcs.push_back(state_.ssrcBkgId);
        }

        // USK
        if(state_.transitionSource & USK)
            srcs.push_back(state_.uskFillSrcId[state_.uskType]);

        // Transition, both sources "live" on pvw
        if(state_.transitionOngoing)
            srcs.push_back(state_.pgmId);

        return srcs;
    }

    bool GoStream::sendMessage_(String message) {
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

    bool GoStream::receiveMessages_() {
        while(client_->available() > 0) {
            int c = client_->peek();
            
            if((char)c != '{') {
                client_->read();
            } else {
                auto* msg = new JsonDocument();
                DeserializationError error = deserializeJson(*msg, *client_);
                messageQueue_.push(msg);

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
    void GoStream::handleMessage_(JsonDocument& doc) {
        String command = String((doc)["id"].as<const char *>());
        JsonArray value =  (doc)["value"].as<JsonArray>();
        if(command == String("pvwIndex")) {
        state_.pvwId = value[0];
        DEBUG_PRINT(state.pvwId);
        } else if(command == String("pgmIndex")) {
        state_.pgmId = value[0];
        DEBUG_PRINT(state.pgmId);
        } else if(command == String("pgmTally")) {
        //state_.pvwId = value[0];
        Serial.println("PGM TALLY");
        serializeJsonPretty(value, Serial);
        } else if(command == String("pvwTally")) {
        //state_.pgmId = value[0];
        Serial.println("PVW TALLY");
        serializeJsonPretty(value, Serial);
        } else if(command == String("autoTransition")) {
        state_.transitionOngoing = value[0] == 1 ? true : false;
        DEBUG_PRINT(state.transitionOngoing);
        } else if(command == String("superSourceSource1")) {
        state_.ssrcSrc1Id = value[0];
        DEBUG_PRINT(state.ssrcSrc1Id);
        } else if(command == String("superSourceSource2")) {
        state_.ssrcSrc2Id = value[0];
        DEBUG_PRINT(state.ssrcSrc2Id);
        } else if(command == String("superSourceBackground")) {
        state_.ssrcBkgId = value[0];
        DEBUG_PRINT(state.ssrcBkgId);
        } else if(command == String("upStreamKeyType")) {
        state_.uskType = value[0];
        DEBUG_PRINT(state.uskType);
        } else if(command == String("pipSource")) {
        state_.uskFillSrcId[PIP] = value[0];
        DEBUG_PRINT(state.pipSrcId);
        } else if(command == String("keyOnAir")) {
        state_.uskActive = value[0] == 1 ? true : false;
        DEBUG_PRINT(state.uskActive);
        } else if(command == String("transitionSource")) {
        state_.transitionSource = value[0];
        DEBUG_PRINT(state.transitionSource);
        } else if(command == String("upStreamKeyFillKeyType")) {
        int type = value[0].as<int>();
        state_.uskFillSrcId[type] = value[1];
        if(type == LUMA)
            state_.lumaKeySrcId = value[2];
        DEBUG_PRINT(state.transitionSource);
        } 
    }

    bool GoStream::receive() {
        receiveMessages_();
        int nofMessages = messageQueue_.size();
        while(messageQueue_.size() > 0) {
            JsonDocument* tmp = messageQueue_.front();
            messageQueue_.pop();
            handleMessage_(*tmp);
            delete tmp;
        }
        return nofMessages > 0;
    }
}