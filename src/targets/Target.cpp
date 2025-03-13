#include "Target.hpp"
#include <mutex>
#include <ESPping.h>
#include <mutex>
#include "tally-serial.hpp"

namespace target {
    struct pingInfoT {
        Target* target;
        IPAddress address;
    } pingInfo;

    std::mutex pingMutex;

    void pingWorker(void *pvParameters) {
        struct pingInfoT info = *(pingInfoT*)pvParameters;
        while(1) {
            bool ret = Ping.ping(pingInfo.address);
            if(!ret) {
                tally::serial::Println("\nLost connection to taget");                    
                pingInfo.target->disconnect();
                vTaskDelete(NULL);
            }
            delay(1000);
        }
    }

    Target::Target(IPAddress address, uint16_t port) : 
      address_(address),
      port_(port) 
    {}

    Target::~Target() {}

    bool Target::connect(Client* client, int numTries) {
        if(client_ == nullptr) client_ = client;
        if(client_->connected()) return true; 
        uint8_t i = 0;
        while(!client_->connect(address_, port_) && i++ <= numTries) {}
        if(client_->connected()) {
            pingInfo = { this, address_};
            xTaskCreate(
                pingWorker,    // Function that should be called
                "pinger",  // Name of the task (for debugging)
                5000,            // Stack size (bytes)
                (void*)&pingInfo,            // Parameter to pass
                1,               // Task priority
                NULL             // Task handle
            );
        }
        return client_->connected(); 
    }

    bool Target::disconnect() {
        if(!client_->connected()) return true;
        pingMutex.lock();
        client_->stop();
        pingMutex.unlock();
        return true;
    }

    bool Target::connected() {
        if(client_) {
            pingMutex.lock();
            bool connected = client_->connected();
            pingMutex.unlock();
            return connected;
        }
        return false;
    }

    std::vector<uint8_t> Target::onPgm() { return {}; };
    std::vector<uint8_t> Target::onPvw() { return {}; };
    bool Target::receive() { return false; }

}