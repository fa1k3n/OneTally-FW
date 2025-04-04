#include <SerialCommands.h>
#include <string>

namespace tally {
  namespace serial {
    void init(void);
    void read(void);

    void Println(const __FlashStringHelper* str);
    void Println(std::string str);
    void Print(const __FlashStringHelper* str);
    void Print(std::string str);
  }
}