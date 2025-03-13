#include <ArduinoHttpServer.h>
#include <string>
#include <vector>
#include <sstream>
#include <WiFi.h>

#include "tally-webui.hpp"
#include "tally-settings.hpp"

extern void restart();

namespace tally {
    namespace webui {
        WiFiServer* server_;

       const char* managementPage = "<html>\n"
"    <head>\n"
"        <style>\n"
"            small {\n"
"                    font-size: smallest;\n"
"                    color: blue;\n"
"            }\n"
"            h1 {\n"
"                    color: red;\n"
"            }\n"
"            .cfg {\n"
"                    background-color: tomato;\n"
"                    padding: 5px 10px 20px 10px;\n"
"                    margin-bottom: 10px\n"
"            }\n"
"\n"
"            form  { display: table;      }\n"
"            p     { display: table-row; padding: 20px 5px 20px 5px; }\n"
"            label { display: table-cell; text-align: right }\n"
"            input { display: table-cell;  }\n"
"            input:invalid { border-style: dashed; border-color: red}\n"
"        </style>\n"
"        <script>\n"
"                function disableUnused() {\n"
"                        \n"
"                        document.getElementById('srcid').disabled = document.getElementById('smart').checked\n"
"                        const manualChecked = document.getElementById('manual').checked\n"
"                        document.getElementById('manualCfg').hidden = !manualChecked\n"
"                        document.getElementById('manualCfg').hidden = !manualChecked\n"
"                        document.getElementById('tip').disabled = !manualChecked\n"
"                        document.getElementById('nip').disabled = !manualChecked\n"
"                }\n"
"\n"
"        </script>\n"
"    </head>\n"
"    <body onload=\"disableUnused()\">\n"
"        <h1>GoTally<sub><small>WiFi</small></sub> configuration</h1>\n"
"        <form action=\"/update\">\n"
"        <div class=\"cfg\">\n"
"                <h2>Network configuration</h2>\n"
"                <p><label for=\"gsip\">Target IP</label><input type=\"text\" minlength=\"7\" maxlength=\"15\"  required pattern=\"^((\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])$\" id=\"gsip\" name=\"gsip\" value=\"%s\"></p>\n"
"                <p><label for=\"ssid\">WiFi SSID:</label><input type=\"text\" required id=\"ssid\" name=\"ssid\" value=\"%s\" list=\"foundWifis\">\n"
"                <datalist id=\"foundWifis\">\n"
"                %s\n"
"                </datalist>\n"
"                </p>\n"
"                <p><label for=\"wpwd\">WiFi Password:</label><input type=\"password\" required id=\"wpwd\" value=\"%s\" name=\"wpwd\"></p>\n"
"        </div>\n"
"        <div class=\"cfg\" >\n"
"                <h2>Tally configuration</h2>\n"
"                <p>\n"
"                <label for=\"srcid\">Source ID (1-7)</label>\n"
"                <input type=\"number\" max=\"5\" min=\"1\" required id=\"srcid\" name=\"srcid\" value=\"%d\">\n"
"                <label for=\"smart\">Smart assign</label>\n"
"                <input type=\"checkbox\" id=\"smart\" %s name=\"smart\" onclick=\"disableUnused()\">\n"
"                </p>\n"
"                <p>\n"
"                <label for=\"brightness\">Led brightness (0-100)</label>\n"
"                <input type=\"range\" id=\"brightness\" value=\"%d\" min=\"0\" max=\"100\" name=\"brightness\">\n"
"                <label for=\"invertled\">Invert led</label>\n"
"                <input type=\"checkbox\" id=\"invertled\" %s name=\"invertled\">\n"
"                </p>\n"
"                <p><label for=\"manual\">Manual configuration</label>\n"
"                <input type=\"checkbox\" id=\"manual\" name=\"manual\" %s onclick=\"disableUnused()\"></p>\n"
"\n"
"                <div id=\"manualCfg\">\n"
"                <p><label for=\"tid\">Tally IP</label>\n"
"                <input type=\"text\" minlength=\"7\" maxlength=\"15\" required pattern=\"^(?>(\\d|[1-9]\\d{2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(?1)$\" id=\"tip\" name=\"tip\" value=\"%s\"></p>\n"
"\n"
"        <p><label for=\"tnmid\">Tally netmask:</label>\n"
"                <input type=\"text\" minlength=\"7\" maxlength=\"15\" required pattern=\"^(?>(\\d|[1-9]\\d{2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(?1)$\" id=\"tnm\" name=\"tnm\" value=\"%s\"></p>\n"
"                </div>\n"
"        </div>\n"
"        <input type=\"submit\" value=\"Submit\">\n"
"        </form>\n"
"    </body>\n"
"</html>\n"
"\n"
"";
       

        std::vector<std::string> availableWifiNetworks;

        bool init(WiFiServer& c) {
            server_ = &c;
            int n = WiFi.scanNetworks();

            if (n != 0) {
                for (int i = 0; i < n; ++i) {
                    availableWifiNetworks.push_back(std::string(WiFi.SSID(i).c_str()));
                }
            }

            return true;
        }

