/*
  GSM Functions
  
  Provides functions for error detection and correction (resetting)
*/
#include <Arduino.h>
#include "SlaveCommunicationsFunctions.h"
#include "GSMFunctions.h"
#include "GSMSoftwareSerial.h"
#include "SerialGSM.h"
#include "DiagnosticFunctions.h"
#include "MegaMaster.h"
#include "ContactManagementFunctions.h"
#include "Sounds.h"
#include "MonitoringFunctions.h"

void (* resetFunc) (void) = 0;
//declare reset function @ address 0
int garbage =0;
/**
* One time configuration for the GSM shield
*/
void initializeGSMShield()
{
  cell.begin(9600);  
  //The cellular shield will output all messages to serial. Disable in production.
  garbage++;
  cell.Verbose(true);
}

/**
* Monitor the GSM shield output and ensure it boots successfully.
* In case of failure, a reset will be performed.
*/
void bootGSMShield(){
  // Startup the modem
  cell.Boot();
  // Set the modem to forward all messages to the serial pins
  cell.FwdSMS2Serial();
  
  // Boot GSM Module
  Serial.print(F("Booting GSM Shield."));

  // Wait for appropriate status
  while (cellStatus != 4){
    if(cellStatus == 7){
      //We have a problem
      playFailSound();
      playAlarmSound();
      resetFunc();
    }

    Serial.println(cellStatus);
    cellStatus = cell.GetGSMStatus();
    cell.ReadLine();

    playShortBeepSound();
    delay(300);
  }
}

/**
* Tries to send an SMS message. In case of failure, this is reattempted.
* In case of a second failure, a reset will occur.
*/
void trySendSMS(char * cellnumber, char * outmsg){
  if(!cell.SendSMS(cellnumber, outmsg)){
    // Retry once
    Serial.println(F("Attempt 1: SMS Failed to send. Retrying."));
    numTimeouts++;
    if(!cell.SendSMS(cellnumber, outmsg)){
      // We got a problem.
      Serial.println(F("Attempt 1: SMS Failed to send. Restarting GSM."));
      numTimeouts++;
      doIncrementalReset(); //Diagnostic Function
    }
  }
}

/**
* Returns true if interrupted
*/
boolean activeDelay(int milliseconds){
  
  // Active Delay for 3000ms after calling SMS
  for(int i = 0; i < (milliseconds/100); i++){       
    cell.ReadLine();
    Serial.println("Waiting.");
    if(gotSMS) return true;
    delay(100);
  }  
  
  return false;
}


/**
* Parse an incoming text message and verifiy it has come from a trusted source.
* If it is trusted, take action based on the message contents.
*/
void checkIncomingSMS(){

  // Check for incoming SMS messages
  if(gotSMS) {
    
    Serial.println(F("-------------------------------------------------------------------"));
    Serial.println(F("Incoming SMS from: "));
    Serial.println(cell.Sender());
    Serial.println(F("*******************************************************************"));
    Serial.println(F("Message: "));
    Serial.println(lastSMS);
    Serial.println(F("-------------------------------------------------------------------"));
    
    // Verify the number
    int contactId = isInContactList(cell.Sender());
    if(contactId != -1){      
      // This is a trusted number, process the message
      
      if(strstr(lastSMS, "BIRLOFF") != NULL){
        // Check for an alarm response
        for (byte i = 0; i < NUMINPUTS; i++) {  
    
          // Only process if no one has responded
          if(inputs[i]->requiresResponse && inputs[i]->whoResponded == -1 && (pressed[i] == 1 || justPressed[i] == 1) ){
     
              inputs[i]->whoResponded = contactId;

              // Notify the slave
              slaveSetAlarmResponse(i, inputs[i]->whoResponded);
              notifyContactsAlarmResponse(i);
              break;
          }
          else{
            // Ignore SMS
            Serial.println(F("Ignoring SMS: Alarm already handled"));   
          }
        }
      }
      
      // Check for an error handle response
      if(wireResponseCode != 0){
        if(strstr(lastSMS, "IKNOW") != NULL){   
          
          wireFailureResponse = true;
          
          const byte msgSize = 140;
          char message[msgSize] = {'\0'};

          strncat(message,  contacts[contactId]->name, msgSize - strlen(message) - 1);
          strncat(message,  " is responding to the I2C error", msgSize - strlen(message) - 1);
          Serial.println(message);
        
          // Null terminate message
          message[139] = '\0';

          // Notify groups 1 and 2
          notifyContactsSMS(1, message);
        }
      }

    }

    //Reset flags
    gotSMS = false;
    
    // If the method returns false, it has timed out. Increment numTimeouts
    if (!cell.DeleteAllSMS()) numTimeouts++;
    cell.ReadLine();
  }
}

/**
 * SMS Recieve Callback function. This is triggered when 
 * an SMS is received. This function should be executed quickly,
 * so the message parameters are saved and processed in the main loop.
 */
int onReceiveSMS(void){
  Serial.print(F("Master Got SMS :"));
  gotSMS = true;
  strncpy (lastSMS, cell.Message(), sizeof(lastSMS));
  strncpy (smsSender, cell.Sender(), sizeof(smsSender));  
  return 0;
}

