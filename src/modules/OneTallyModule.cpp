#include "OneTallyModule.hpp"
#include <mutex>
#include <ESPping.h>
#include <mutex>
#include "tally-serial.hpp"

namespace module {
    OneTallyModule::OneTallyModule(IPAddress address, uint16_t port) : 
      address_(address),
      port_(port) 
    {}

    OneTallyModule::~OneTallyModule() {}

    bool OneTallyModule::connect(Client* client, int numTries) {
        if(client_ == nullptr) client_ = client;
        if(client_->connected()) return true; 
        if(address_.toString() == "") return false;
        uint8_t i = 0;
        while(!client_->connect(address_, port_) && i++ <= numTries) {
            sleep(1);
        }
        return client_->connected(); 
    }

    const ModuleInfo* const OneTallyModule::getInfo() const {
        return &moduleInfo_;
    }


    bool OneTallyModule::start(Client* client) { return true; }
    bool OneTallyModule::stop() { return true; };

    bool OneTallyModule::handleTrigger(JsonVariant trigger) { return false; }

}