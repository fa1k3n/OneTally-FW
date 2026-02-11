#include "OneTallyModule.hpp"
#include <ArduinoJson.h>
#include <queue>
#include <functional>

namespace module {
 
    class GoStream: public OneTallyModule {
    public:
        GoStream(IPAddress address, uint16_t port = 19010, QueueHandle_t* messageQ = NULL);
        ~GoStream() {}
        bool handleTrigger(JsonVariant trigger) override;
        bool start(Client* client) override;
        bool stop() override { return true; }


    private:
        bool onPgm(JsonVariant trigger);
        bool onPvw(JsonVariant trigger);
        bool receiveMessages_();
        void handleMessage_(JsonDocument& doc);
        bool sendMessage_(String message);
        static void checkConnectionWorker_(void *pvParameters);


        std::queue<JsonDocument*> messageQueue_;

        std::vector<uint8_t> pgmIds_;
        std::vector<uint8_t> pvwIds_;        
    };
}