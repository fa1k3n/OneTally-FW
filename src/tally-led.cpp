#include "tally-led.hpp"
#include "tally-settings.hpp"
#include "tally-serial.hpp"
#include <vector>
#include <algorithm> 

namespace tally {
    namespace led {
        // Board specific pins
        constexpr int LED1_CTRL_PIN = 18;
        constexpr int LED1_PWR_PIN = 19;
        constexpr int LED2_CTRL_PIN = 14;
        constexpr int LED2_PWR_PIN = 12;
        constexpr int NOF_PIXELS = 2;
        
        std::vector<Adafruit_NeoPixel*> leds;

        JsonDocument pifAllocationMap;
 
        void init() {
            leds.clear();
            auto peripherals = tally::settings::query<JsonVariant>("/peripherals").value();    
            for(auto pif : peripherals.as<JsonArray>()) {
                pinMode(pif["pwrPin"].as<int>(), OUTPUT);
                if(pif["type"].as<String>() == "WS2811") {
                    String orderStr = pif["rgbOrder"];
                    int order =  NEO_RGB;
                    if(orderStr == "RGB") {
                        order = NEO_RGB;
                    } else if (orderStr == "RBG") {
                        order = NEO_RBG;
                    } else if (orderStr == "GRB") {
                        order = NEO_GRB;
                    } else if (orderStr == "GBR") {
                        order = NEO_GBR;
                    } else if (orderStr == "BRG") {
                        order = NEO_BRG;
                    } else if (orderStr == "BGR") {
                        order = NEO_BGR;
                    }
                    pifAllocationMap[pif["id"].as<int>()] = leds.size();
                    leds.push_back(new Adafruit_NeoPixel(pif["count"].as<int>(), pif["ctrlPin"].as<int>(), order + NEO_KHZ800));
                    pinMode(pif["ctrlPin"].as<int>(), OUTPUT);
                    digitalWrite(pif["pwrPin"].as<int>(), HIGH);
                }
            }

            delay(100);
            begin();
            clear();
        }

        void begin() {
            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->begin(); });
        }

        void clear() {
            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->clear(); });
        }

        void show() {
            auto status = tally::settings::query<std::string>("/state/status");
            auto pvw = tally::settings::query<int>("/state/pvw");
            auto pgm = tally::settings::query<int>("/state/pgm");
            if(!status) return;
            JsonArray triggers = tally::settings::query<JsonArray>("/triggers").value();
            JsonArray pifs = tally::settings::query<JsonVariant>("/peripherals").value();

            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { 
                led->clear(); 
                led->show();
            });

            std::vector<std::pair<uint32_t, uint8_t>> colorBrightness(leds.size());

            // Go through the triggers in prio, 1. PGM , 2. PVW, 3. Other
            // Change this to go through the LEDS, then the triggers
            // Other
            for(auto trigger : triggers) { 
                auto pifId = trigger["peripheral"].as<int>();
                auto led = leds[pifAllocationMap[pifId]];
                auto event = trigger["event"].as<String>();
                if(event.c_str() == status.value()) {
                    colorBrightness[pifAllocationMap[pifId]] = std::make_pair(std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16), trigger["brightness"].as<uint8_t>());   
                }

            }

            // PVW
            for(auto trigger : triggers) {
                auto pifId = trigger["peripheral"].as<int>();
                auto led = leds[pifAllocationMap[pifId].as<int>()];
                auto event = trigger["event"].as<String>();
                if(status.value() == "connected" && event == "onPvw" && (pvw.value() + 1) == trigger["srcId"].as<int>())
                    colorBrightness[pifAllocationMap[pifId]] = std::make_pair(std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16), trigger["brightness"].as<uint8_t>());   
            }

            // PGM
            for(auto trigger : triggers) {
                auto pifId = trigger["peripheral"].as<int>();
                auto led = leds[pifAllocationMap[pifId]];
                auto event = trigger["event"].as<String>();
                if(status.value() == "connected" &&  event == "onPgm" && (pgm.value() + 1) == trigger["srcId"].as<int>())
                    colorBrightness[pifAllocationMap[pifId]] = std::make_pair(std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16), trigger["brightness"].as<uint8_t>());   
            }

            for(int i = 0; i < colorBrightness.size(); i++) {
                auto led = leds[i];
                for(uint16_t i = 0; i < led->numPixels(); i ++)
                    led->setPixelColor(i, colorBrightness[i].first);
                led->setBrightness(colorBrightness[i].second);
                led->show();
            } 
                

        }

        void setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
            std::for_each(leds.begin(), leds.end(), [r, g, b](Adafruit_NeoPixel* led) { 
                for(uint16_t i = 0; i < led->numPixels(); i ++)
                    led->setPixelColor(i, r, g, b); 
            });
        }

        void setBrightness(uint8_t brightness) {
            std::for_each(leds.begin(), leds.end(), [brightness](Adafruit_NeoPixel* led) { led->setBrightness(brightness * 255 / 100); });
        }
    }
}