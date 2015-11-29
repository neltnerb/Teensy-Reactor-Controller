# Teensy-Reactor-Controller
Library and example for creating a parser for the Teensy to do common Serial control.

It relies upon the std::vector library, but this is not strictly necessary if you are using a more memory constrained device like the traditional Arduino. In that case you can replace the vectors with either dynamic or a static length char* array for argv and manually pass argc, but doing it with full vectors made the code a bit easier to follow for me. Should also work with Arduino ARM based chips where there is an implemented vector library, but untested.

This version assumes you're using Serial as your port. Future idea to make this possible to change so that you can use other USARTs.

## Library Usage:

### command(name)

Creates a command function, similar to an interrupt handler, which is called by the checkSerial() command when it receives a matching serial command.

Use this by creating the section:

    command(name) {
      <Code to Execute>
      }

I recommend having these at the bottom of the code and to use a formal prototype at the top due to the added benefit of having a single compact list of all of the commands registered to the interface. But you can do it either way, it's just a function definition that needs to happen before registerCommand() in the file.

Under the hood, this creates a functioned named "name", so you cannot use the chosen name for any other functions. i.e. Do *not* do:

```
command(echo) {
  int value = intArg(1);
  echo(value);
  }

void echo(int value) {
  Serial.println("Received the integer value " + String(value));
  }
```

because although it looks like a function called "command" calling a function "echo" in reality it's a function "echo" trying to call a second defined function "echo".

#### Special Functions inside of a command block

While inside the command(name) { } function definition, there are four special functions which you can use to populate local variables from the arguments in the command string.

##### float floatArg(int i)

Converts the i-th argument into a float (if valid) and returns. For an invalid conversion, this special function will break out of the main command, so do this at the beginning of a command function to check all of the variable conversions before doing work on them. It checks that the argument number is valid and is a valid float. Invalid floats or argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### int intArg(int i)

Converts the i-th argument into an int (if valid) and returns. For an invalid conversion, this special function will break out of the main command, so do this at the beginning of a command function to check all of the variable conversions before doing work on them. It checks that the argument number is valid and is a valid float. Invalid ints or argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### String stringArg(int i)

Converts the i-th argument into a String (if valid) and returns. For an invalid conversion, this special function will break out of the main command, so do this at the beginning of a command function to check all of the variable conversions before doing work on them. It checks that the argument number is valid, but all arguments start as valid String objects so there is no possibility of an invalid String to String conversion. Invalid argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### int numArgs()

Returns the total number of arguments, not including the command itself. i.e. if you send "SetPurge 1" over serial, and then in the registered function you ask numArgs() it will return 1. This allows you to check for a fully properly formatted string with no extra parameters that shouldn't be there.

### void registerCommand(String name, function* name, String description);

In the setup section, you must register each command with (for example):

    registerCommand("CommandName", commandname, "Does a commanding Thing")
   
with previously defined:

    command(commandname) { };

This automatically adds the new Command to a vector of Commands. This is your command registry, and is automatically searched for a match to a command name if a serial packet with a terminal newline or carriage return is present when you run

    checkSerial();

Upon a match, checkSerial() automatically runs the registered function associated with the name.

### void checkSerial()

This checks the Serial interface for new bytes. If there is new data it adds it to a buffer. If it sees a carriage return or newline, it takes the data so far and evaluates it against the registered functions. It properly handles backspaces for interactive control. This should be run with high priority inside the main loop since otherwise the Serial buffer will just fill up instead of being parsed.

The above routine has the advantage of allowing for either newline or carriage returns as a termination character, frequently a point of contention when using LabView or other very high level GUI systems, and allows for you to use backspace to delete typed characters. The test against 0x20 and 0x7E tells it to only store "normal" characters.

If you send the special serial command "ListCommands" it will print back a list of all registered commands along with the descriptions provided when registering. For instance, in you had registered CommandName with:

    registerCommand("CommandName", commandname, "Does a commanding Thing")

sending ListCommands over serial would print:

    ListCommands
    CommandName - Does a commanding Thing

If there is no match for the registered function, it prints back:

    MissingCommand
    ERROR - No such command - MissingCommand