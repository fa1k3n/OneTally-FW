#include <Adafruit_NeoPixel.h>

namespace tally {
    namespace led {
        void init();

        void begin();
        void clear();
        void clear(int pifId);
        void show(int pifId, uint32_t colour, uint8_t brightness);
        void setPixelColor(uint8_t r, uint8_t g, uint8_t b);
        void setBrightness(uint8_t brightness);
    }
}