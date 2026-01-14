#ifndef _TARGET_HPP_
#define _TARGET_HPP_

#include "Client.h"
#include <vector>
#include <ArduinoJson.h>

namespace target {
    class Target {
        public:
        Target(IPAddress address, uint16_t port);
        virtual ~Target();
        virtual bool connect(Client* client, int numRetries = 0);
        virtual bool connected();
        virtual bool disconnect();
        virtual bool onPgm(uint8_t srcId);
        virtual bool onPvw(uint8_t srcId);
        virtual bool handleTrigger(JsonVariant trigger);
        virtual bool receive();

        protected:
        Client* client_ = nullptr;
        IPAddress address_;
        uint16_t port_;

    };
}

#endif