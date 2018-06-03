# Teensy-Reactor-Controller
Library and example for creating a parser for the Teensy to do common Serial control.

It relies upon the std::vector library, but this is not strictly necessary if you are using a more memory constrained device like the traditional Arduino. In that case you can replace the vectors with either dynamic or a static length char* array for argv and manually pass argc, but doing it with full vectors made the code a bit easier to follow for me. Should also work with Arduino ARM based chips where there is an implemented vector library, but untested.

This version assumes you're using Serial as your port. Future idea to make this possible to change so that you can use other USARTs.

In general philosophy, this is intended to parallel the usage of interrupt handlers -- the Serial command is automatically read, triggers an "interrupt" that calls the specified command (but without blocking, so as to allow further Serial reception to occur while parsing), and the only major difference is that the commands have the special power of reading arguments from the thing that triggered the interrupt.

In design philosphy, it is designed to be **extremely defensive**. If the end user types invalid commands, it should not execute any code other than to tell the user that their command was invalid. By example of where this is really important, say you have a command SetPurge which accepts either a 0 or a 1 to indicate on or off. If the end user doesn't read the manual you give them, and tries to do "SetPurge ON", the normal string.toInt() function would return -- zero. Not an error, but zero. So you'd end up setting some valve to off when the user intended it to be on.

These errors are pernicious, hard to anticipate, and so this library works to make it so that everything is checked always and if there is anything unexpected to warn the end user and not execute potentially dangerously misinterpreted commands. It may not be as ideal as anticipating every user error and guessing what they meant, but it puts the burden on the end user to actually send proper commands as you expected them to be sent rather than on you to guess what they'll do wrong.

## Library Usage:

### command(name)

Creates a command function, similar to an interrupt handler, which is called by the checkSerial() command when it receives a matching serial command.

Use this by creating the section:

    command(name) {
      <Code to Execute>
    }

I recommend having these at the bottom of the code and to use a formal prototype at the top due to the added benefit of having a single compact list of all of the commands registered to the interface. But you can do it either way, it's just a function definition that needs to happen before registerCommand() in the file.

Under the hood, this creates a function named "name", so you cannot use the chosen name for any other functions. i.e. **Do not do**:

```
command(echo) {
  int Value = intArg(1);
  echo(Value);
}

void echo(int Value) {
  Serial.println("Received the integer value " + string(Value));
}
```

because although it looks like a function called "command" calling a function "echo" in reality it's a function "echo" trying to call a second defined function "echo".

If you want to do this kind of thing where you have a helper function callable from (for instance) multiple commands, just give the helper function a different name from any command.

**Do do this -- different name for helper function than any command:**
```
command(echo) {
  int Value = intArg(1);
  printval(Value);
}

command(multiecho) {
  int NumberOfArguments = numArgs();
  int Values[NumberOfArguments];
  
  // Note that I first parse all of the ints before using them.
  // This is recommended to ensure that all values are valid
  // before starting to work with them.
  for (int i = 1; i <= NumberOfArguments; i++) Values[i] = intArg(i);

  // And then actually use them.
  for (int i = 0; i < NumberOfArguments; i++) printval(Values[i]);
}

void printval(int Value) {
  Serial.println("Received the integer value " + string(Value));
}
```

Observe also that I demonstrate how to pull all the arguments into the function through the parser before attempting to work with them. In this example, if any of the arguments **aren't** valid ints, it will return an error saying that the offending argument was invalid whereas if you just did:

    for (int i = 1; i <= NumberOfArguments; i++) printval(intArg(i));

it would printval for the arguments up until it sees the invalid one. I think in most cases the behavior where none of the actual function work is done unless *all* arguments used are valid first is preferred to prevent end user errors that are hard to predict from breaking anything important.

#### Special Functions inside of a command block

While inside the command(name) { } function definition, there are four special functions which you can use to populate local variables from the arguments in the command string.

##### float floatArg(int i)

Converts the i-th argument into a float (if valid) and returns. For an invalid conversion, this special function will break out of the main command, so do this at the beginning of a command function to check all of the variable conversions before doing work on them. It checks that the argument number is valid and is a valid float. Invalid floats or argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### int intArg(int i)

Converts the i-th argument into an int (if valid) and returns. For an invalid conversion, this special function will break out of the main command, so do this at the beginning of a command function to check all of the variable conversions before doing work on them. It checks that the argument number is valid and is a valid float. Invalid ints or argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### string stringArg(int i)

Converts the i-th argument into a string (if valid) and returns. For an invalid conversion, this special function will break out of the main command, so do this at the beginning of a command function to check all of the variable conversions before doing work on them. It checks that the argument number is valid, but all arguments start as valid string objects so there is no possibility of an invalid string to string conversion. Invalid argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### int numArgs()

Returns the total number of arguments, not including the command itself. i.e. if you send "SetPurge 1" over serial, and then in the registered function you ask numArgs() it will return 1. This allows you to check for a fully properly formatted string with no extra parameters that shouldn't be there.

### void registerCommand(string name, function* name, string description);

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