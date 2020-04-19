/*
  Slave Communication Functions
  
  Provides functions for slave I2C communication
*/
#include <Arduino.h>
#include "SlaveCommunicationsFunctions.h"
#include "GSMFunctions.h"
#include "GSMSoftwareSerial.h"
#include "SerialGSM.h"
#include "DiagnosticFunctions.h"
#include "MegaMaster.h"
#include "CRC32.h"
#include "ContactManagementFunctions.h"
#include "MonitoringFunctions.h"
#include "Wire.h" //A custom Wire library which has timeouts: https://github.com/steamfire/WSWireLib

/**
* Let the slave know an input has gone into alarm
* switchNum: The machine id
*/
void slaveSetAlarm(byte switchNum){
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_SETALARM);
  Wire.write(switchNum);
  Wire.endTransmission();
}

/**
* Let the slave know an input alarm has cleared
* switchNum: The machine id
*/
void slaveClearAlarm(byte switchNum){
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_CLEARALARM);
  Wire.write(switchNum);
  Wire.endTransmission();
}

/**
* Let the slave know an a contact has responded to an alarm
* alarmId: The machine id
* userId: The contact id
*/
void slaveSetAlarmResponse(byte alarmId, char userId)
{
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_ALARMRESPONSE);
  Wire.write(alarmId);
  Wire.write(userId);
  wireResponseCode = Wire.endTransmission();  
}

/**
* Check if the alarm contacts have been updated on the slave
* 
* Returns:
*   0 if there are no changes in the contacts file
*   1 if the contacts file has changed
*/
byte slaveGetContactsFileChanged()
{
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_REQUEST);
  Wire.write(REQUEST_ID_CONTACTS_CHANGED);
  wireResponseCode = Wire.endTransmission();

  // Give the slave time to process
  delay(10);

  if (Wire.requestFrom(2, 1) == 1){
    return Wire.read();
  }
  return 0;
}

/**
* Check if the alarm has been disabled on the slave
* 
* Returns:
*   -byte (0-255) representing the hours the alarm should be disabled for.
*/
byte slaveGetAlarmDisabledHours(){
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_REQUEST);
  Wire.write(REQUEST_ID_ALARMDISABLEHOURS);
  Wire.write(alarmStatus);  //Send the slave the current status so it can update the webpage
  wireResponseCode = Wire.endTransmission();

  // Give the slave time to process
  delay(10);

  if (Wire.requestFrom(2, 1) == 1){
    return Wire.read();
  }
  return 0;
}

/**
* Gets the saved alarm state from the slave. This function only handles 32 bytes, limiting the total number of machines to 32
* 
* Returns:
*   -byte (0 or 1) representing the saved state of the alarm.
*/
byte slaveGetSavedAlarmState(){
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_REQUEST);
  Wire.write(REQUEST_ID_ALARMSTATE);
  wireResponseCode = Wire.endTransmission();

  // Give the slave time to process
  delay(500);

  byte currentMachine = 0;

  if (Wire.requestFrom(2, 32) >= 1){
    while(Wire.available()){
      byte value = Wire.read();
      
      Serial.print("Got savestate: ");
      Serial.println(value); 
            
      if(currentMachine < NUMINPUTS && value <= 1){
        pressed[currentMachine] = value;
      }
      
      lastTime = millis();      
      currentMachine++;
    }
    return 1;
  }
  return 0;
}

/**
* Get the CRC32 hash of the contacts file from the slave
* 
* Returns:
*   -unisgned long containing the hash
*/
unsigned long int slaveGetContactsCheckSum(){
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_REQUEST);
  Wire.write(REQUEST_ID_CONTACTS_CHECKSUM);
  wireResponseCode = Wire.endTransmission();

  // Give the slave time to process
  delay(10);

  if (Wire.requestFrom(2, 4) == 4){
    //Use a union to assembly the bytes back into an unsigned long
    union bytes {
      unsigned char c[4];
      unsigned long int l;
    }
    myb;

    myb.c[0] = Wire.read();
    myb.c[1] = Wire.read();
    myb.c[2] = Wire.read();
    myb.c[3] = Wire.read();

    return myb.l;
  }
  return 0;
}

/**
* Get the contacts from the slave. All contacts will be stored in the contacts array (contacts).
* Data is buffered as it comes in over the I2C connection and is reassembled and parsed to extract data.
* A CRC32 hash of the recieved data is generated.
* 
* fileSize: The filesize to request from the slave
* 
* Returns:
*   -unisgned long containing the data hash
*/
unsigned long int slaveGetContacts(unsigned long fileSize){
  
  unsigned long crc = ~0L;
  
  // Request the contacts from the slave
  Wire.beginTransmission(2);
  Wire.write(COMM_TYPE_REQUEST);
  Wire.write(REQUEST_ID_CONTACTS);
  wireResponseCode = Wire.endTransmission();

  // Let the slave open the SD card and prep the file for reading
  delay(500);

  // Each line in the contacts.csv has a max length of 64 bytes.
  // The max I2C transfer is 32 bytes, so two transmissions are concatenated in the buffer
  char lineBuffer[64];
  byte lIndex = 0;
  numContacts = 0;

  // Counter for the total bytes recieved
  unsigned int totalBytes = 0;
  
  // Execute once to allow totalBytes to be greater than 0. If totalBytes is still 0
  // after execution, transfer has failed and the loop breaks
  do{

    Serial.print("Transferred: ");
    Serial.print(totalBytes);
    Serial.print("/");
    Serial.println(fileSize);
    
    Wire.requestFrom(2, 32);

    // Give the slave time to process
    delay(75);

    while(Wire.available()){
      char c = Wire.read();    // Receive a byte as character
      if(c > 0){
        lineBuffer[lIndex] = c;
        lIndex++;
        Serial.print(c);
        
        // Update the hash
        crc = crc_update(crc, c);        
      }
      
      totalBytes++;
      
      // Check for a full contact (64 bytes)
      if(c == '\n' || lIndex >= 64){
        // Parse the contact details and store in the contacts array

        char* temp;
        byte strSize = 0;

        // Group - Derefrence the pointer and convert char to decimal
        contacts[numContacts]->group = (*strtok(lineBuffer, ",") - 48);

        // Name
        strSize = sizeof(contacts[numContacts]->name);
        temp = strtok(NULL, ",");     
        strncpy(contacts[numContacts]->name, temp, strSize - 1);
        contacts[numContacts]->name[strSize - 1] = '\0'; //Null terminate the string

        // Email
        strSize = sizeof(contacts[numContacts]->email);
        temp = strtok(NULL, ",");     
        strncpy(contacts[numContacts]->email, temp, strSize - 1);
        contacts[numContacts]->email[strSize - 1] = '\0'; //Null terminate the string

        // Phone                
        strSize = sizeof(contacts[numContacts]->phone);
        temp = strtok(NULL, "\n");   
        strncpy(contacts[numContacts]->phone, temp, strSize - 1);
        contacts[numContacts]->phone[strSize - 1] = '\0'; //Null terminate the string        

        numContacts++;        
        lIndex = 0;        
      }
    }
  } 
  
  while (totalBytes > 0 && totalBytes < fileSize);  // Loop as long data is coming in and there's still some remaining to transfer
  
  Serial.println();
  Serial.print(F("Total bytes transferred:"));
  Serial.println(totalBytes);
  
  // Finalize the hash
  crc = ~crc;
  return crc;
}


