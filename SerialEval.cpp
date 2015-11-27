#include "SerialEval.h"
#include <vector>

// This is some stuff to fix issues with the Teensy library vector implementation where
// it throws warnings or fails to link if there is no __throw_bad_alloc()
// or __throw_length_error() functions.

namespace std {
  void __throw_bad_alloc() {
    Serial.println("Unable to allocate memory");
    while(1);
  }
  void __throw_length_error(char const *e) {
    Serial.print("Length Error :");
    Serial.println(e);
    while(1);
  }
}

struct Command {
  String name;
  String description;
  handler_t *handler;
};

std::vector<Command> CommandVector;

void evaluateCommand(String commandstring) {
  // If commandstring is empty, return without doing anything. No point in
  // evaluating an empty command.
  
  if (commandstring.length() == 0) return;
  
  std::vector<String> parsed_argv;

  // Get the index of the first space. This should be after the command name.
  int spaceIndex = commandstring.indexOf(' ');

  // Until there are no more spaces, keep adding the char* from the commandstring
  // as new elements of the char* vector parsed_argv.
  while (spaceIndex != -1) {
    parsed_argv.push_back(commandstring.substring(0, spaceIndex));
    commandstring = commandstring.substring(spaceIndex + 1);
    spaceIndex = commandstring.indexOf(' ');
  }

  // This either pushes the very last portion of the command into the final argv,
  // or in the case where there were no spaces in the command just puts the full
  // command into argv[0].
  
  parsed_argv.push_back(commandstring);

  // Flag to identify if the command was handled.
  
  boolean CommandHandled = false;

  // After parsing the string into parsed_argv, search the CommandVector of registered
  // commands for a match between the Command name and the argv[0] string. First check
  // for the special command "ListCommands".
  
  if (parsed_argv[0] == "ListCommands") {
    for (unsigned int i=0; i<CommandVector.size(); i++) {
      Serial.println(CommandVector[i].name + " - " + CommandVector[i].description);
      CommandHandled = true;
    }
  }
  
  else {
    for (unsigned int i=0; i<CommandVector.size(); i++) {
      if (CommandVector[i].name == parsed_argv[0]) {
        CommandVector[i].handler(parsed_argv);
        CommandHandled = true;
      }
    }
  }
  
  if (!CommandHandled) Serial.println("ERROR: No such command - " + parsed_argv[0]);
}

void registerCommand(String name, handler_t *handler, String description) {
  Command newCommand;
  newCommand.name = name;
  newCommand.handler = handler;
  newCommand.description = description;
  CommandVector.push_back(newCommand);
}

// For now, I'm going to try using a vector of chars instead of a std::string to see if it
// works well. Doing this using String objects and concatenate causes reliability
// issues for unclear reasons, but possibly too much overhead.

std::vector<char> commandstring;

void checkSerial(void) {
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();
    
    // If the byte is a carriage return or newline (since I can't guarantee which if either will come
    // first), then send the command previously read to evaluateCommand() and clear the commandstring
    // buffer. This has the convenient effect of rendering irrelevant whether LabView or other such
    // GUI sends something reasonable for a termination character, as long as it sends *something*.
    
    if ((incomingByte == 0x0D) || (incomingByte == 0x0A)) {

      // This tests that there's a string to return in the buffer. If not, ignore. This is both
      // to avoid testing when it's an empty command, and also to deal with sequential CR/NL that
      // can happen with some GUIs sending serial data.
      
      if (commandstring.size() != 0) {
        // Append the termination character to the command string.
        commandstring.push_back('\0');
  
        // Write a newline to clean up echoing.
        Serial.write(0x0D);
        Serial.write(0x0A);
  
        // Evaluate the data.
        evaluateCommand(commandstring.data());
      }
      commandstring.clear();
    }

    // If the byte is a backspace, remove the previously appended char if the length is non-zero.
    else if (incomingByte == 0x7F) {
      if (commandstring.size() > 0) {
        commandstring.pop_back();
        Serial.write(0x7F);
      }
    }
    
    // If the byte is not a carriage return, and is a normal ASCII character, put it onto the commandstring.
    else if ((incomingByte >= 0x20) && (incomingByte <=0x7E)) {
      commandstring.push_back(incomingByte);
      Serial.write(incomingByte);
    }
  }
}

int _parse_float(std::vector<String> argv, int paramnum, float *flt) {
  if (paramnum < 0) {
    Serial.println("ERROR: In Function Call, invalid argument number.");
    return -1;
  }
  unsigned int i = paramnum;
  if (!(0 <= i && i < argv.size())) {
    Serial.println("ERROR: Missing argument " + String(i) + ".");
    return -1;
  }
  char *end;
  *flt = strtof(argv[i].c_str(), &end);
  if (errno == ERANGE) {
    Serial.println("ERROR: Argument " + String(i) + " - (float) not in range.");
    return -1;
  }
  if (end[0] != '\0') {
    Serial.println("ERROR: Argument " + String(i) + " - invalid (float).");
    return -1;
  }
  return 0;
}

int _parse_int(std::vector<String> argv, int paramnum, int *num) {
  if (paramnum < 0) {
    Serial.println("ERROR: In Function Call, invalid argument number.");
    return -1;
  }
  unsigned int i = paramnum;
  if (!(0 <= i && i < argv.size())) {
    Serial.println("ERROR: Missing argument " + String(i) + ".");
    return -1;
  }
  char *end;
  long numl = strtol(argv[i].c_str(), &end, 0);
  *num = numl;
  if (errno == ERANGE || numl > INT_MAX || numl < INT_MIN) {
    Serial.println("ERROR: Argument " + String(i) + " - (int) not in range.");
    return -1;
  }
  if (end[0] != '\0') {
    Serial.println("ERROR: Argument " + String(i) + " - invalid (int).");
    return -1;
  }
  return 0;
}

int _parse_string(std::vector<String> argv, int paramnum, String *str) {
  if (paramnum < 0) {
    Serial.println("ERROR: In Function Call, invalid argument number.");
    return -1;
  }
  unsigned int i = paramnum;
  if (!(0 <= i && i < argv.size())) {
    Serial.println("ERROR: Missing argument " + String(i) + ".");
    return -1;
  }
  *str = argv[i];
  return 0;
}

