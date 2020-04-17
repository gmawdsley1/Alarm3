/*
  Slave Communication Functions
  
  Provides functions used to monitor alarm inputs.
  Used:  http://www.adafruit.com/blog/2009/10/20/example-code-for-multi-button-checker-with-debouncing/
*/
#include <Arduino.h>
#include "SlaveCommunicationsFunctions.h"
#include "GSMFunctions.h"
#include "GSMSoftwareSerial.h"
#include "SerialGSM.h"
#include "DiagnosticFunctions.h"
#include "MegaMaster.h"
#include "ContactManagementFunctions.h"

#include "MonitoringFunctions.h"

/**
* Read switch input values and updates the status arrays (justPressed, justReleased, pressed)
* Also handles debouncing of inputs.
*/
void checkInputs(){
  byte index;

  // Handle rollover properly
  if ((unsigned long)(millis() - lastTime) < DEBOUNCE){
    // Not enough time has passed to debounce
    return; 
  }

  // DEBOUNCE milliseconds have passed, reset the timer
  lastTime = millis();

  // Loop over all the buttons
  for (index = 0; index < NUMINPUTS; index++) {

    // Clear all previous flags
    justPressed[index] = 0;
    justReleased[index] = 0;

    // Get the current state
    currentState[index] = digitalRead(inputs[index]->pin);

    if (currentState[index] == previousState[index]) {
      if ((pressed[index] == LOW) && (currentState[index] == LOW)) {
        // Button has just been pressed
        justPressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentState[index] == HIGH)) {
        // Button has just been released
        justReleased[index] = 1;
      }

      // This is a pullup, digital HIGH means NOT pressed
      pressed[index] = !currentState[index];
    }

    // Keep a running tally of the inputs
    previousState[index] = currentState[index];
  }
}

/**
* Check if any input is, was or will be in alarm state.
* Returns: true or false
*/
boolean inAlarmState(){
  for (byte i = 0; i < NUMINPUTS; i++){
    if (justPressed[i] || pressed[i] || justReleased[i]){
      return true;
    }
  }
  return false;
}
