#include "OBS.hpp"

#include <ArduinoJson.h>
#include <queue>

        WebSocketsClient webSocket;
        bool connecting = false;

namespace target {
     
    void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
	const uint8_t* src = (const uint8_t*) mem;
	Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
	for(uint32_t i = 0; i < len; i++) {
		if(i % cols == 0) {
			Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
		}
		Serial.printf("%02X ", *src);
		src++;
	}
	Serial.printf("\n");
}

    bool connected_ = false;

    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	    switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED: {
            Serial.printf("[WSc] Connected to url: %s\n", payload);
			break;
        }
		case WStype_TEXT: {
            Serial.printf("[WSc] get text: %s\n", payload);
            JsonDocument resp; 
            deserializeJson(resp, payload);
            if(resp["op"] == 0) {
                Serial.println("Hello message received");
                JsonDocument doc;
                doc["op"] = 1;
                doc["d"]["rpcVersion"] = 1;
                std::string json;
                size_t size = serializeJson(doc, json);
                webSocket.sendTXT(json.c_str());
            } else if(resp["op"] == 2) {
                Serial.println("Identified message received");
                connected_ = true;
                connecting = false;
            }
            else if(resp["op"] == 5) {
                Serial.println("Event received");
            }
			break;
        }
		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}

}

    OBS::OBS(IPAddress address, uint16_t port) : Target(address, port) {
    }
    
    bool OBS::connected() { return connected_; }
    bool OBS::connect(Client* client, int numRetries) { 
        if(!connecting) {
            connecting = true;
            webSocket.begin(address_, port_, "/");
            webSocket.onEvent(webSocketEvent);
        }
        webSocket.loop();
        return true; 
    }

    std::vector<uint8_t> OBS::onPgm() {
        return {};
    }

    std::vector<uint8_t> OBS::onPvw() {
        return {};
    }

    bool OBS::receive() {
        webSocket.loop();
        return true;
    }
        
}