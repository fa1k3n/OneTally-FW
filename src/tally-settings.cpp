#include "tally-settings.hpp"
#include "tally-firmware.hpp"
#include <Preferences.h>
#include <mutex>



namespace tally {
    namespace settings {
        JsonDocument settingsBank;
        Preferences preferences;
        const __FlashStringHelper* errStr_;
        std::mutex settingsMutex;
        bool hasUnsavedChanges = false;

        const __FlashStringHelper* lastError() {
            return errStr_;
        }

        void init () {
            preferences.begin("GoTally", false);
            load();
        }

        bool hasChanges() {
            return hasUnsavedChanges;
        }

        bool reset() {
            settingsMutex.lock();
            preferences.clear();
            settingsBank.clear();
            settingsBank["debug"] = false;
            settingsBank["target"]["gostream"]["address"] = "";
            settingsBank["target"]["gostream"]["port"] = 19010;
            settingsBank["target"]["gostream"]["active"] = true;
            settingsBank["target"]["obs"]["address"] = "";
            settingsBank["target"]["obs"]["port"] = 80;
            settingsBank["target"]["obs"]["active"] = false;
            settingsBank["tally"]["srcId"] = 0;
            settingsBank["tally"]["smartMode"] = true;
            settingsBank["network"]["wifi"]["ssid"] = "";
            settingsBank["network"]["wifi"]["pwd"] = "";
            settingsBank["network"]["wifi"]["useDHCP"] = true;
            settingsBank["network"]["wifi"]["address"] = "";
            settingsBank["network"]["wifi"]["gateway"] = "";
            settingsBank["network"]["wifi"]["netmask"] = "255.255.255.0";
            settingsBank["board"]["led"]["0"]["enable"] = true;
            settingsBank["board"]["led"]["0"]["ctrlPin"] = 18;
            settingsBank["board"]["led"]["0"]["pwrPin"] = 19;
            settingsBank["board"]["led"]["0"]["count"] = 1;
            settingsBank["board"]["led"]["0"]["invert"] = false;
            settingsBank["board"]["led"]["0"]["brightness"] = 10;
            settingsBank["board"]["led"]["1"]["enable"] = true;
            settingsBank["board"]["led"]["1"]["ctrlPin"] = 14;
            settingsBank["board"]["led"]["1"]["pwrPin"] = 12;
            settingsBank["board"]["led"]["1"]["count"] = 1;
            settingsBank["board"]["led"]["1"]["invert"] = false;
            settingsBank["board"]["led"]["1"]["brightness"] = 10;
            settingsBank["board"]["firmware"]["version"] = tally::firmware::version;
            settingsBank["state"]["status"] = "disconnected";
            settingsBank["state"]["dhcpAddress"] = "";
            settingsBank["state"]["tally"] = (int)0;
            
            commit();
            settingsMutex.unlock();
            return true;
        }

        bool commit() {  
            std::string settings_str;
            serializeJson(settingsBank, settings_str);
            preferences.putString("settings", settings_str.c_str());
            hasUnsavedChanges = false;
            return true;
        }

        bool load() {
            std::string settings_str = preferences.getString("settings", "").c_str();
            if(settings_str.length() > 0) {
                settingsMutex.lock();
                deserializeJson(settingsBank, settings_str);
                settingsMutex.unlock();
            } else {
                Serial.println("No settings found in flash! Writing default");
                reset();  // No settings in flash, create basic
            }
            
            hasUnsavedChanges = false;
            return true;
        }

        bool query_(const char* path, JsonVariant& var) {
            if(settingsBank.isNull()) {
                Serial.printf("Settings not available %s\n", path);
                errStr_ = F("Error: Settings not available");
                return false;
            }

            if(strchr(path, '/') == nullptr) {
                errStr_ = F("Error: Malformed parameter address");
                return false;
            }

            // Split nodes
            char* copy = strdup(path);
            char *t = strtok(copy, "/");
            settingsMutex.lock();
            JsonVariant tmp = settingsBank;
            while (t != nullptr) {           
                JsonVariant foundObject = tmp[t];
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    settingsMutex.unlock();
                    return false;
                }
                tmp = tmp[t];
                t = strtok(nullptr, "/");
            }
            free(copy);
            settingsMutex.unlock();
            var = tmp;
            return true;
        }

        bool update(const char* path, std::string value) {
            if(settingsBank.isNull()) {
                Serial.println("Settings not loaded");
                errStr_ = F("Error: Settings not available");
                return false;
            }

            if(strchr(path, '/') == nullptr) {
                errStr_ = F("Error: Malformed parameter address");
                return false;
            }

            // Split nodes
            char* copy = strdup(path);
            char *t = strtok(copy, "/");
            settingsMutex.lock();
            JsonVariant tmp = settingsBank;
            while (t != nullptr) {
                JsonVariant foundObject = tmp[t];
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    settingsMutex.unlock();
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
            settingsMutex.unlock();

            if (std::string(path).rfind("/state", 0) != 0) {
                // Dont mark the state node as changes
                hasUnsavedChanges = true;
            }

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
        std::optional<int> query(const char* path) {
            JsonVariant var;
            if(!query_(path, var)) {
                return std::nullopt;
            }
            int val = atoi(var.as<std::string>().c_str());
            return std::make_optional<int>(val);
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
            return update(path, value == true ? "true" : "false");        
        }
    }
}