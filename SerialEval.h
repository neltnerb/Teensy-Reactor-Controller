#pragma once

#include <Arduino.h>
#include <errno.h>
#include <limits.h>
#include <vector>

// Expects argv to be present, handled by the command macro when defining a command.
#define floatArg(i, var_ptr)  do { if (-1 == _parse_float(argv, i, var_ptr)) return; } while (0)
#define intArg(i, var_ptr)  do { if (-1 == _parse_int(argv, i, var_ptr)) return; } while (0)
#define stringArg(i, var_ptr)  do { if (-1 == _parse_string(argv, i, var_ptr)) return; } while (0)
#define numArgs() (argv.size()-1)

#define command(name)  static void name(std::vector<String> argv)

typedef void (handler_t)(std::vector<String> argv);

int _parse_float(std::vector<String> argv, int i, float *flt);
int _parse_int(std::vector<String> argv, int i, int *num);
int _parse_string(std::vector<String> argv, int i, String *str);
void registerCommand(String name, handler_t *handler, String description);
void evaluateCommand(String commandstring);
