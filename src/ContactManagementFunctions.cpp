/*
  Contact Management Functions
  
  Provides functions for loading and notifying contacts
*/

#include <Arduino.h>
#include "SlaveCommunicationsFunctions.h"
#include "GSMFunctions.h"
#include "SerialGSM.h"
#include "GSMSoftwareSerial.h"
#include "DiagnosticFunctions.h"
#include "MegaMaster.h"
#include "ContactManagementFunctions.h"
#include "MonitoringFunctions.h"
#include "sounds.h"
#include "Wire.h"
#include "WatchdogFunctions.h"

/**
* Requests contacts from the slave and verifies the transfer was successful.
* Contacts are stored in the contacts array
*/

static byte numContacts=0;

void loadAndValidateContacts(){
  // Calculate the maximum possible file size
  unsigned long fileSize = CONTACTS_MAX_NUMBER * 64;

  unsigned long checkSum = slaveGetContacts(fileSize);
  if(checkSum == slaveGetContactsCheckSum() && checkSum > 0){
    Serial.println(F("Contacts transferred successfully."));
    playSuccessSound();
  }
  else{
    Serial.println(F("Hash mismatch! Possible data corruption"));
    playAlarmSound();
    // Retry on the next loop
    numContacts = 0;
    numTimeouts++; // Count this as a timeout
  }

  // Print all the contacts
  for(byte i = 0; i < numContacts; i++){        
    Serial.print(F("Contact #"));
    Serial.println(i);

    Serial.print(F("Group: "));
    Serial.println(contacts[i]->group);

    Serial.print(F("Name: "));
    Serial.println(contacts[i]->name);

    Serial.print(F("Email: "));
    Serial.println(contacts[i]->email);

    Serial.print(F("Phone: "));
    Serial.println(contacts[i]->phone); 
  }
}

int isInContactList(char* number){  
  for(int j = 0; j < numContacts; j++){
    if(strstr(number, contacts[j]->phone)){
      return j;      
    }
  }  
  return -1;
}

