#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include <string.h>

namespace tally {
  namespace serial {
    char serial_command_buffer_[128];
    SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

     void printHelp(SerialCommands* sender) {
        sender->GetSerial()->println("GoTally serial interface commands");
        sender->GetSerial()->println("q [path] - queries for setting value with path. q / will show all settings");
        sender->GetSerial()->println("u [path] [value] - updates setting at path with value");
        sender->GetSerial()->println("c - commits current settings to persistent memory");
        sender->GetSerial()->println("l - load nonpersisten settings to values from persistent memory");
        sender->GetSerial()->println("r - restarts the tally");
        sender->GetSerial()->println("? - prints help menu");
     }

    //This is the default handler, and gets called when no other command matches. 
    void cmd_unrecognized(SerialCommands* sender, const char* cmd)
    {
      sender->GetSerial()->print("Unrecognized command [");
      sender->GetSerial()->print(cmd);
      sender->GetSerial()->println("]");
      printHelp(sender);
    }

    // Query a parameter
    void cmd_query(SerialCommands* sender)
    {
        char* path = sender->Next();
        if (path == NULL)
        {
            sender->GetSerial()->println(F("Error: Missing parameter argument"));
            return;
        }

        JsonVariant value;
        if(!tally::settings::query(path, value)) {
            sender->GetSerial()->println(F("Error: Missing parameter argument"));
            return;
        }
        serializeJsonPretty(value, *sender->GetSerial());
        Serial.println("");
    }

     // Update a parameter
    void cmd_update(SerialCommands* sender)
    {
        char* path = sender->Next();
        if (path == NULL) {
            sender->GetSerial()->println(F("Error: Missing parameter argument"));
            return;
        }

        char* value = sender->Next();
        if (value == NULL) {
            sender->GetSerial()->println(F("Error: Missing value argument"));
            return;
        }

        if(!tally::settings::update(path, std::string(value))) {
            sender->GetSerial()->printf("Error: %s\n", tally::settings::lastError());
            return;
        }
        sender->GetSerial()->println(F("OK"));
    }

    void cmd_commit(SerialCommands* sender)
    {
        if(tally::settings::commit())
            sender->GetSerial()->println(F("OK"));
        else 
            sender->GetSerial()->printf("Error: %s\n", tally::settings::lastError());
    }

    void cmd_load(SerialCommands* sender)
    {
        tally::settings::load();
        sender->GetSerial()->println(F("OK"));
    }

    void cmd_restart(SerialCommands* sender)
    {
        ESP.restart();
    }

    void cmd_help(SerialCommands* sender)
    {
        printHelp(sender);
    }

    SerialCommand cmd_query_("q", cmd_query);
    SerialCommand cmd_update_("u", cmd_update);
    SerialCommand cmd_commit_("c", cmd_commit);
    SerialCommand cmd_help_("?", cmd_help);
    SerialCommand cmd_load_("l", cmd_load);
    SerialCommand cmd_restart_("r", cmd_restart);
    
    void init(void) {
        Serial.println(F("Welcome to GoTally serial interface"));
        serial_commands_.SetDefaultHandler(cmd_unrecognized);
        serial_commands_.AddCommand(&cmd_query_);
        serial_commands_.AddCommand(&cmd_update_);
        serial_commands_.AddCommand(&cmd_commit_);
        serial_commands_.AddCommand(&cmd_load_);
        serial_commands_.AddCommand(&cmd_restart_);
        serial_commands_.AddCommand(&cmd_help_);
    }

    void read(void) {
      serial_commands_.ReadSerial();
    }

  }
}