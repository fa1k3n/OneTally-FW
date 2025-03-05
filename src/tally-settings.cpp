#include "tally-settings.hpp"
#include <Preferences.h>

JsonDocument settingsBank;

namespace tally {
    namespace settings {
        Preferences preferences;
        const __FlashStringHelper* errStr_;

         const __FlashStringHelper* lastError() {
            return errStr_;
        }

        void init () {
            preferences.begin("GoTally", false); 
         /*
            settingsBank["state"]["status"] = "disconnected";
            settingsBank["state"]["dhcpAddress"] = "";
            settingsBank["debug"] = false;
            settingsBank["srcId"] = 0;
            settingsBank["smartMode"] = true;
            settingsBank["gostream"]["address"] = "127.0.0.1";
            settingsBank["gostream"]["port"] = 19010;
            settingsBank["tally"]["led"]["brightness"] = 100;
            settingsBank["tally"]["wifi"]["ssid"] = "";
            settingsBank["tally"]["wifi"]["pwd"] = "";
            settingsBank["tally"]["wifi"]["useDHCP"] = false;
            settingsBank["tally"]["wifi"]["address"] = "127.0.0.1";
            settingsBank["tally"]["wifi"]["gateway"] = "127.0.0.1";
            settingsBank["tally"]["wifi"]["netmask"] = "255.255.255.0";
            commit();
        */
        
            load();
        }

        bool commit() {  
            std::string settings_str;
            serializeJson(settingsBank, settings_str);
            preferences.putString("settings", settings_str.c_str());
            return true;
        }

        bool load() {
            std::string settings_str = preferences.getString("settings", "").c_str();
            deserializeJson(settingsBank, settings_str);
            return true;
        }

        bool query_(const char* path, JsonVariant& var) {
            if(strchr(path, '/') == nullptr) {
                errStr_ = F("Error: Malformed parameter address");
                return false;
            }
            // Split nodes
            char* copy = strdup(path);
            char *t = strtok(copy, "/");
            JsonVariant tmp = settingsBank;
            while (t != nullptr) {           
                JsonVariant foundObject = tmp[t];
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    return false;
                }
                tmp = tmp[t];
                t = strtok(nullptr, "/");
            }
            free(copy);
            var = tmp;
            return true;
        }

        bool update(const char* path, std::string value) {
            // Split nodes
            if(strchr(path, '/') == nullptr) {
                errStr_ = F("Error: Malformed parameter address");
                return false;
            }
            
            char* copy = strdup(path);
            char *t = strtok(copy, "/");
            JsonVariant tmp = settingsBank;
            while (t != nullptr) {           
                JsonVariant foundObject = tmp[t];
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    return false;
                }
                tmp = tmp[t];
                t = strtok(nullptr, "/");
                if(t == nullptr) {
                    if(value.compare("true") == 0) {
                        foundObject.set(true);
                    } else if(value.compare("false") == 0) {
                        foundObject.set(false);
                    } else if (all_of(value.begin(), value.end(), ::isdigit)) {
                        foundObject.set(atoi(value.c_str()));
                    } else {
                        foundObject.set(value);
                    }
                        
                }
            }
            free(copy);
            return true;
        }

        template<>
        std::optional<JsonVariant> query(const char* path) {
            JsonVariant var;
            if(!query_(path, var)) {
                return std::nullopt;
            }
            return std::make_optional<JsonVariant>(var);
        }

        template<>
        std::optional<std::string> query(const char* path) {
            JsonVariant var;
            if(!query_(path, var)) {
                return std::nullopt;
            }
            return std::make_optional<std::string>(var.as<std::string>());
        }

        template<>
        std::optional<uint16_t> query(const char* path) {
            JsonVariant var;
            if(!query_(path, var)) {
                return std::nullopt;
            }
            int val = atoi(var.as<std::string>().c_str());
            return std::make_optional<uint16_t>((uint16_t)val);
        }

        template<>
        std::optional<uint8_t> query(const char* path) {
            uint16_t val = query<uint16_t>(path).value();
            return std::make_optional<uint8_t>((uint8_t)val);
        }

        template<>
        std::optional<IPAddress> query(const char* path) {
            JsonVariant var;
            if(!query_(path, var)) {
                return std::nullopt;
            }
            IPAddress address;
            address.fromString(var.as<std::string>().c_str());
            return std::make_optional<IPAddress>(address);
        }

        template<>
        std::optional<bool> query(const char* path) {
            JsonVariant var;
            if(!query_(path, var)) {
                return std::nullopt;
            }
            bool ret = false;
            if(var.as<std::string>() == "true") 
                ret = true;
            return  std::make_optional<bool>(ret); 
        }

        bool update(const char* path, const char* value) {
            return update(path, std::string(value));        
        }

        bool update(const char* path, int value) {
            return update(path, std::to_string(value));        
        }

        bool update(const char* path, bool value) {
            return update(path, std::to_string(value));        
        }
    }
}