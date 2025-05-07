#include "ESPAsyncWebServer.h"

#include <string>
#include <vector>
#include <sstream>
#include <WiFi.h>

#include "SPIFFS.h"

#include "tally-webui.hpp"
#include "tally-settings.hpp"
#include "tally-led.hpp"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <sstream>

extern void restart();

namespace tally {
    namespace webui {     
        AsyncWebServer server(80);
        std::vector<String> availableWifiNetworks;

        String processor(const String& var) {
            if(var == "FOUND_WIFI_NETWORKS") {
                String wifiStr = "";
                for (auto & element : availableWifiNetworks) {
                    wifiStr += "<option>" + element + "</option>";
                }
                return wifiStr;
            } 
            return String();
        }

        static AsyncCallbackJsonWebHandler *wifi_handler = new AsyncCallbackJsonWebHandler("/network", [](AsyncWebServerRequest *request, JsonVariant &json) {
                serializeJsonPretty(json, Serial);
                tally::settings::update("/network/wifi/ssid", json["wifi"]["ssid"].as<String>().c_str());
                tally::settings::update("/network/wifi/pwd", json["wifi"]["pwd"].as<String>().c_str());
                tally::settings::update("/network/targetAddress", json["targetAddress"].as<String>().c_str());
                tally::settings::update("/network/wifi/manualCfg", json["wifi"]["manualCfg"].as<bool>());   
                tally::settings::update("/network/wifi/address", json["wifi"]["address"].as<String>().c_str());
                tally::settings::update("/network/wifi/netmask", json["wifi"]["netmask"].as<String>().c_str());
            
                request->send(200, "application/json");
            });

        static AsyncCallbackJsonWebHandler *triggers_handler = new AsyncCallbackJsonWebHandler("/triggers", [](AsyncWebServerRequest *request, JsonVariant &json) {
            const auto id = std::to_string(json["id"].as<int>());
            std::string resource = std::string(request->url().c_str());

            if (request->method() == HTTP_PUT) {
                tally::settings::update("/triggers/" + id + "/peripheral", json["peripheral"].as<String>().c_str());
                tally::settings::update("/triggers/" + id + "/event", json["event"].as<String>().c_str());
                tally::settings::update("/triggers/" + id + "/srcId", json["srcId"].as<String>().c_str());
                tally::settings::update("/triggers/" + id + "/colour", json["colour"].as<String>().c_str());
                tally::settings::update("/triggers/" + id + "/brightness", json["brightness"].as<int>());
            } else if (request->method() == HTTP_POST) {
                Serial.printf("Creating resource %s\n", resource.c_str());
                JsonDocument doc;
                doc["id"] = json["id"].as<std::string>();
                doc["peripheral"] = json["peripheral"].as<std::string>();
                doc["event"] = json["event"].as<std::string>();
                doc["srcId"] = json["srcId"].as<std::string>();
                doc["assignedSrcId"] = "0";
                doc["colour"] = json["colour"].as<std::string>();
                doc["brightness"] = json["brightness"].as<int>();
                tally::settings::create(request->url(), doc);
            } 

            auto value = tally::settings::query<JsonVariant>(resource);
            Serial.printf("Query for resource %s gave answer %d\n", resource.c_str(), value.has_value());

            if(value.has_value()) {
                Serial.println("Send response");
                String sendValue; 
                serializeJson(value.value(), sendValue);
                request->send(200, "application/json", sendValue);
            } else {
                request->send(400, "application/json");
            }
        });

