#include "GoStream.hpp"
#include <CRC16.h>
#include <algorithm>
#include <tally-led.hpp>
#include "tally-settings.hpp"

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
    
    unsigned long lastResp = millis();
    void pingWorker2(void *pvParameters) {
        GoStream* switcher = (GoStream*)(pvParameters);
        while(1) {
            if(millis() - lastResp > 4000) {
                Serial.println("Lost connection");
                tally::settings::update("/state/status", "connecting");
            }
            switcher->sendMessage_("pgmTally");
            switcher->sendMessage_("pvwTally");
            delay(3000);
        }
    }

    bool GoStream::connect(Client* client, int numRetries) {
        Target::connect(client, numRetries);        
        if(client_->connected()) {
            lastResp = millis();
            sendMessage_("pgmTally");
            sendMessage_("pvwTally");
            //xTaskCreatePinnedToCore(pingWorker2, "Ping worker", 10000, this, 1, NULL, 0);
        }
        return client_->connected();
    }

    bool GoStream::onPgm(uint8_t srcId) {
        if(std::find(pgmIds_.begin(), pgmIds_.end(), srcId) != pgmIds_.end()) {
            return true;
        }
        return false;
    }
    
    bool GoStream::onPvw(uint8_t srcId) {
        if(std::find(pvwIds_.begin(), pvwIds_.end(), srcId) != pvwIds_.end()) {
            return true;
        }
        return false;
    }

    bool GoStream::handleTrigger(JsonVariant trigger) {
        auto srcId = trigger["srcId"].as<int>();
        auto event = trigger["event"].as<String>();
        auto pifId = trigger["peripheral"].as<int>();
        auto colour = std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16);
        auto brightness = trigger["brightness"].as<uint8_t>();

        if(event == "tally") {
          if(onPgm(srcId)) {
            tally::led::show(pifId, colour, brightness);
            return true;
          } else if(onPvw(srcId)) {
            auto altColour = std::strtoul(trigger["colourAlt"].as<String>().c_str(), NULL, 16);
            tally::led::show(pifId, altColour, brightness);  
            return true;
          }
        }

        return false;
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

    uint8_t packet[1024];
    uint8_t dataIndex = 0;
    bool GoStream::receiveMessages_() {
        // No pending packets, clear it
        if(dataIndex == 0)
            memset(packet, 0, 1024 * sizeof(uint8_t));
        while(client_->available() > 0) {
            int c = client_->peek();
            if(c != -1) {
                dataIndex += client_->readBytes((uint8_t*)&packet[dataIndex], 5);
                // Check that enough has been read that we have the header
                if(dataIndex < 5) {
                    Serial.println("Incomplete packet header");
                    continue;
                }

                // At least header has been received, check magic number
                if(packet[0] != 0xeb || packet[1] != 0xa6) {
                    Serial.println("Header is not correct. Throwing away frame");
                    client_->flush();
                    continue;
                }

                // Correct header, read data
                uint16_t len = packet[4] << 8 | packet[3];
                dataIndex += client_->readBytes((uint8_t*)&packet[dataIndex], len);

                // Not all data received, save current write pointer
                if(dataIndex < len) {
                    Serial.printf("Incomplete packet data. Write pointer saved at %i\n", dataIndex);
                    continue;
                }
                    
                dataIndex = 0;
                // Complete package read, check that CRC is correct
                CRC16 crc(CRC16_MODBUS_POLYNOME,
                            CRC16_MODBUS_INITIAL,
                            CRC16_MODBUS_XOR_OUT,
                            CRC16_MODBUS_REV_IN,
                            CRC16_MODBUS_REV_OUT);
                crc.add(packet, len + 5 - 2);

                uint16_t calculatedCrcSum = crc.calc();
                uint16_t receivedCrcSum = packet[len + 5 - 1] << 8 | packet[len + 5 - 2];
                if(calculatedCrcSum != receivedCrcSum) {
                    Serial.printf("CRC is not correct. Received 0x%4x, calc 0x%4x\n", receivedCrcSum, calculatedCrcSum);
                    continue;
                }

                // CRC is OK, deserialize data
                auto* msg = new JsonDocument();
                DeserializationError error = deserializeJson(*msg, &packet[5]);
                messageQueue_.push(msg);
            }
        }
        return true;
    }

    #define DEBUG_PRINT(val) {}; //{ if(tally::settings::query<bool>("/debug")) { Serial.print(#val); Serial.print(": "); Serial.println(val); }} 
    void GoStream::handleMessage_(JsonDocument& doc) {
        String command = String((doc)["id"].as<const char *>());
        JsonArray value =  (doc)["value"].as<JsonArray>();
        if(command == String("pgmTally")) {
            pgmIds_.clear();
            for(auto id : value) {
                pgmIds_.push_back(id.as<uint8_t>());
            }
            lastResp = millis();
        } else if(command == String("pvwTally")) {
            pvwIds_.clear();
            for(auto id : value) {
                pvwIds_.push_back(id.as<uint8_t>());
            }
            lastResp = millis();
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