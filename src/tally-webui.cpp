#include <ArduinoHttpServer.h>
#include <string>
#include <vector>
#include <sstream>

#include "tally-webui.hpp"
#include "tally-settings.hpp"
namespace tally {
    namespace webui {
        WiFiServer* server_;

        const __FlashStringHelper* managementPage = F("<html>\n"
          "\t<head>\n"
          "\t\t<style>\n"
          "\t\t\tsmall {\n"
          "  \t\t\t\tfont-size: smallest;\n"
          "  \t\t\t\tcolor: blue;\n"
          "\t\t\t}\n"
          "\t\t\th1 {\n"
          "  \t\t\t\tcolor: red;\n"
          "\t\t\t}\n"
          "\t\t\t.cfg {\n"
          "  \t\t\t\tbackground-color: tomato;\n"
          " \t\t\t\tpadding: 5px 10px 20px 10px;\n"
          "\t\t\t\tmargin-bottom: 10px\n"
          "\t\t\t}\n"
          "\t\t\t\n"
          "\t\t\tform  { display: table;      }\n"
          "\t\t\tp     { display: table-row; padding: 20px 5px 20px 5px; }\n"
          "\t\t\tlabel { display: table-cell; text-align: right }\n"
          "\t\t\tinput { display: table-cell;  }\n"
          "\t\t\tinput:invalid { border-style: dashed; border-color: red}\n"
          "\t\t</style>\n"
          "\t</head>\n"
          "\t<body onload=\"document.getElementById('manualCfg').hidden = !document.getElementById('manual').checked, document.getElementById('srcid').disabled = document.getElementById('smart').checked;\">\n"
          "\t\t<h1>GoTally<sub><small>WiFi</small></sub> configuration</h1>\n"
          "\t\t<form action=\"/update\">\n"
          "\t\t<div class=\"cfg\">\n"
          "\t\t\t<h2>Network configuration</h2>\n"
          "\t\t\t<p><label for=\"gsip\">GoStream IP</label><input type=\"text\" minlength=\"7\" maxlength=\"15\"  required pattern=\"^((\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d|[1-9]\\d|1\\d\\d|2[0-4]\\d|25[0-5])$\" id=\"gsip\" name=\"gsip\"></p>\n"
          "  \t\t\t<p><label for=\"ssid\">WiFI SSID:</label><input type=\"text\" required id=\"ssid\" name=\"ssid\"></p>\n"
          "  \t\t\t<p><label for=\"wpwd\">WiFI Password:</label><input type=\"password\" required id=\"wpwd\" name=\"wpwd\"></p>\n"
          "\t\t</div>\n"
          "\t\t<div class=\"cfg\" >\n"
          "\t\t\t<h2>Tally configuration</h2>\n"
          "\t\t\t<p>\n"
          "\t\t\t<label for=\"srcid\">Source ID (1-5)</label>\n"
          "\t\t\t<input type=\"number\" max=\"5\" min=\"1\" required id=\"srcid\" name=\"srcid\">\n"
          "\t\t\t<label for=\"smart\">Smart assign</label>\n"
          "\t\t\t<input type=\"checkbox\" id=\"smart\" checked=true name=\"smart\" onclick=\"document.getElementById('srcid').disabled = this.checked;\">\n"
          "\t\t\t</p>\n"
       /*   "\t\t\t<p><label for=\"manual\">Manual configuration</label>\n"
          "\t\t\t<input type=\"checkbox\" id=\"manual\" name=\"Manual\" onclick=\"document.getElementById('manualCfg').hidden = !this.checked;\"></p>\n"
          "\t\t\t\n"
          "\t\t\t<div id=\"manualCfg\">\n"
          "\t\t\t<p><label for=\"tid\">Tally IP</label>\n"
          "\t\t\t<input type=\"text\" minlength=\"7\" maxlength=\"15\" required pattern=\"^(?>(\\d|[1-9]\\d{2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(?1)$\" id=\"tip\" name=\"tip\"></p>\n"
          "  \t\t\t\n"
          "\t\t<p><label for=\"tnmid\">Tally netmask:</label>\n"
          "\t\t\t<input type=\"text\" minlength=\"7\" maxlength=\"15\" required pattern=\"^(?>(\\d|[1-9]\\d{2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(?1)$\" id=\"tnip\" name=\"tnip\" value=\"255.255.255.0\"></p>\n"
          "\t\t\t</div>\n"
          "\t\t</div>\n" */
          "\t\t<input type=\"submit\" value=\"Submit\">\n"
          "\t\t</form>\n"
          "\t</body>\n"
          "</html>\n"
          "");

        bool init(WiFiServer& c) {
            server_ = &c;
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
                        reply.send(managementPage); 
                        
                    } else {
                        String tmp = httpRequest.getResource()[0];
                        tmp.replace("update?", "");

                        std::string token;
                        std::istringstream tokenStream(std::string(tmp.c_str()));
                        bool hasUpdates = false;
                        while (std::getline(tokenStream, token, '&')){
                            
                            int splitter = token.find('=');
                            if(splitter == -1) {
                                Serial.printf("Error: malformed token %s\n", token.c_str());
                                continue;
                            }
                            auto setting = token.substr(0, splitter);
                            auto value = token.substr(splitter + 1);
                            if(setting == "srcid") tally::settings::update("/srcId", atoi(value.c_str()));
                            else if(setting == "gsip") tally::settings::update("/gostream/address", value);
                            else if(setting == "ssid") tally::settings::update("/tally/wifi/ssid", value);
                            else if(setting == "wpwd") tally::settings::update("/tally/wifi/pwd", value);
                            else if(setting == "smart") tally::settings::update("/smartMode", value == "on" ? true : false);
                            else if(setting == "manual") tally::settings::update("/tally/wifi/useDHCP", value == "on" ? true : false);
                            else if(setting == "tip") tally::settings::update("/tally/wifi/address", value);
                            else if(setting == "tnip") tally::settings::update("/tally/wifi/netmask", value);
                            else {
                                Serial.printf("Error: unknown setting %s\n", setting.c_str());
                                continue;
                            }
                            hasUpdates = true;
                        }
                        if(hasUpdates) {
                            tally::settings::commit();
                            ESP.restart();
                        }
                    }
                }
            }
        }
    }
}