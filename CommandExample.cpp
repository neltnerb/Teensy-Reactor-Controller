/************************************************************************************
 * This example demonstrates the general design of a firmware to accept serial
 * commands and do things like configure interrupts based on external triggers,
 * parse space separated lists of arguments into int, float, and string types,
 * and will automatically search a list of registered commands for a match.
 * 
 * Brian Neltner, Copyright 2018
 */

#include <Arduino.h>
#include <vector>
#include <CommandEval.h>
#include <WiFi.h>

#define ssid "LumenLanSlow"
#define password "lights4life"

// Create the interfaces to monitor. Serial already exists.
WiFiServer server(22222);

// Command Handler Prototypes.
command(setPurge);

void setup() {
  // Activate the USB Serial Port.
  Serial.begin(115200);

  // Register the commands created to attach running the function with receiving a starting string over USB Serial.
  // Third argument is a description to be displayed when you send the command ListCommands.
  registerCommand("SetPurge", setPurge, "Echo back a single argument (unless it is 1).");

  // Initialize wifi.
  Serial.print("Connecting to " + String(ssid));

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

WiFiClient client;

void loop() {
  checkSerial(Serial);
  if (client) checkWiFi(client);
  else client = server.available();
}

// General Structure of a Handler. Check number of arguments, import them, do typechecking using
// return throughout to abort the handler if sanity checking fails with a useful error message. At
// the end, do the actual processing. It is cleaner to use returns to abort than to use a very
// complex nested if statement when possible. Use "reply()" to send a message back on the same
// interface it was received (serial, wifi, ethernet, etc).

command(setPurge) {
  if (numArgs() != 1) {
    return "ERROR: Command expects 1 argument, received ";
  }
  
  String argument = stringArg(1);
  
  if (argument == "1") {
    return "ERROR: You may not hack me Dave.";
  }

  reply(argument);
  return "SUCCESS: Command executed and echoed back your argument.";
}