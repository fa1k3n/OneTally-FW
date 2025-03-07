#include "Target.hpp"

namespace target {
    Target::Target(Client& client) {
        client_ = &client;
    }

    Target::~Target() {}
    bool Target::connect(IPAddress address, int numTries) { return false; }

    bool Target::connected() {
        return client_->connected();
    }

    std::vector<uint8_t> Target::onPgm() { return {}; };
    std::vector<uint8_t> Target::onPvw() { return {}; };
    bool Target::receiveAndHandleMessages() { return false; }

}