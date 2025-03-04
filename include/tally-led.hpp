#include <Adafruit_NeoPixel.h>

namespace tally {
    namespace led {
        void init();

        void begin();
        void clear();
        void show();
        void setPixelColor(uint8_t r, uint8_t g, uint8_t b);
        void setBrigtness(uint8_t brightness);
    }
}