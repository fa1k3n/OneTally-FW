#include "tally-settings.hpp"
#include "tally-firmware.hpp"
#include <Preferences.h>
#include <mutex>
#include <string>
#include <fstream>
#include <SPIFFS.h>


namespace tally {
    namespace settings {
        JsonDocument settingsBank;
        JsonDocument privateBank;

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

        JsonDocument& private_getPrivateBank() {
            return privateBank;
        }

        bool reset() {
 
            settingsMutex.lock();
            preferences.clear();
            settingsBank.clear();
            File settingsFile = SPIFFS.open("/defaults.json", "r");
            if(!settingsFile) {
                Serial.println("Error reading file");
            }

            deserializeJson(settingsBank, settingsFile);
            settingsFile.close();
            settingsBank["debug"] = false;
            settingsBank["board"]["firmware"]["version"] = tally::firmware::version;
            settingsBank["state"]["status"] = "disconnected";
            settingsBank["state"]["dhcpAddress"] = "";
            settingsBank["state"]["pgm"] = (int)0;
            settingsBank["state"]["pvw"] = (int)0; 
            
            commit();
            settingsMutex.unlock();
            return true;
        }

        bool commit() {  
            std::string settings_str;
            serializeJson(settingsBank, settings_str);
            preferences.putString("settings", settings_str.c_str());

            //serializeJson(privateBank, settings_str);
            //preferences.putString("private", settings_str.c_str());
            hasUnsavedChanges = false;
            return true;
        }

        bool load() {
            std::string settings_str = preferences.getString("settings", "").c_str();
            std::string private_str = preferences.getString("private", "").c_str();
            if(settings_str.length() > 0) {
                settingsMutex.lock();
                deserializeJson(settingsBank, settings_str);
                deserializeJson(privateBank, private_str);
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
                JsonVariant foundObject;
                if(tmp.is<JsonArray>()) {
                    // Search item based on ID 
                    for(auto item : tmp.as<JsonArray>()) {
                        if(item["id"] == std::stoi(t)) {
                            foundObject = item;
                        }
                    }
                } else {
                    foundObject = tmp[t];
                }
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    settingsMutex.unlock();
                    return false;
                }
                tmp = foundObject;
                t = strtok(nullptr, "/");
            }
            free(copy);
            settingsMutex.unlock();
            var = tmp;
            return true;
        }

        std::optional<JsonVariant> _find(const char* path) {
             if(settingsBank.isNull()) {
                Serial.printf("Settings not available %s\n", path);
                errStr_ = F("Error: Settings not available");
                return std::nullopt;
            }

            if(strchr(path, '/') == nullptr) {
                errStr_ = F("Error: Malformed parameter address");
                return std::nullopt;
            }

            // Split nodes
            char* copy = strdup(path);
            char *t = strtok(copy, "/");
            settingsMutex.lock();
            JsonVariant tmp = settingsBank;
            while (t != nullptr) {           
                JsonVariant foundObject;
                if(tmp.is<JsonArray>()) {
                    foundObject = tmp.as<JsonArray>()[std::stoi(t)].as<JsonVariant>();
                } else {
                    foundObject = tmp[t];
                }
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    settingsMutex.unlock();
                    return std::nullopt;
                }
                tmp = foundObject;
                t = strtok(nullptr, "/");
            }
            free(copy);
            settingsMutex.unlock();
            return tmp;
        }

        bool create(String path, JsonVariant value) {
            String p = path.substring(0, path.lastIndexOf('/'));
            String resource = path.substring(path.lastIndexOf('/') + 1);
            settingsMutex.lock();
            if(p.isEmpty()) {
                // TODO: FIX THIS HACK SOLUTION
                settingsBank[resource].add(value);
            } else {
                auto item = _find(p.c_str()).value();
                try {
                    // Is it a number
                    auto index = std::stoi(resource.c_str());
                    item[index] = value;
                } catch (std::invalid_argument const& ex) {
                    // Nope it was a string
                    item[resource] = value;
                }
            }
             
            
            settingsMutex.unlock();
            return true;
        }

        bool remove(String path) {
            String p = path.substring(0, path.lastIndexOf('/'));
            String resource = path.substring(path.lastIndexOf('/') + 1);
             auto item = _find(p.c_str()).value();
            settingsMutex.lock();
            try {
                // Is it a number
                auto index = std::stoi(resource.c_str());
                 item.remove(index);
            } catch (std::invalid_argument const& ex) {
                // Nope it was a string
                 item.remove(resource);
            }
            //settingsBank.remove(item[resource]);
            settingsMutex.unlock();
            return true;
        }

        bool update(std::string path, std::string value) {
            if(settingsBank.isNull()) {
                Serial.println("Settings not loaded");
                errStr_ = F("Error: Settings not available");
                return false;
            }

            if(strchr(path.c_str(), '/') == nullptr) {
                errStr_ = F("Error: Malformed parameter address");
                return false;
            }

            // Split nodes
            char* copy = strdup(path.c_str());
            char *t = strtok(copy, "/");
            settingsMutex.lock();
            JsonVariant tmp = settingsBank;
            while (t != nullptr) {
                JsonVariant foundObject;
                if(tmp.is<JsonArray>()) {
                    foundObject = tmp.as<JsonArray>()[std::stoi(t)].as<JsonVariant>();
                } else {
                    foundObject = tmp[t];
                }
                if (foundObject.isNull()) {
                    free(copy);
                    errStr_ = F("path not found");
                    settingsMutex.unlock();
                    return false;
                }
                tmp = foundObject; 
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
        std::optional<JsonVariant> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            return std::make_optional<JsonVariant>(var);
        }

        template<>
        std::optional<std::string> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            return std::make_optional<std::string>(var.as<std::string>());
        }

        template<>
        std::optional<uint16_t> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            int val = atoi(var.as<std::string>().c_str());
            return std::make_optional<uint16_t>((uint16_t)val);
        }

        template<>
        std::optional<int> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            int val = atoi(var.as<std::string>().c_str());
            return std::make_optional<int>(val);
        }

        template<>
        std::optional<uint8_t> query(std::string path) {
            uint16_t val = query<uint16_t>(path).value();
            return std::make_optional<uint8_t>((uint8_t)val);
        }

        template<>
        std::optional<JsonArray> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            return std::make_optional<JsonArray>(var.as<JsonArray>());
        }

        template<>
        std::optional<IPAddress> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            IPAddress address;
            address.fromString(var.as<std::string>().c_str());
            return std::make_optional<IPAddress>(address);
        }

        template<>
        std::optional<bool> query(std::string path) {
            JsonVariant var;
            if(!query_(path.c_str(), var)) {
                return std::nullopt;
            }
            bool ret = false;
            if(var.as<std::string>() == "true") 
                ret = true;
            return  std::make_optional<bool>(ret); 
        }

        bool update(std::string path, const char* value) {
            return update(path, std::string(value));        
        }

        bool update(std::string path, int value) {
            return update(path, std::to_string(value));        
        }

        bool update(std::string path, bool value) {
            return update(path, value == true ? "true" : "false");        
        }
    }
}