#include <ArduinoJson.h>

namespace tally {
    namespace settings {
        void init();
        bool commit();
        bool load();
        bool query(const char* path, JsonVariant &var);
        bool update(const char* path, std::string value);
        bool update(const char* path, const char* value);
        bool update(const char* path, int value);
        bool update(const char* path, bool value);

        const __FlashStringHelper* lastError();
    }
}