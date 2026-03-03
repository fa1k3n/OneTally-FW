#include "GoStream.hpp"
#include <CRC16.h>
#include <algorithm>
#include <tally-led.hpp>
#include "tally-settings.hpp"

namespace module {

    static QueueHandle_t* msgQ = NULL;

    GoStream::GoStream(IPAddress address, uint16_t port, QueueHandle_t* messageQ) : OneTallyModule(address, port) {
        moduleInfo_.moduleName = "GoStream";
        moduleInfo_.triggers = std::vector<String>{"onPgm", "onPvw", "connecting", "connected"};
        if(messageQ != NULL) msgQ = messageQ;
    }

    void GoStream::checkConnectionWorker_(void *pvParameters) {
        GoStream* instance = static_cast<GoStream*>(pvParameters);
        while(1) {
            instance->receiveMessages_();
            int nofMessages = instance->messageQueue_.size();
            while(instance->messageQueue_.size() > 0) {
                JsonDocument* tmp = instance->messageQueue_.front();
                instance->messageQueue_.pop();
                instance->handleMessage_(*tmp);
                delete tmp;
            }
            if(nofMessages > 0 && msgQ != NULL) xQueueSend(*msgQ, instance, portMAX_DELAY); 
            vTaskDelay(200 / portTICK_RATE_MS); //delay(200);
        }
        vTaskDelete(NULL);
    }

    bool GoStream::start(Client* client) { 
        OneTallyModule::connect(client, 5);        
        if(client_->connected()) {
            sendMessage_("pgmTally");
            sendMessage_("pvwTally");
        }
        if(client_->connected()) {
            xTaskCreatePinnedToCore(
                this->checkConnectionWorker_
                , "GoStreamWorker", 10000, this, 10, NULL, 1);
            started_ = true;
        }
        return started_;
    }


    bool GoStream::onPgm(JsonVariant trigger) {
        if(std::find(pgmIds_.begin(), pgmIds_.end(), trigger["srcId"].as<int>()) != pgmIds_.end()) {
            return true;
        }
        return false;
    }
    
    bool GoStream::onPvw(JsonVariant trigger) {
        if(std::find(pvwIds_.begin(), pvwIds_.end(), trigger["srcId"].as<int>()) != pvwIds_.end()) {
            return true;
        }
        return false;
    }

    #define ON_PGM_ID 1
    #define ON_PVW_ID 2
    #define ON_CONNECTING 3
    #define ON_CONNECTED 4
    bool GoStream::handleTrigger(JsonVariant trigger) {
        auto event = trigger["event"].as<int>();

        if(event == ON_PGM_ID)
            return onPgm(trigger);
        else if(event == ON_PVW_ID)
            return onPvw(trigger);
       // else if(event == ON_CONNECTING)
       //     return client_ != nullptr && !client_->connected();
       // else if(event == ON_CONNECTED)
       //     return client_ != nullptr && client_->connected();
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
        // Add 2 to size as it includes CRC bytes
        packet[3] = (size + 2) & 0xFF;   
        packet[4] = ((size + 2) >> 8) & 0xFF; 

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
        // No pending packets, clear packetbuffer
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
                    dataIndex = 0; // Reset dataIndex
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

    void GoStream::handleMessage_(JsonDocument& doc) {
        String command = doc["id"].as<String>();
        JsonArray value =  doc["value"].as<JsonArray>();
        if(command == String("pgmTally")) {
            pgmIds_.clear();
            for(auto id : value) {
                pgmIds_.push_back(id.as<uint16_t>());
            }
        } else if(command == String("pvwTally")) {
            pvwIds_.clear();
            for(auto id : value) {
                pvwIds_.push_back(id.as<uint16_t>());
            }
        } 
    }
}