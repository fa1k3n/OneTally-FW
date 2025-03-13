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
 
        void init() {
            auto led0_count_var = tally::settings::query<int>("/board/led/0/count");
            auto led0_ctrl_var = tally::settings::query<int>("/board/led/0/ctrlPin");
            auto led0_pwr_var = tally::settings::query<int>("/board/led/0/pwrPin");
            auto led0_invert_var = tally::settings::query<bool>("/board/led/0/invert");
            if(led0_count_var && led0_ctrl_var && led0_pwr_var && led0_invert_var ) {
                leds.push_back(new Adafruit_NeoPixel(led0_count_var.value(), led0_ctrl_var.value(), (led0_invert_var.value() ? NEO_GRB : NEO_RGB) + NEO_KHZ800));
                pinMode(led0_pwr_var.value(), OUTPUT);
                digitalWrite(led0_pwr_var.value(), HIGH);
            } else {
                tally::serial::Println("Warning: strange led settings found");
            }

            auto led1_count_var = tally::settings::query<int>("/board/led/1/count");
            auto led1_ctrl_var = tally::settings::query<int>("/board/led/1/ctrlPin");
            auto led1_pwr_var = tally::settings::query<int>("/board/led/1/pwrPin");
            auto led1_invert_var = tally::settings::query<bool>("/board/led/1/invert");
            if(led1_count_var && led1_ctrl_var && led1_pwr_var && led1_invert_var ) {
                leds.push_back(new Adafruit_NeoPixel(led1_count_var.value(), led1_ctrl_var.value(), (led1_invert_var.value() ? NEO_GRB : NEO_RGB) + NEO_KHZ800));
                pinMode(led1_pwr_var.value(), OUTPUT);
                digitalWrite(led1_pwr_var.value(), HIGH);
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
             // Light the candles, if both PGM and PWV are active then only light PGM  
            auto enabled = tally::settings::query<bool>("/board/led/0/enable");
            auto brightness = tally::settings::query<int>("/board/led/0/brightness");
            auto status = tally::settings::query<std::string>("/state/status");
            auto tallyState = tally::settings::query<int>("/state/tally");
            if(!brightness || !status || !tallyState || !enabled) return;

            if(enabled.value())
                setBrightness(brightness.value());
            else 
                setBrightness(0);

            if(status.value().compare("configuration") == 0) {
                // Configuration mode
                setPixelColor(255, 255, 0);
            } else if(status.value().compare("searching") == 0 || status.value().compare("connecting") == 0) {
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