/**
* Notify contacts that a machine state has changed
* switchNum: The machine id
*/
void notifyContactsAlarmState(byte switchNum){
  
  // Message Buffer
  const byte msgSize = 140;
  char message[msgSize] = {0};

  char Responded = inputs[switchNum]->whoResponded;

  
  // Begin building the SMS message
  // Add the machine name
  strncat(message,  "The ", msgSize - strlen(message) - 1);
  strncat(message, inputs[switchNum]->name, msgSize - strlen(message) - 1);

  // Add the appropriate status update
  if (justPressed[switchNum] == 1 || pressed[switchNum] == 1){
    
    if(justPressed[switchNum] == 1){
      // Machine has just entered alarm state
      strncat(message, " is in an alarm state.", msgSize - strlen(message) - 1);
    }
    else{
      // Machine is still in alarm state
      strncat(message, " is still in an alarm state.", msgSize - strlen(message) - 1);         
    }
    
    if(inputs[switchNum]->requiresResponse){
      // Ask the recipient to reply with BIRLOFF
      strncat(message, " Please reply with 'BIRLOFF' if you are responding.", msgSize - strlen(message) - 1);    
    }
  }
  else if (justReleased[switchNum] == 1){
    strncat(message, " alarm has been cleared. The messages will now cease.", msgSize - strlen(message) - 1);
  }

  // Null terminate message
  message[139] = '\0';

  // Notify groups 1 & 2
  notifyContactsSMS(1, message);
  notifyContactsSMS(2, message);
  activeDelay(1000);

  for(byte i = 0; i < numContacts; i++){
    // Feed the watchdog (Calls are 15 seconds each)
    ResetWatchdog();

    // Call Group 1
    if (contacts[i]->group == 1  && Responded == -1){
      if(pressed[switchNum] == 1 && inputs[switchNum]->requiresResponse){

        Serial.print(F("Calling: "));
        Serial.println(contacts[i]->phone);

        if(!cell.Call(contacts[i]->phone)) numTimeouts++;

        // Keep monitoring the cell output while the call is ringing, to ensure no messages are missed
        for(int j = 0; j < 150; j++){
          cell.ReadLine();

          if(gotSMS || cell.GetGSMStatus() == 9){
            if(!cell.Hangup()) numTimeouts++;
            cell.ReadLine();
            break;
          }

          // Ensure the cell has not encountered problems
          checkGSMProblems();

          delay(100);
        }

        if(!cell.Hangup()) numTimeouts++;        
      }
    }

    activeDelay(1000);

    // Someone has responded to the alarm
    if(gotSMS){
      Serial.println(F("Got SMS Message! Break #2"));
      break; //TODO: Should verify this is a legit number before stopping calling
    }
  }

  if(gotSMS){
    Serial.println(F("Got SMS Message! Processing"));
    checkIncomingSMS();
  }

  Serial.println(F("________"));
}
// --------------------------- July 2014 update ---------------------------------------------------------
/*
* Notify contacts that the person who responded failed to address the issue
*/
void notifyContactsAlarmStillNotAddressed(byte switchNum){
	const byte msgSize = 140;
	// for warning Group 1 to expect the alarm to reset and someone new will have to take 
	// responsibility for address the alarm
	char message1[msgSize] = {
		'\0'  };
	// For warning Group 2 that the person who had earlier responded has failed to address the alarm
	char message2[msgSize] = {
		'\0'  };
		
	//Message 1
	strncat(message1, contacts[switchNum]->name, (msgSize - strlen(message1) - 1));
  strncat(message1, "  has not addressed the " ,(msgSize - strlen(message1) - 1));
	strncat(message1, inputs[switchNum]->name,msgSize - strlen(message1) - 1);
	strncat(message1," alarm from 2 hours ago. The alarm has been reset, so please standby.", msgSize - strlen(message1) - 1);
	Serial.println(message1);
	
	//Message 2
	strncat(message2, contacts[switchNum]->name,(msgSize - strlen(message2) - 1));
  strncat(message2, " has not addressed the " ,(msgSize - strlen(message2) - 1));
	strncat(message2, inputs[switchNum]->name,(msgSize - strlen(message2) - 1));
	strncat(message2," alarm from 2 hours ago. The alarm has been reset.", (msgSize - strlen(message2) - 1));
	Serial.println(message2);
	
	// Notify groups 1 and 2
  notifyContactsSMS(1, message1);
  notifyContactsSMS(2, message2);
}

// ----------------------------------------- July 2014 update ----------------------------------------------------------


/**
* Notify contacts that someone has responded
*/
void notifyContactsAlarmResponse(byte switchNum)
{
  // Message Buffer
  const byte msgSize = 140;
  char message[msgSize] = {
    '\0'  };
  strncat(message,  contacts[switchNum]->name, (msgSize - strlen(message) - 1));
  strncat(message,  " is responding to the ", (msgSize - strlen(message) - 1));
  strncat(message, inputs[switchNum]->name, (msgSize - strlen(message) - 1));
  strncat(message,  " alarm.", (msgSize - strlen(message) - 1));
  Serial.println(message);

  // Null terminate message
  message[139] = '\0';

  // Notify groups 1 and 2
  notifyContactsSMS(1, message);
  notifyContactsSMS(2, message);
}

/**
* Send a SMS message to all contacts
* @param contactGroup The contact group to send message to (1 or 2)
* @param message The message to send. Should be null terminated
*/
void notifyContactsSMS(byte contactGroup, char* message){

  if (contactGroup == 3){
    Serial.println(F("Cannot send SMS to group 3!"));
    return; 
  }

  for(byte i = 0; i < numContacts; i++){
    // Feed the watchdog
    ResetWatchdog();

    Serial.print(F("Processing contact: "));
    Serial.println(contacts[i]->name);
    Serial.print(F("Group: "));
    Serial.println(contacts[i]->group);

    Serial.println();

    // Just send an SMS message
    if (contacts[i]->group == contactGroup){
      Serial.print(F("Sending SMS: "));
      Serial.println(contacts[i]->phone);
      trySendSMS(contacts[i]->phone, message);
      cell.ReadLine();
    }
    cell.ReadLine();

  }

  if(!cell.DeleteAllSMS()) numTimeouts++;
  cell.ReadLine();

  Serial.println("________");
}

