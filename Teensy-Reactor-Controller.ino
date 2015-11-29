/************************************************************************************
 * This example demonstrates the general design of a firmware to accept serial
 * commands and do things like configure interrupts based on external triggers,
 * parse space separated lists of arguments into int, float, and String types,
 * and will automatically search a list of registered commands for a match.
 * 
 * Brian Neltner, Copyright 2015
 * Version 0.1
 * 
 * Requires SerialEval.cpp and SerialEval.h
 */

#include <Arduino.h>
#include <elapsedMillis.h>
#include <vector>
#include "SerialEval.h"

// PurgePin is an output attached to the solenoid valve.
#define PurgePin 3

// StartPin is an input attached to an external start signal and attached to an interrupt to detect the rising edge.
#define StartPin 4

// TriggeredPin is an output that is set high when the StartPin is triggered and goes low after some delay.
#define TriggeredPin 6
#define TriggeredPinDelay 5000

// Prototypes for the commands.
command(setPurge);

// Create a Flag to tell the main loop when the StartPin interrupt has been triggered.
boolean StartPinFlag = false;

// Sets the trigger edge for the StartPin signal. 0 is falling, 1 is rising. I am making it
// an int instead of a define so that you can for instance have a command that changes which
// edge the start pin triggers the interrupt on.
int StartPinEdge = 1;

// This is a trivial interrupt handler for the Start Pin. Generally don't put much in here since it blocks Serial communication.
void StartPinInterrupt() {
  if ((StartPinEdge == 0) && (digitalRead(StartPin) == LOW)) StartPinFlag = true;
  else if ((StartPinEdge == 1) && (digitalRead(StartPin) == HIGH)) StartPinFlag = true;
}

void setup() {
  // Put here any configuration commands to run once when the system starts.
  
  // Activate the USB Serial Port.
  Serial.begin(9600);

  // Configure PurgePin as a digital output.
  pinMode(PurgePin, OUTPUT);
  digitalWrite(PurgePin, LOW);

  // Configure TriggeredPin as a digital output.
  pinMode(TriggeredPin, OUTPUT);
  digitalWrite(TriggeredPin, LOW);

  // Configure the StartPin as an input with the pullup resistor active, and create an interrupt
  // handler so that on a change to the value of the pin it will call StartPinInterrupt().
  pinMode(StartPin, INPUT_PULLUP);
  attachInterrupt(StartPin, StartPinInterrupt, CHANGE);

  // Register the commands created to attach running the function with receiving a starting string over USB Serial.
  // Third argument is a description to be displayed when you send the command ListCommands.
  registerCommand("SetPurge", setPurge, "Set Purge Valve State (0/1)");
}

elapsedMillis elapsed;

void loop() {
  // This checks for serial commands and sends them for evaluation when a carriage return or newline comes in.
  //
  // checkSerial() automatically reads strings from the USB Serial interface by splitting at every space.
  // As such, as with UNIX, you cannot have a command with a space in it. Something like "Purge Valve 0"
  // would be read as the command "Purge" followed by arguments "Valve" and "0" which is probably not what
  // you want. checkSerial() automatically searches the registered functions on receiving either a carriage
  // return OR a newline character to make it a little more likely to work with high level GUIs like LabView
  // where it's often really annoying or obfuscated how to send a specific singular termination character.
  
  checkSerial();

  // The main loop should check for any flags, and do any actions which should be done based on time delays.
  // For instance, here I'm checking for the StartPinFlag which tells me if I should run something based on
  // the interrupt generated when pin 4 sees a rising edge. You can also create a timer using elapsedMillis to
  // check to see if this time through the loop the time is larger than the last time you checked plus a delay.

  if (StartPinFlag) {
    // First you'd clear the existing flag so this only runs once per interrupt.
    StartPinFlag = false;

    // And then you'd do whatever you wanted, be it setting the start time:
    elapsed = 0;
    
    // which assumes a globally defined elapsedMillis named elapsed, or
    // setting some pin:
    digitalWrite(TriggeredPin, HIGH);
  }

  // This example shows a way to used the elapsed time to set the TriggeredPin low after TriggeredPinDelay (ms).
  if (elapsed > TriggeredPinDelay) {
    digitalWrite(TriggeredPin, LOW);
    elapsed =- TriggeredPinDelay;
  }

  // The reason for doing time based stuff like this rather than just with a function like:
  // delay(5000);
  // is that this way the system is free to do other important tasks while waiting for the delay to end.
  
  // For instance, we might have multiple delays running simultaneously for different valves,
  // and should always be checking the Serial.available() for new commands. This speeds up the real
  // responsiveness of a system by many orders of magnitude and lets you extend to almost arbitrarily
  // many commands without huge growing pains.
}

// Place all the actual commands here. The command() macro sets up the real function setPurge with appropriate
// arguments to handle command parsing transparently.

command(setPurge) {
  // A quick check that the number of arguments received is correct.
  // Does not include the command name in the number of arguments.
  if (numArgs() != 1) {
    Serial.println("ERROR: SetPurge expects 1 argument, received " + String(numArgs()) + ".");
    return;
  }
  
  // Collect the first argument to the Command sent over Serial, convert to int, and put into PurgeValveState.
  // intArg, floatArg, and stringArg are special functions that automatically have access to the argument
  // strings received and which will break out of the command if the value can't be parsed. So, if this
  // function fails to properly convert the argument to an int due to it not existing or being improperly
  // formatted, the "if(PurgeValveState == 0) digitalWrite(PurgePin, LOW);" and other subsequent lines do
  // not get called.
  
  int PurgeValveState = intArg(1);

  // Based on the argument to the Serial command, change the ValvePin output.
  if (PurgeValveState == 0) {
    digitalWrite(PurgePin, LOW);
    Serial.println("Purge valve set to OFF.");
  }
  else if (PurgeValveState == 1) {
    digitalWrite(PurgePin, HIGH);
    Serial.println("Purge valve set to ON.");
  }

  // and if the argument isn't 0 or 1, return an error.
  else Serial.println("ERROR: Purge valve state invalid (must be 0 or 1).");
}
