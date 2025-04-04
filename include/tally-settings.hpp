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
        bool hasChanges();

        bool create(String path, JsonVariant value = JsonObject());
        bool remove(String path);

        template<typename T> std::optional<T> query(std::string path);

        // Remake to JsonVariant to be able to handle vector
        bool update(std::string path, std::string value);
        //bool update(std::string path, std::vector<int> value);
        bool update(std::string path, const char* value);
        bool update(std::string path, int value);
        bool update(std::string path, bool value);

        const __FlashStringHelper* lastError();
    }
}
#endif