#include "ESPAsyncWebServer.h"

#include <string>
#include <vector>
#include <sstream>
#include <WiFi.h>

#include "SPIFFS.h"

#include "tally-webui.hpp"
#include "tally-settings.hpp"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <sstream>

extern void restart();

namespace tally {
    namespace webui {     
        AsyncWebServer server(80);
        std::vector<String> availableWifiNetworks;

        String peripheralProcessor(const String& var) {
            std::stringstream pifStr;
            if(var == "PERIPHERALS") {
                JsonArray peripherals = tally::settings::query<JsonArray>("/peripherals").value();
                for(auto pif : peripherals) {
                    int id = pif["id"];
                    pifStr << "<tr class='clickable-row' onclick='showPeripheral(" << id << ")' style='cursor:pointer'>" << std::endl;
                    pifStr << "<td>" << id << "</td>" << std::endl;
                    pifStr << "<td id=\"" << id << "_name\">" << pif["name"].as<std::string>() << "</td>" << std::endl;
                    pifStr << "<td id=\"" << id << "_type\">" << pif["type"].as<std::string>() << "</td>" << std::endl;
                    pifStr << "<td id=\"" << id << "_rgbOrder\">" << pif["rgbOrder"].as<std::string>() << "</td>" << std::endl;
                    pifStr << "<td id=\"" << id << "_pwrPin\">" << pif["pwrPin"] << "</td>" << std::endl;
                    pifStr << "<td id=\"" << id << "_ctrlPin\">" << pif["ctrlPin"] << "</td>" << std::endl;
                    pifStr << "<td id=\"" << id << "_count\">" << pif["count"] << "</td>" << std::endl;
                    pifStr << "<td><span data-feather='trash-2'></span></td>" << std::endl;
                    pifStr << "</tr>" << std::endl;
                }
                return String(pifStr.str().c_str());
            }
            return String();
        }

        String triggersProcessor(const String& var) {
            std::stringstream eventStr;
            if(var == "TRIGGERS") {
                JsonArray triggers = tally::settings::query<JsonArray>("/triggers").value();
                for(auto trigger : triggers) {
                    int id = trigger["id"];
                    eventStr << "<tr class='clickable-row' style='cursor:pointer'>" << std::endl;
                    eventStr << "<td onclick='showTrigger(" << id << ")' id='" << id << "_id'>" << id << "</td>" << std::endl;
                    eventStr << "<td onclick='showTrigger(" << id << ")' id='" << id << "_event'>" << trigger["event"].as<std::string>() << "</td>" << std::endl;
                    eventStr << "<td onclick='showTrigger(" << id << ")' id='" << id << "_srcId'>" << trigger["srcId"].as<std::string>() << "</td>" << std::endl;

                  /*  JsonVariant peripheral = tally::settings::query<JsonVariant>("/peripheral/" + std::to_string(event["peripheralId"].as<int>())).value();
                    eventStr << "<td onclick='showTally(" << id << ")' id='" << id << "_peripheral'>" << peripheral["name"] << "</td>" << std::endl;      
                    eventStr << "<td onclick='showTally(" << id << ")' id='" << id << "_colour'>" << event["colour"].as<std::string>() << "</td>" << std::endl;
                    eventStr << "<td><input type='range' id='" << id << "_brightness' value='" << event["colour"].as<int>() << "'min='0' max='100' name='brightness'></td>" << std::endl;
                    eventStr << "<td><span data-feather='trash-2'></span></td>" << std::endl;
                    eventStr << "</tr>" << std::endl;*/
                }
                return String(eventStr.str().c_str());
            }
            return String();
        }

