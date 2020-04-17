/*
  Diagnostic Functions
  
  Provides functions for error detection and correction (resetting)
*/
#include <Arduino.h>
#include "SlaveCommunicationsFunctions.h"
#include "GSMFunctions.h"
#include "GSMSoftwareSerial.h"
#include "SerialGSM.h"
#include "MegaMaster.h"
#include "ContactManagementFunctions.h"
#include "MonitoringFunctions.h"
#include "Sounds.h"
#include "DiagnosticFunctions.h"
#include "WatchdogFunctions.h"



static boolean doneSoftReset = false;

/**
* Check for GSM errors and reset if required
*/
void checkGSMProblems(){
  
  // Handle GSM Errors
  if(cell.GetErrorCode() != 0){
    Serial.print(F("Error has occured! Restarting. Code: "));    
    Serial.println(cell.GetErrorCode());    
    doIncrementalReset();
  }
  
  // Handle Timeouts
  if(numTimeouts > 3){
    Serial.print(numTimeouts);
    Serial.println(F(" operations have timed out! Restarting."));
    doIncrementalReset();
  }
  
  // Handle unexpected status codes
  if(cell.GetGSMStatus() == 1 || cell.GetGSMStatus() == 10){
    // Cell has reset itself (Status 1 or 10)
    // Call bootGSMShield() to ensure that the cell reboots properly.
    // This also ensures that FwdSMS2Serial is called, which allows SMS messages to be recieved.
    bootGSMShield();
  }
  else if(cell.GetGSMStatus() == 7){
    // Unrecoverable Error - Status 7
    Serial.println("Cell Status 7. Restarting.");
    doIncrementalReset();
  }
}

/**
* Gracefully resets alarm by first attempting a GSM soft reset.
* If the error recurs, a full hardware reset is triggered
*/
void doIncrementalReset(){
  // Try restarting shield first (this preserves Arduino Alarm state)
  if(!doneSoftReset){
    cell.Reset();   
    bootGSMShield();
    doneSoftReset = true;
  }else{
    // Reset Arduino
    HardwareReset();
  }
}

/**
* Check for I2C errors and notify as appropriate
*/
void checkI2CProblems(){ 
  // Check wire status.
  Serial.print(F("Wire Status: "));
  Serial.println(wireResponseCode);
  if(wireResponseCode != 0){
    // Wire is malfunctioning
    playShortBeepSound();
    
    // Alert everyone
    if(!wireFailureResponse && (((unsigned long)(millis() - lastI2CFailNotification) > I2C_FAIL_NOTIFICATION_PERIOD) || lastI2CFailNotification == 0)){
      char message[85] = "Master -> Slave I2C has failed. Reply with 'IKNOW' to stop these updates. Status: ";
      message[83] = (wireResponseCode + 48); //Convert decimal code to ASCII equivalent
      message[84] = '\0';
      notifyContactsSMS(1, message);
      lastI2CFailNotification = millis();
    }
  }else{
    // I2C is working again, notify contacts
    if(lastI2CFailNotification != 0){
      notifyContactsSMS(1,(char *) "Master -> Slave I2C has sucessfully restarted.\0");
      lastI2CFailNotification = 0;
    }
  }
}
