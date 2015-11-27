# Teensy-Reactor-Controller
Library and example for creating a parser for the Teensy to do common Serial control.

It relies upon the std::vector library, but this is not strictly necessary if you are using a more memory constrained device like the traditional Arduino. In that case you can replace the vectors with either dynamic or a static length char* array for argv and manually pass argc, but doing it with full vectors made the code a bit easier to follow for me. Should also work with Arduino ARM based chips where there is an implemented vector library, but untested.

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

#### Special Functions inside of a command block

Anywhere, you must define a function to tell it what to do when there is a match. In this function there are four special functions which you can use to populate local variables from the arguments in the command string sent to evaluateCommand().

##### floatArg(int i, float* var_ptr)

Converts the i-th argument into a float (if valid) and puts it into the pointer var_ptr. It checks that the argument number is valid and is a valid float. Invalid floats or argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### intArg(int i, int* var_ptr)

Converts the i-th argument into an int (if valid) and puts it into the pointer var_ptr. It checks that the argument number is valid and is a valid int. Invalid ints or argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

#####stringArg(int i, String* var_ptr)

Converts the i-th argument into a String (if valid) and puts it into the pointer var_ptr. It checks that the argument number is valid, but all arguments start as valid String objects so there is no possibility of an invalid String to String conversion. Invalid argument numbers cause the function to return immediately, printing error messages to the Serial port for debugging either the code error or the command error.

##### numArgs()

Returns the total number of arguments, not including the command itself. i.e. if you send evaluateCommand("SetPurge 1") and then in the registered function you ask numArgs() it will return 1. This allows you to check for a fully properly formatted string with no extra parameters that shouldn't be there.

### registerCommand(String name, name, String description);

In the setup section, you must register each command with (for example):

    registerCommand("CommandName", commandname, "Does a commanding Thing")
   
with previously defined:

    command(commandname) { };

This automatically adds the new Command to a vector of Commands. This is your command registry, and is automatically searched for a match to a command name when you run

    evaluateCommand(String commandstring)

with a received string you want to parse. On a match, evaluateCommand() automatically runs the registered function associated with the name.

### evaluateCommand(String commandstring)

Takes a given commandstring and passes it to the parser to search for a matching registered command and if there is a match to run the registered function.

My recommended method for capturing and parsing serial data is shown in the example, and is:

```
std::vector<char> commandstring;

void loop() {
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
```

placed inside the main loop. You will need the global character vector defined outside as in the example code.

I use here a char vector instead of String to make it faster and give access to push_back and pop_back along with clear which aren't present in String but provide quick methods for appending single characters. It also lets me have arbitrarily long strings, limited only by the device RAM. The library itself relies on the vector library being available for your platform, so this adds no additional requirements. However, this variable length data does introduce the possibility of crashing the device if you try to send it too much data without clearing it via carriage return or newline.

The above recommended routine also has the advantage of allowing for either newline or carriage returns as a termination character, frequently a point of contention when using LabView or other very high level GUI systems, and allows for you to use backspace to delete typed characters. The test against 0x20 and 0x7E tells it to only store "normal" characters.

If you send the special serial command "ListCommands" it will print back a list of all registered commands along with the descriptions provided when registering. For instance, in the above it would print:

    ListCommands
    CommandName - Does a commanding Thing

If there is no match for the registered function, it prints back:

    MissingCommand
    ERROR - No such command - MissingCommand