        static AsyncCallbackJsonWebHandler *peripherals_handler = new AsyncCallbackJsonWebHandler("/peripherals", [](AsyncWebServerRequest *request, JsonVariant &json) {
            std::string resource = std::string(request->url().c_str());
            if (request->method() == HTTP_PUT) {
                tally::settings::update( resource + "/type", json["type"].as<std::string>());
                tally::settings::update( resource + "/rgbOrder", json["rgbOrder"].as<std::string>());
                tally::settings::update( resource + "/pwrPin", json["pwrPin"].as<int>());
                tally::settings::update( resource + "/ctrlPin", json["ctrlPin"].as<int>());
                tally::settings::update( resource + "/count", json["count"].as<int>());
            } else if (request->method() == HTTP_POST) {
                Serial.printf("Creating resource %s\n", request->url().c_str());
                JsonDocument doc;
                doc["type"] = json["type"].as<std::string>();
                doc["rgbOrder"] = json["rgbOrder"].as<std::string>();
                doc["pwrPin"] = json["pwrPin"].as<int>();
                doc["ctrlPin"] = json["ctrlPin"].as<int>();
                doc["count"] = json["count"].as<int>();
                tally::settings::create(request->url(), doc);
            } 
            
            auto value = tally::settings::query<JsonVariant>(resource);
            if(value) {
                String sendValue; 
                serializeJson(value.value(), sendValue);
                request->send(200, "application/json", sendValue);
                led::init(); // Re-init the periferals
            } else {
                request->send(400, "application/json");
            }
            
        });

        bool init() {
          
  
             if(!SPIFFS.begin(true)){
                  Serial.println("An Error has occurred while mounting SPIFFS");
                        return false;
            }

            int n = WiFi.scanNetworks();

            if (n != 0) {
                for (int i = 0; i < n; ++i) {
                    availableWifiNetworks.push_back(WiFi.SSID(i));
                }
            }

            server.serveStatic("/", SPIFFS, "/");

            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                request->redirect("/index.html#!network");
            });
            server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/index.html", "text/html");
            });

            server.on("/network.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/network.html", "text/html", false, processor);
            });

            server.on("/triggers.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/triggers.html", "text/html");
            });

            server.on("/peripherals.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/peripherals.html", "text/html");
            });

            server.on("/mesh.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/mesh.html", "text/html");
            });
            
            server.on("/admin.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/admin.html", "text/html");
            });
   
            server.on("/peripherals", HTTP_GET, [](AsyncWebServerRequest *request){               
                JsonVariantConst value = tally::settings::query<JsonVariant>(request->url().c_str()).value();
                String sendValue; 
                serializeJson(value, sendValue);
                request->send(200, "application/json", sendValue);
            });

            server.on("/peripherals", HTTP_DELETE, [](AsyncWebServerRequest *request){        
                tally::settings::remove(request->url());
                request->send(200, "application/json");
            });

            server.on("/triggers", HTTP_DELETE, [](AsyncWebServerRequest *request){     
                Serial.printf("Removing trigger %s\n", request->url().c_str() );   
                tally::settings::remove(request->url());
                request->send(200, "application/json");
            });

            server.on("/network", HTTP_GET, [](AsyncWebServerRequest *request){               
                JsonVariantConst value = tally::settings::query<JsonVariant>(request->url().c_str()).value();
                String sendValue; 
                serializeJson(value, sendValue);
                request->send(200, "application/json", sendValue);
            });

            server.on("/triggers", HTTP_GET, [](AsyncWebServerRequest *request){               
                JsonVariantConst value = tally::settings::query<JsonVariant>(request->url().c_str()).value();
                String sendValue; 
                serializeJson(value, sendValue);
                request->send(200, "application/json", sendValue);
            });

            server.on("/commit", HTTP_PUT, [](AsyncWebServerRequest *request){               
                if (tally::settings::commit())
                    request->send(200, "application/json");
                else 
                    request->send(400);
            });

            server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){        
                request->send(200, "application/json");       
                restart();
            });

            wifi_handler->setMethod(HTTP_POST | HTTP_PUT);
            server.addHandler(wifi_handler);

            triggers_handler->setMethod(HTTP_POST | HTTP_PUT);
            server.addHandler(triggers_handler);

            peripherals_handler->setMethod(HTTP_POST | HTTP_PUT);
            server.addHandler(peripherals_handler);

            server.begin();

            return true;
        }
    }
}