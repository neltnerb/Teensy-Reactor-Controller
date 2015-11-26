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
  handler_t *handler;
};

std::vector<Command> CommandVector;

void evaluateCommand(String commandstring) {
  std::vector<String> parsed_argv;

  // Get the index of the first space. This should be after the command name.
  int spaceIndex = commandstring.indexOf(' ');

  // Until there are no more spaces, keep adding the char* from the commandstring
  // as new elements of the char* vector parsed_argv.
  while (spaceIndex != -1) {
    parsed_argv.push_back(commandstring.substring(0, spaceIndex));
    spaceIndex = commandstring.indexOf(' ');
  }

  // If there was no command string at all (i.e. just a carriage return with no command)
  // return without trying to parse it.
  if (parsed_argv.size() == 0) return;

  for (unsigned int i=0; i<CommandVector.size(); i++) {
    if (CommandVector[i].name == parsed_argv[0]) {
      CommandVector[i].handler(parsed_argv);
    }
  }
}

void registerCommand(String name, handler_t *handler) {
  Command newCommand;
  newCommand.name = name;
  newCommand.handler = handler;
  CommandVector.push_back(newCommand);
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

