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

        typedef enum {
            WS2811 = 1
        } boardType;

        // Change these to the NEO_ defines and add endpoint to retreive them
        typedef enum {
            RGB = 1,
            RBG = 2,
            GRB = 3,
            GBR = 4,
            BRG = 5,
            BGR = 6
        } rgbByteOrder;
 
        void init() {
            leds.clear();
            auto peripherals = tally::settings::query<JsonVariant>("/peripherals").value();    
            for(auto pif : peripherals.as<JsonArray>()) {
                if(pif["type"].as<int>() == WS2811) {
                    int order = pif["rgbOrder"];
                    if(order == RGB) {
                        order = NEO_RGB;
                    } else if (order == RBG) {
                        order = NEO_RBG;
                    } else if (order == GRB) {
                        order = NEO_GRB;
                    } else if (order == GBR) {
                        order = NEO_GBR;
                    } else if (order == BRG) {
                        order = NEO_BRG;
                    } else if (order == BGR) {
                        order = NEO_BGR;
                    }
                    pifAllocationMap[pif["id"].as<int>()] = leds.size();
                    leds.push_back(new Adafruit_NeoPixel(pif["count"].as<int>(), pif["ctrlPin"].as<int>(), order + NEO_KHZ800));
                    pinMode(pif["ctrlPin"].as<int>(), OUTPUT);
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
            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->clear(); led->show(); });
        }

        void clear(int pifId) {
            auto led = leds[pifAllocationMap[pifId]];
            led->clear();
            led->show();
        }

        void show(int pifId, uint32_t colour, uint8_t brightness) {
            auto led = leds[pifAllocationMap[pifId]];
            for(uint16_t i = 0; i < led->numPixels(); i ++)
                led->setPixelColor(i, colour);
            led->setBrightness(brightness);
            led->show();


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