        void checkAndServeConnection() {
            WiFiClient client = server_->available();
            if (client) {
                Serial.println("Client connected");

                ArduinoHttpServer::StreamHttpRequest<1024> httpRequest(client);

                // Parse the request.
                if (httpRequest.readRequest())
                {
                    auto url = httpRequest.getResource().toString();
                    if(url == "/") {
                        ArduinoHttpServer::StreamHttpReply reply(client, httpRequest.getContentType());
                        char* parsedPage = (char*)malloc(5000);

                        // Target IP
                        auto targetIP_var = tally::settings::query<IPAddress>("/target/gostream/address");
                        std::string targetIP = "";
                        if(targetIP_var) targetIP = targetIP_var.value().toString().c_str();

                         // Configured WiFi
                        auto confWifiSSID_var = tally::settings::query<std::string>("/network/wifi/ssid");
                        std::string wifiSSID = "";
                        if(confWifiSSID_var) wifiSSID = confWifiSSID_var.value();

                        // Fix the WiFi networks option list
                        std::string wifiStr = "";
                        for (auto & element : availableWifiNetworks) {
                            wifiStr += "<option>" + element + "</option>";
                        }

                        // WiFi password
                        auto confWifiPwd_var = tally::settings::query<std::string>("/network/wifi/pwd");
                        std::string wifiPwd = "";
                        if(confWifiPwd_var) wifiPwd = confWifiPwd_var.value();

                        // srcid
                        auto srcId_var = tally::settings::query<int>("/tally/srcId");
                        int srcId = 1;
                        if(srcId_var) srcId = srcId_var.value() + 1;

                        // smart mode
                        auto smartMode_var = tally::settings::query<bool>("/tally/smartMode");
                        bool smartMode = true;
                        if(smartMode_var) smartMode = smartMode_var.value();

                        // Brightness
                        auto brightness_var = tally::settings::query<int>("/board/led/0/brightness");
                        int brightness = 10;
                        if(brightness_var) brightness = brightness_var.value();

                        // Invert led
                        auto invertled_var = tally::settings::query<bool>("/board/led/0/invert");
                        bool invertled = false;
                        if(invertled_var) invertled = invertled_var.value();

                        // Manual confid
                        auto manualConfig_var = tally::settings::query<bool>("/network/wifi/useDHCP");
                        bool manualConfig = false;
                        if(manualConfig_var) manualConfig = !manualConfig_var.value();

                        // Tally IP
                        auto tallyIP_var = tally::settings::query<IPAddress>("/network/wifi/address");
                        std::string tallyIP = "";
                        if(tallyIP_var) tallyIP = tallyIP_var.value().toString().c_str();

                        // Tally netmask
                        auto tallyNetmask_var = tally::settings::query<IPAddress>("/network/wifi/netmask");
                        std::string tallyNetmask = "";
                        if(tallyNetmask_var) tallyNetmask = tallyNetmask_var.value().toString().c_str();

                        snprintf(parsedPage, 5000, managementPage, targetIP.c_str(), wifiSSID.c_str(), wifiStr.c_str(), wifiPwd.c_str(), srcId, smartMode ? "checked" : "", brightness, invertled ? "checked" : "", manualConfig ? "checked" : "", tallyIP.c_str(), tallyNetmask.c_str());
                        reply.send(parsedPage); 

                        free(parsedPage);
                        
                    } else {
                        String tmp = httpRequest.getResource()[0];
                        tmp.replace("update?", "");

                        std::string token;
                        std::istringstream tokenStream(std::string(tmp.c_str()));
                        bool hasUpdates = false;
                        bool wantsSmartMode = false;
                        bool wantsDHCP = true;
                        bool wantsInvertLed = false;
                        while (std::getline(tokenStream, token, '&')){
                            
                            int splitter = token.find('=');
                            if(splitter == -1) {
                                Serial.printf("Error: malformed token %s\n", token.c_str());
                                continue;
                            }
                            auto setting = token.substr(0, splitter);
                            auto value = token.substr(splitter + 1);
                            if(setting == "srcid") tally::settings::update("/tally/srcId", atoi(value.c_str()) - 1);
                            else if(setting == "gsip") tally::settings::update("/target/gostream/address", value);
                            else if(setting == "ssid") tally::settings::update("/network/wifi/ssid", value);
                            else if(setting == "wpwd") tally::settings::update("/network/wifi/pwd", value);
                            else if(setting == "smart") wantsSmartMode = true;
                            else if(setting == "manual") wantsDHCP = false;
                            else if(setting == "tip") tally::settings::update("/network/wifi/address", value);
                            else if(setting == "tnm") tally::settings::update("/network/wifi/netmask", value);
                            else if(setting == "brightness") tally::settings::update("/board/led/0/brightness", value);
                            else if(setting == "invertled") wantsInvertLed = true; 
                            else {
                                Serial.printf("Error: unknown setting %s\n", setting.c_str());
                                continue;
                            }
                            hasUpdates = true;
                        }
                        if(hasUpdates) {
                            tally::settings::update("/tally/smartMode", wantsSmartMode);
                            tally::settings::update("/network/wifi/useDHCP", wantsDHCP);
                            tally::settings::update("/board/led/0/invert", wantsInvertLed);
                            tally::settings::commit();
                            restart();
                        }
                    }
                }
            }
        }
    }
}