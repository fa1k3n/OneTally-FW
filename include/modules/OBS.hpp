#include "OneTallyModule.hpp"
#include <ArduinoJson.h>
#include <queue>
#include <WebSocketsClient.h>

namespace module {
    class OBS: public OneTallyModule {
    public:
        OBS(IPAddress address, uint16_t port);
        ~OBS() {}
        bool start(Client* client) override;


    private:
        void webSocketEvent_(WStype_t type, uint8_t * payload, size_t length);

        std::queue<JsonDocument*> messageQueue_; 

    };
}