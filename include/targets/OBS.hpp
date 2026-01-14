#include "Target.hpp"
#include <ArduinoJson.h>
#include <queue>
#include <WebSocketsClient.h>

namespace target {
    class OBS: public Target {
    public:
        OBS(IPAddress address, uint16_t port);
        ~OBS() {}
        bool connect(Client* client, int numRetries = 0) override;
        bool onPgm(uint8_t srcId) override;
        bool onPvw(uint8_t srcId) override;
        bool handleTrigger(JsonVariant trigger) override { return false; }
        bool receive() override;
        virtual bool connected() override;

    private:
        void webSocketEvent_(WStype_t type, uint8_t * payload, size_t length);

        std::queue<JsonDocument*> messageQueue_; 

    };
}