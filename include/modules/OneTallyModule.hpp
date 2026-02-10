#pragma once

#include "Client.h"
#include <vector>
#include <ArduinoJson.h>

namespace module {
    struct ModuleInfo {
        String moduleName;
        std::vector<String> triggers;
    };

    class OneTallyModule {
        public:
        OneTallyModule(IPAddress address, uint16_t port);
        virtual ~OneTallyModule();
        const ModuleInfo* const getInfo() const;

        virtual bool handleTrigger(JsonVariant trigger);
        virtual bool start(Client* client);
        virtual bool stop();
        bool started() { return started_; }
        protected:
        bool connect(Client* client, int numTries);

        bool started_ = false;
        Client* client_ = nullptr;
        IPAddress address_;
        uint16_t port_;
        ModuleInfo moduleInfo_;

    };
}