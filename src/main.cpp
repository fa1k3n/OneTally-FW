#include <SPI.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include "GoStream.hpp"
#include "OBS.hpp"

#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include "tally-led.hpp"
#include "tally-webui.hpp"
#include <SPIFFS.h>
#include <set>

WiFiClient client, webclient;
typedef module::OneTallyModule* ModuleHandle_t;
ModuleHandle_t internal = nullptr;
ModuleHandle_t switcher = nullptr;
QueueHandle_t updateQueue = nullptr;

TaskHandle_t ledTask;

typedef enum {
  INACTIVE,
  SEARCHING,
  CONNECTING,
  CONNECTED,
  CONFIGURATION
} tStatus;

tStatus status = INACTIVE;

bool smartModeInitialized = false;
JsonArray triggers;

void initializeDevice() {
  if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
            return;
  }

  tally::settings::init();
  triggers = tally::settings::query<JsonArray>("/triggers").value();
  tally::led::init();
  tally::serial::init();
}

void updateTally(module::OneTallyModule* module) {
  bool isUpdated = false;
  for(auto trigger : triggers) { 
    auto event = trigger["event"].as<String>();
    auto colour = std::strtoul(trigger["colour"].as<String>().c_str(), NULL, 16);
    auto brightness = trigger["brightness"].as<uint8_t>();

    if ((event == "connecting" && status == CONNECTING) || 
        (event == "configuration" && status == CONFIGURATION)) {
      tally::led::show(0, colour, brightness);
      isUpdated = true;
      break;
    } else if(module != nullptr && module->handleTrigger(trigger)) { 
      tally::led::show(0, colour, brightness);
      isUpdated = true;
      break;
    } 
    if(isUpdated) break;
  }

  if(!isUpdated) tally::led::clear();
}

bool setUpWiFi(int maxTries) {
  auto ssid = tally::settings::query<std::string>("/network/wifi/ssid");
  auto pwd = tally::settings::query<std::string>("/network/wifi/pwd");
  auto manualCfg =  tally::settings::query<bool>("/network/wifi/manualCfg");

  if(!ssid || !pwd || !manualCfg) return false;

  if(ssid.value().length() < 5) {
    tally::serial::Println(F("Warning: SSID not set, entering configuration mode"));
    tally::settings::update("/state/status", "configuration");
    status = CONFIGURATION;
    updateTally(internal); 
    return true;
  }

  tally::settings::update("/state/status", "searching");
  status = SEARCHING;
  updateTally(internal); 
  if(manualCfg.value()) {
    const auto tallyAddress = tally::settings::query<IPAddress>("/network/wifi/address").value();  
  }
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.begin(ssid.value().c_str(), pwd.value().c_str());

  tally::serial::Print(F("\nConnecting to WiFi AP."));
  int numTries = 0;
  while(WiFi.status() != WL_CONNECTED && numTries ++ < maxTries){
      tally::serial::Print(".");
      delay(1000);
  }
  if(WiFi.status() == WL_CONNECTED) {
    tally::serial::Println(F("OK"));
    tally::settings::update("/state/status", "connecting");
    status = CONNECTING;
    updateTally(internal); 
    if(!manualCfg.value())
      tally::settings::update("/state/dhcpAddress", WiFi.localIP().toString().c_str());
  } else {
    tally::serial::Println(F("ERROR"));
    return false;
  }

  return true;
}

void connect() {
  setUpWiFi(5);  
  if(WiFi.status() != WL_CONNECTED) {
    // Setup AP
    tally::settings::update("/state/status", "configuration");
    status = CONFIGURATION;
    updateTally(internal); 
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.softAPConfig(IPAddress(192, 168, 172, 1), IPAddress(192, 168, 172, 1), IPAddress(255, 255, 255, 0));
    if(WiFi.softAP("OneTally")) {
      tally::serial::Print("OneTally AP is ready. WebUi can be found at ");
      tally::serial::Println(WiFi.softAPIP().toString().c_str());
    }
   
  } else {
    switcher = (module::OneTallyModule*) new module::GoStream(tally::settings::query<IPAddress>("/network/targetAddress").value(), 19010, &updateQueue);
  }
   tally::webui::init();
}

void tallyWorker(void *pvParameters) {
  while(1) {
    module::OneTallyModule* module = switcher;
    if (status == CONNECTED) {
      xQueueReceive(updateQueue, module, portMAX_DELAY);
    }
    updateTally(module);
    vTaskDelay(200 / portTICK_RATE_MS);//delay(2000);
    //taskYIELD();
  }
}

void serialWorker(void *pvParameters) {
  while(1) {
    tally::serial::read();
    vTaskDelay(200 / portTICK_RATE_MS);//delay(200);
  }
}

void batteryWorker(void *pvParameters) {
  while(1) {
    int digitalValue = analogRead(GPIO_NUM_34);// read the value from the analog channel
    float Aout = digitalValue * (3.20 / (4096.00 - 1.00));
    Serial.println(digitalValue);
    delay(1000);
  }
}
void setup() {
  Serial.begin(115200);
  initializeDevice();
  updateQueue = xQueueCreate(5, sizeof(bool));
  xTaskCreatePinnedToCore(
    tallyWorker, "TallyWorker", 2048, NULL, 1, &ledTask, 1);
  xTaskCreatePinnedToCore(
    serialWorker, "SerialWorker", 10000, NULL, 1, NULL, 0);
  connect();
}

void restart() {
  // Notify pending shutdown
  tally::serial::Println("Info: tally is preparing to restart");

  // Stop switcher
  if(switcher && switcher->started()) switcher->stop();

  // clear leds
  tally::led::clear();

  delay(300);
  tally::serial::Println("Info: tally is restarting");

  ESP.restart();
}

void loop() { 
  // Check that we are connected 
  if(switcher) {  
    if(!switcher->started()) { //} && state != "configuration") {
      if(switcher->start(&client)) {
        tally::settings::update("/state/status", "connected");
        status = CONNECTED;
      } else {
        tally::settings::update("/state/status", "connecting");
        status = CONNECTING;
      }
      updateTally(internal);
      //xQueueSend(updateQueue, nullptr, portMAX_DELAY);
    } 
  }
  vTaskDelay(200 / portTICK_RATE_MS);
}
