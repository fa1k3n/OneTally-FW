#include "Target.hpp"
#include <ArduinoJson.h>
#include <queue>

namespace target {
    class GoStream: public Target {
    public:
        GoStream(Client& client);
        ~GoStream() {}
        bool connect(IPAddress address, int numRetries = 0) override;
        
        std::vector<uint8_t> onPgm() override;
        std::vector<uint8_t> onPvw() override;
        void ping();

        bool receiveAndHandleMessages() override;

    private:
        bool sendMessage(String message);
        bool receiveMessages();
        void handleMessage(JsonDocument& doc);
    

        IPAddress address_;
        uint16_t port_;
        std::queue<JsonDocument*> messageQueue_;

        struct {
            uint8_t pgmId;
            uint8_t pvwId;
            uint8_t ssrcSrc1Id;
            uint8_t ssrcSrc2Id;
            uint8_t ssrcBkgId;
            uint8_t pipSrcId;

            int uskType;
            uint8_t uskFillSrcId[4];
            uint8_t lumaKeySrcId;
            bool uskActive;
            bool uskTied;

            bool transitionOngoing;
            int transitionSource;
        } state_; 
        
    };
}