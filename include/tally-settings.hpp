#ifndef TALLY_SETTINGS_HPP
#define TALLY_SETTINGS_HPP
#include <ArduinoJson.h>
#include <optional>

namespace tally {
    namespace settings {
        void init();
        bool commit();
        bool load();
        bool reset();

        template<typename T> std::optional<T> query(const char* path);

        bool update(const char* path, std::string value);
        bool update(const char* path, const char* value);
        bool update(const char* path, int value);
        bool update(const char* path, bool value);

        const __FlashStringHelper* lastError();
    }
}
#endif