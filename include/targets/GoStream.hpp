#include "Target.hpp"
#include <ArduinoJson.h>
#include <queue>

namespace target {
    class GoStream: public Target {
    public:
        GoStream(IPAddress address, uint16_t port = 19010);
        ~GoStream() {}
        bool connect(Client* client, int numRetries = 0) override;
        std::vector<uint8_t> onPgm() override;
        std::vector<uint8_t> onPvw() override;
        bool receive() override;

    private:
        bool sendMessage_(String message);
        bool receive_();
        bool receiveMessages_();
        void handleMessage_(JsonDocument& doc);

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