        String processor(const String& var) {
           if(var == "TARGET_IP"){
                auto targetIP_var = tally::settings::query<IPAddress>("/target/gostream/address");
                String targetIP = "";
                if(targetIP_var) targetIP = targetIP_var.value().toString();
                return targetIP;
            } else if(var == "WIFI_SSID") {
                auto confWifiSSID_var = tally::settings::query<std::string>("/network/wifi/ssid");
                String wifiSSID = "";
                if(confWifiSSID_var) wifiSSID = String(confWifiSSID_var.value().c_str());
                return wifiSSID;
            } else if(var == "FOUND_WIFI_NETWORKS") {
                String wifiStr = "";
                for (auto & element : availableWifiNetworks) {
                    wifiStr += "<option>" + element + "</option>";
                }
                return wifiStr;
            } else if(var == "WIFI_PWD") {
                auto confWifiPwd_var = tally::settings::query<std::string>("/network/wifi/pwd");
                String wifiPwd = "";
                if(confWifiPwd_var) wifiPwd = String(confWifiPwd_var.value().c_str());
                return wifiPwd;
            } else if(var == "SWITCHER_IP") {
                auto confSwitcherIP_var = tally::settings::query<std::string>("/target/gostream/address");
                String switcherIP = "";
                if(confSwitcherIP_var) switcherIP = String(confSwitcherIP_var.value().c_str());
                return switcherIP;
            } else if(var == "SRC_ID") {
                auto srcId_var = tally::settings::query<int>("/tally/srcId");
                int srcId = 1;
                if(srcId_var) srcId = srcId_var.value() + 1;
                return String(srcId);
            } else if(var == "SMART_MODE") {
                auto smartMode_var = tally::settings::query<bool>("/tally/smartMode");
                bool smartMode = true;
                if(smartMode_var) smartMode = smartMode_var.value();
                return smartMode ? "checked" : "";
            }else if(var == "MANUAL_CFG") {
                auto useDHCP_var = tally::settings::query<bool>("/network/wifi/useDHCP");
                bool manualConfig = false;
                if(useDHCP_var) manualConfig = !useDHCP_var.value();
                return manualConfig ? "checked" : "";
            } else if(var == "TALLY_IP") {
                auto tallyIP_var = tally::settings::query<IPAddress>("/network/wifi/address");
                String tallyIP = "";
                if(tallyIP_var) tallyIP = tallyIP_var.value().toString();
                return tallyIP;
            } else if(var == "TALLY_NETMASK") {
                auto tallyNetmask_var = tally::settings::query<IPAddress>("/network/wifi/netmask");
                String tallyNetmask = "";
                if(tallyNetmask_var) tallyNetmask = tallyNetmask_var.value().toString();
                return tallyNetmask;
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
            //server.serveStatic("/*.css", SPIFFS, "/*.css");
            //server.serveStatic("/trigger-details.template.html", SPIFFS, "/trigger-details.template.html");
            //server.serveStatic("/triggers-list.template.html", SPIFFS, "/triggers-list.template.html");
            /*server.serveStatic("/bootstrap.min.css", SPIFFS, "/bootstrap.min.css");
            server.serveStatic("/webui.css", SPIFFS, "/webui.css");
            server.serveStatic("/bootstrap.bundle.js", SPIFFS, "/bootstrap.bundle.js");
            server.serveStatic("/ui-bootstrap.min.js", SPIFFS, "/ui-bootstrap.min.js");
            server.serveStatic("/angular.min.js", SPIFFS, "/angular.min.js");
            server.serveStatic("/angular-route.js", SPIFFS, "/angular-route.js");
            server.serveStatic("/feather.min.js", SPIFFS, "/feather.min.js");
            server.serveStatic("/jquery.min.js", SPIFFS, "/jquery.min.js");
            server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
            server.serveStatic("/trigger-details.module.js", SPIFFS, "/trigger-details.module.js");
            server.serveStatic("/trigger-details.template.html", SPIFFS, "/trigger-details.template.html");
            server.serveStatic("/triggers-list.module.js", SPIFFS, "/triggers-list.module.js");
            server.serveStatic("/triggers-list.template.html", SPIFFS, "/triggers-list.template.html");
            server.serveStatic("/dashboard.config.js", SPIFFS, "/dashboard.config.js");
            server.serveStatic("/dashboard.module.js", SPIFFS, "/dashboard.module.js");*/

            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                request->redirect("/index.html#!network");
            });
            server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/index.html", "text/html", false, processor);
            });

            server.on("/network.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/network.html", "text/html", false, processor);
            });

            server.on("/triggers.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/triggers.html", "text/html");
            });

            server.on("/peripherals.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/peripherals.html", "text/html", false, peripheralProcessor);
            });

            server.on("/mesh.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/mesh.html", "text/html", false, processor);
            });
            
            server.on("/admin.html", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/admin.html", "text/html", false, processor);
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