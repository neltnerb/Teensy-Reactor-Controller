#pragma once

#include <Arduino.h>
#include <errno.h>
#include <limits.h>
#include <vector>

// Expects argv to be present, handled by the command macro when defining a command. Uses a GCC
// expression (https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html) to appear to return a
// value despite it being a macro.

#define floatArg(i)  ({ float var_ptr; if (-1 == _parse_float(argv, i, &var_ptr)) return; var_ptr; })
#define intArg(i)  ({ int var_ptr; if (-1 == _parse_int(argv, i, &var_ptr)) return; var_ptr; })
#define stringArg(i)  ({ String var_ptr; if (-1 == _parse_string(argv, i, &var_ptr)) return; var_ptr; })
#define numArgs() (argv.size()-1)

#define command(name)  static void name(std::vector<String> argv)

typedef void (handler_t)(std::vector<String> argv);

int _parse_float(std::vector<String> argv, int i, float *flt);
int _parse_int(std::vector<String> argv, int i, int *num);
int _parse_string(std::vector<String> argv, int i, String *str);
void registerCommand(String name, handler_t *handler, String description);
//void evaluateCommand(String commandstring);
void checkSerial(void);
