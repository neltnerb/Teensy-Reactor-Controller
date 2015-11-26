# Teensy-Reactor-Controller
Library and example for creating a parser for the Teensy to do common Serial control.

It relies upon the std::vector library, but this is not strictly necessary if you are using a more memory constrained device like the traditional Arduino. In that case you can replace the vectors with either dynamic or a static length char* array for argv and manually pass argc, but doing it with full vectors made the code a bit easier to follow for me. To compile for the Arduino Due, it should be possible by just replacing elapsedMillis with a long and manually checking millis().

## Library Usage:

### command(name)

creates a command, under the hood creating the function:

    static void name(std::vector<String> argv)

In this library, argv is a vector of Strings generated when evaluateCommand() is run on a raw command string, but that is hidden from the user of the library. You should never be directly calling name(), and instead call it through the evaluateCommand(String commandstring) function.

Use this by creating the section:

    command(name) {
      <Code to Execute>
      }

I recommend having these at the bottom of the code and to use a formal prototype at the top due to the added benefit of having a single compact list of all of the commands registered to the interface. But you can do it either way, it's just a function definition that needs to happen before registerCommand() in the file.

### registerCommand(String name, name);

In the setup section, you must register each command with (for example):

    registerCommand("CommandName", commandname)
   
with previously defined:

    command(commandname) { };

This automatically adds the new Command to a vector of Commands. This is your command registry, and is automatically searched for a match to a command name when you run

    evaluateCommand(String commandstring)

with a received string you want to parse. On a match, evaluateCommand() automatically runs the registered function associated with the name.

### evaluateCommand(String commandstring)

Takes a given commandstring and passes it to the parser to search for a matching registered command and if there is a match to run the registered function.

My recommendation is to use this with:

    if (Serial.available()) evaluateCommand(Serial.readStringUntil(0x0D));

to tell it to automatically keep filling up the command until it reads a carriage return for a termination character. I have found that in practice this is vastly faster than doing manual byte manipulations.

## Special Functions inside of a command block

Anywhere, you must define a function to tell it what to do when there is a match. In this function there are four special functions which you can use to populate local variables from the arguments in the command string sent to evaluateCommand().

### floatArg(int i, float* var_ptr)

Converts the i-th argument into a float (if valid) and puts it into the pointer var_ptr. It checks that the argument number is valid, that the command has an argument that matches the number, and is a valid float. Invalid floats cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

### intArg(int i, int* var_ptr) and stringArg(int i, String* var_ptr)

Same as floatArg for ints and String variable types. stringArg returns a String object, not a char*.

### numArgs()

Returns the total number of arguments, not including the command itself. i.e. if you send evaluateCommand("SetPurge 1") and then in the registered function you ask numArgs() it will return 1. This allows you to check for a fully properly formatted string with no extra parameters that shouldn't be there.