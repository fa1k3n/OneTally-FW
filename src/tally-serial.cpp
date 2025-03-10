#include "tally-serial.hpp"
#include "tally-settings.hpp"
#include <string.h>

namespace tally {
  namespace serial {
    char serial_command_buffer_[128];
    SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

    void Prompt_(SerialCommands* sender) {
        Serial.print("OneTally > ");
        Serial.flush();
    }

    void Error_(const __FlashStringHelper* errStr, SerialCommands* sender = &serial_commands_) {
        sender->GetSerial()->print("Error: ");
        sender->GetSerial()->println(errStr);
        Prompt_(sender);
    }

    void Error_(std::string errStr, SerialCommands* sender = &serial_commands_) {
        sender->GetSerial()->print("Error: ");
        sender->GetSerial()->println(errStr.c_str());
        Prompt_(sender);
    }

    void OK_(SerialCommands* sender) {
        sender->GetSerial()->println("OK");
        Prompt_(sender);
    }

    void Println(const __FlashStringHelper* str) {
        serial_commands_.GetSerial()->println(str);
        Prompt_(&serial_commands_);
    }

    void Println(std::string str) {
        serial_commands_.GetSerial()->println(str.c_str());
        Prompt_(&serial_commands_);
    }

    void Print(const __FlashStringHelper* str) {
        serial_commands_.GetSerial()->print(str);
    }

    void Print(std::string str) {
        serial_commands_.GetSerial()->print(str.c_str());
    }

    void printHelp(SerialCommands* sender) {
        sender->GetSerial()->println("OneTally serial interface commands");
        sender->GetSerial()->println("q [path] - queries for setting value with path. q / will show all settings");
        sender->GetSerial()->println("u [path] [value] - updates setting at path with value");
        sender->GetSerial()->println("c - commits current settings to persistent memory");
        sender->GetSerial()->println("l - load nonpersisten settings to values from persistent memory");
        sender->GetSerial()->println("r - restarts the tally");
        sender->GetSerial()->println("d - loads factory settings");
        sender->GetSerial()->println("? - prints help menu");
    }

    //This is the default handler, and gets called when no other command matches. 
    void cmd_unrecognized(SerialCommands* sender, const char* cmd)
    {
        Error_(F("Unrecognized command"), sender);
        printHelp(sender);
        Prompt_(sender);
    }

    // Query a parameter
    void cmd_query(SerialCommands* sender)
    {
        char* path = sender->Next();
        if (path == NULL)
        {
            Error_(F("Missing parameter argument"), sender);
            return;
        }

        auto var = tally::settings::query<JsonVariant>(path);
        if(!var) {
            Error_(F("Missing parameter argument"), sender);
            return;
        }
        serializeJsonPretty(var.value(), *sender->GetSerial());
        Serial.println("");
        Prompt_(sender);
    }

     // Update a parameter
    void cmd_update(SerialCommands* sender)
    {
        char* path = sender->Next();
        if (path == NULL) {
            Error_(F("Missing parameter argument"), sender);
            return;
        }

        char* value = sender->Next();
        if (value == NULL) {
            Error_(F("Missing value argument"), sender);
            return;
        }

        if(!tally::settings::update(path, std::string(value))) {
            Error_(tally::settings::lastError(), sender);
            return;
        }
        OK_(sender);
    }

    void cmd_commit(SerialCommands* sender)
    {
        if(tally::settings::commit())
            OK_(sender);
        else 
            Error_(tally::settings::lastError(), sender);
    }

    void cmd_load(SerialCommands* sender)
    {
        char* reallySure = sender->Next();
        if (tally::settings::hasChanges() && (!reallySure || *reallySure != '!'))
        {
            Error_(F("There are unsaved settings that will be overwritted. Save settings or add ! after the command to disregard this error message"), sender);
            return;
        }
        tally::settings::load();
        OK_(sender);
    }

    void cmd_restart(SerialCommands* sender)
    {
        char* reallySure = sender->Next();
        if (tally::settings::hasChanges() && (!reallySure || *reallySure != '!'))
        {
            Error_(F("There are unsaved settings that will be overwritted. Save settings or add ! after the command to disregard this error message"), sender);
            return;
        }
        ESP.restart();
    }

    void cmd_default(SerialCommands* sender)
    {
        char* reallySure = sender->Next();
        if (!reallySure || *reallySure != '!')
        {
            Error_(F("To reset the settings you need to add a ! after the command"), sender);
            return;
        }
        tally::settings::reset();
        ESP.restart();
    }

    void cmd_help(SerialCommands* sender)
    {
        printHelp(sender);
        Prompt_(sender);
    }

    SerialCommand cmd_query_("q", cmd_query);
    SerialCommand cmd_update_("u", cmd_update);
    SerialCommand cmd_commit_("c", cmd_commit);
    SerialCommand cmd_help_("?", cmd_help);
    SerialCommand cmd_load_("l", cmd_load);
    SerialCommand cmd_restart_("r", cmd_restart);
    SerialCommand cmd_default_("d", cmd_default);
    
    void init(void) {
        Serial.println(F("Welcome to OneTally serial interface"));
        Prompt_(&serial_commands_);
        serial_commands_.SetDefaultHandler(cmd_unrecognized);
        serial_commands_.AddCommand(&cmd_query_);
        serial_commands_.AddCommand(&cmd_update_);
        serial_commands_.AddCommand(&cmd_commit_);
        serial_commands_.AddCommand(&cmd_load_);
        serial_commands_.AddCommand(&cmd_restart_);
        serial_commands_.AddCommand(&cmd_help_);
        serial_commands_.AddCommand(&cmd_default_);
    }

    void read(void) {
      serial_commands_.ReadSerial();
    }

  }
}