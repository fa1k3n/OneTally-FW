#include <ArduinoJson.h>
#include <stdbool.h>
#include "WiFiClient.h"

// Transition sources
#define USK 1
#define DSK 2
#define BKG 4

// USK stuff
#define LUMA 0
#define CHROMA 1
#define KEYPATTERN 2
#define PIP 3

namespace comm {

  typedef struct stateT_ {
    int pgmId;
    int pvwId;
    int ssrcSrc1Id;
    int ssrcSrc2Id;
    int ssrcBkgId;
    int pipSrcId;

    int uskType; 
    int uskFillSrcId[4];// = {0, 0, 0, 0};
    int lumaKeySrcId; // = 0;
    bool uskActive; // = false;
    bool uskTied; // = false;

    bool transitionOngoing; // = false;
    int transitionSource;
  } stateT;


  bool init(WiFiClient& c);
  bool connect(IPAddress address, uint16_t port);
  bool sendMessage(String message);
  bool receiveMessage(JsonDocument* doc);
  void handleMessage(JsonDocument* doc);
  stateT* getState();
  bool checkForUpdates();
}