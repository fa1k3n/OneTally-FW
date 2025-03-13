#ifndef _TARGET_HPP_
#define _TARGET_HPP_

#include "Client.h"
#include <vector>

namespace target {
    class Target {
        public:
        Target(IPAddress address, uint16_t port);
        virtual ~Target();
        virtual bool connect(Client* client, int numRetries = 0);
        virtual bool connected();
        virtual bool disconnect();
        virtual std::vector<uint8_t> onPgm();
        virtual std::vector<uint8_t> onPvw();
        virtual bool receive();

        protected:
        Client* client_ = nullptr;
        IPAddress address_;
        uint16_t port_;

    };
}

#endif