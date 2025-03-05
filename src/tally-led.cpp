#include "tally-led.hpp"
#include "tally-settings.hpp"
#include <vector>
#include <algorithm> 

namespace tally {
    namespace led {
        // Board specific pins
        constexpr int LED1_CTRL_PIN = 18;
        constexpr int LED1_PWR_PIN = 19;
        constexpr int LED2_CTRL_PIN = 14;
        constexpr int LED2_PWR_PIN = 12;
        constexpr int NOF_PIXELS = 1;
        
        std::vector<Adafruit_NeoPixel*> leds;
        JsonVariant connectionStatus;

        void init() {
            leds.push_back(new Adafruit_NeoPixel(NOF_PIXELS, LED1_CTRL_PIN, NEO_GRB + NEO_KHZ800));
            leds.push_back(new Adafruit_NeoPixel(NOF_PIXELS, LED2_CTRL_PIN, NEO_GRB + NEO_KHZ800));

            pinMode(LED1_PWR_PIN, OUTPUT);
            digitalWrite(LED1_PWR_PIN, HIGH);
            pinMode(LED2_PWR_PIN, OUTPUT);
            digitalWrite(LED2_PWR_PIN, HIGH);

            tally::settings::query("/state/status", connectionStatus); 
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
            if(connectionStatus.as<std::string>().compare("configuration") == 0) {
                // Configuration mode
                setPixelColor(255, 255, 0);
            } else if(connectionStatus.as<std::string>().compare("searching") == 0 || connectionStatus.as<std::string>().compare("attached") == 0 || connectionStatus.as<std::string>().compare("connecting") == 0) {
                // Configuration mode
                setPixelColor(0, 0, 255);
            }

            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->show(); });
        }

        void setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
            std::for_each(leds.begin(), leds.end(), [r, g, b](Adafruit_NeoPixel* led) { led->setPixelColor(NOF_PIXELS - 1, r, g, b); });
        }

        void setBrigtness(uint8_t brightness) {
            std::for_each(leds.begin(), leds.end(), [brightness](Adafruit_NeoPixel* led) { led->setBrightness(brightness); });
            //show();
        }
    }
}