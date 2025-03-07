#include "Client.h"
#include <vector>

namespace target {
    class Target {
        public:
        Target(Client& client);
        virtual ~Target();
        virtual bool connect(IPAddress address, int numRetries = 0);
        virtual bool connected();
        virtual std::vector<uint8_t> onPgm();
        virtual std::vector<uint8_t> onPvw();
        virtual bool receiveAndHandleMessages();

        protected:
        Client* client_;
    };
}