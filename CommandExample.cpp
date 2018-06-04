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
#include <nvs.h>
#include <nvs_flash.h>

#define STORAGE_NAMESPACE "storage"
String ssid = "Not Configured";
String password = "Not Configured";

// Create the interfaces to monitor. Serial already exists.
WiFiServer server(22222);

// Command Handler Prototypes.
command(setSSID);
command(setPassword);
command(saveConfig);
command(setPurge);

void setup() {
  // Activate the USB Serial Port.
  Serial.begin(115200);

  nvs_handle my_handle;
  esp_err_t err;

  nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);

  size_t datalength;
  err = nvs_get_str(my_handle, "ssid", NULL, &datalength);
  if (err == ESP_OK) { 
    char ssiddata[datalength];
    nvs_get_str(my_handle, "ssid", ssiddata, &datalength);
    ssid = String(ssiddata);
  }
  
  err = nvs_get_str(my_handle, "password", NULL, &datalength);
  if (err == ESP_OK) {
    char passworddata[datalength];
    nvs_get_str(my_handle, "password", passworddata, &datalength);
    password = String(passworddata);
  }

  // Register the commands created to attach running the function with receiving a starting string over USB Serial.
  // Third argument is a description to be displayed when you send the command ListCommands.
  registerCommand("SetSSID", setSSID, "Set the SSID.");
  registerCommand("SetPassword", setPassword, "Set the Password.");
  registerCommand("SaveConfig", saveConfig, "Save the SSID and Password to ROM and reboot.");
  registerCommand("SetPurge", setPurge, "Echo back a single argument (unless it is 1).");

  // Initialize wifi. Send debug information to serial port.
  Serial.print("Connecting to " + String(ssid));

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long starttime = millis();

  // Using a timer here because the WiFi library is buggy and doesn't report failed connection attempts.
  while ((WiFi.status() != WL_CONNECTED) and (millis()<starttime+10000) and (WiFi.status() != WL_CONNECT_FAILED)) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
  }
  else {
    Serial.println("");
    Serial.println("WiFi Connection Failed.");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
  }
}

WiFiClient client;

void loop() {
  delay(500);
  checkSerial(Serial);
  if (client) {
    if (checkWiFi(client) == 1) { // A return code of 1 means it is no longet connected.
      client.stop();
      client = server.available();
    }
  }
  // If the wifi is connected, connect the client to the open socket.
  else if (WiFi.status() == WL_CONNECTED) client = server.available();
  // If it's disconnected, try to connect. This doesn't work because you can't do begin multiple times.
  // else if (WiFi.status() == WL_DISCONNECTED) WiFi.begin(ssid.c_str(), password.c_str());
}

// General Structure of a Handler. Check number of arguments, import them, do typechecking using
// return throughout to abort the handler if sanity checking fails with a useful error message. At
// the end, do the actual processing. It is cleaner to use returns to abort than to use a very
// complex nested if statement when possible. Use "reply()" to send a message back on the same
// interface it was received (serial, wifi, ethernet, etc).

command(setSSID) {
  if (numArgs() != 1) {
    return "ERROR: Sorry, I can only handle SSIDs without spaces right now.";
  }

  String argument = stringArg(1);
  ssid = argument;

  nvs_handle my_handle;
  nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  nvs_set_str(my_handle, "ssid", ssid.c_str());

  return String("SUCCESS: SSID set to " + argument);
}

command(setPassword) {
  if (numArgs() != 1) {
    return "ERROR: Sorry, I can only handle passwords without spaces right now.";
  }

  String argument = stringArg(1);
  password = argument;

  nvs_handle my_handle;
  nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  nvs_set_str(my_handle, "password", password.c_str());

  return String("SUCCESS: Password set to " + argument);
}

command(saveConfig) {
  if (numArgs() != 0) {
    return "ERROR: SaveConfig does not take arguments.";
  }

  reply(String("SSID: ") + ssid + String(" Password: ") + password);
  reply("Rebooting...");
  nvs_handle my_handle;
  nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  nvs_commit(my_handle);
  return "SUCCESS: SSID and Password Saved to ROM, Manual Reboot Required.";
}

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