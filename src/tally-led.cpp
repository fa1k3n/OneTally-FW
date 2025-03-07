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
 
        void init() {
            leds.push_back(new Adafruit_NeoPixel(NOF_PIXELS, LED1_CTRL_PIN, NEO_RGB + NEO_KHZ800));
            leds.push_back(new Adafruit_NeoPixel(NOF_PIXELS, LED2_CTRL_PIN, NEO_RGB + NEO_KHZ800));

            pinMode(LED1_PWR_PIN, OUTPUT);
            digitalWrite(LED1_PWR_PIN, HIGH);
            pinMode(LED2_PWR_PIN, OUTPUT);
            digitalWrite(LED2_PWR_PIN, HIGH);

            delay(100);
            begin();
        }

        void begin() {
            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->begin(); });
        }

        void clear() {
            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->clear(); });
        }

        bool goingDown = true;
        void show() {
             // Light the candles, if both PGM and PWV are active then only light PGM  
            int brightness = tally::settings::query<int>("/tally/led/brightness").value();
            auto status = tally::settings::query<std::string>("/state/status").value();
            int tallyState = tally::settings::query<int>("/state/tally").value();
            setBrightness(brightness);

            if(status.compare("configuration") == 0) {
                // Configuration mode
                setPixelColor(255, 255, 0);
            } else if(status.compare("searching") == 0 || status.compare("connecting") == 0) {
                // Configuration mode    
                setPixelColor(0, 0, 255);
            } else if(tallyState == 2) {
                setPixelColor(255, 0, 0);  
            } else if(tallyState == 1) {
                setPixelColor(0, 255, 0);
            } else if(tallyState == 0) {
                clear();
            } 
            std::for_each(leds.begin(), leds.end(), [](Adafruit_NeoPixel* led) { led->show(); });
        }

        void setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
            std::for_each(leds.begin(), leds.end(), [r, g, b](Adafruit_NeoPixel* led) { led->setPixelColor(NOF_PIXELS - 1, r, g, b); });
        }

        void setBrightness(uint8_t brightness) {
            std::for_each(leds.begin(), leds.end(), [brightness](Adafruit_NeoPixel* led) { led->setBrightness(brightness * 255 / 100); });
        }
    }
}