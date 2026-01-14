#include "Target.hpp"
#include <ArduinoJson.h>
#include <queue>

namespace target {
    class GoStream: public Target {
    public:
        GoStream(IPAddress address, uint16_t port = 19010);
        ~GoStream() {}
        bool connect(Client* client, int numRetries = 0) override;
        bool onPgm(uint8_t srcId) override;
        bool onPvw(uint8_t srcId) override;
        bool handleTrigger(JsonVariant trigger) override;
        bool receive() override;
                bool sendMessage_(String message);


    private:
        bool receive_();
        bool receiveMessages_();
        void handleMessage_(JsonDocument& doc);

        std::queue<JsonDocument*> messageQueue_;

        std::vector<uint8_t> pgmIds_;
        std::vector<uint8_t> pvwIds_;        
    };
}