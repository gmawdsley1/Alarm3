/*
  MegaMaster v 2.4
  July 2014
 
 Part of the BIRL Arduino Alarm project. This code is written for the Arduino Mega Rev 3.
 See the PDF for full description and design.
 
 The circuit:
 * Described in schematic.fz
 
 Created 15 Jan 2014
 v1.0 by Mayan Murray
 v2.0 by Aaron Lobo  alobo@sri.utoronto.ca  lobo.aaron1@gmail.com
 v2.4 by Mayan Murray - adding the feature of resetting alarm if 
 
 Resources Used:
 http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/
 
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
#include "Sounds.h"
#include "WatchdogFunctions.h"
#include <WSWire.h> //A custom Wire library which has timeouts: https://github.com/steamfire/WSWireLib

// Begin Cellular Variables
SerialGSM cell(10,11);
int cellStatus = 0;
boolean gotSMS = false;
char lastSMS[160] = {0};
char smsSender[13] = {0};
int numTimeouts = 0;
// End Cellular Variables


// Begin I2C Variables
byte wireResponseCode = 0;
unsigned long lastI2CFailNotification = 0;
#define I2C_FAIL_NOTIFICATION_PERIOD 21600000
boolean wireFailureResponse = false;
// End I2C Variables

Contact *contacts[CONTACTS_MAX_NUMBER];
Input *inputs[NUMINPUTS];
 
byte previousState[NUMINPUTS];
byte currentState[NUMINPUTS];
long lastTime;

long lastContactsCheck = 0;
//End Monitoring Variables

// Alarm disable variables
unsigned long alarmDisabledTime = 0;
byte disabledHours = 0;
// End alarm disable variables



byte pressed[NUMINPUTS], justPressed[NUMINPUTS], justReleased[NUMINPUTS];

byte alarmStatus = 1; // 0 = Disabled, 1 = Enabled
// End Common Code

unsigned long TimeResponded = 0;
unsigned long AlCallRestartTime = 120000;




void setup()
{
  SetupWatchdog();

  Wire.begin();
  Serial.begin(9600); 

  // Manually setup inputs
  inputs[0] = new Input();
  strlcpy(inputs[0]->name, "Shandon TP", sizeof(inputs[0]->name));
  inputs[0]->pin = 49;
  inputs[0]->notificationInterval = 420000;  //7 Minutes
  inputs[0]->lastNotificationTime = 0;
  inputs[0]->requiresResponse = true;
  inputs[0]->whoResponded = -1;

  inputs[1] = new Input();
  strlcpy(inputs[1]->name, "Pathos Delta", sizeof(inputs[1]->name));
  inputs[1]->pin = 51;
  inputs[1]->notificationInterval = 420000;  //7 Minutes
  inputs[1]->lastNotificationTime = 0;
  inputs[1]->requiresResponse = true;
  inputs[1]->whoResponded = -1;

  inputs[2] = new Input();
  strlcpy(inputs[2]->name, "Lab Power", sizeof(inputs[2]->name));
  inputs[2]->pin = 53;
  inputs[2]->notificationInterval = 21600000;  //6 Hours
  inputs[2]->lastNotificationTime = 0;
  inputs[2]->requiresResponse = false;
  inputs[2]->whoResponded = -1;
  // End manual input setup

  // Time before re-phoning all if alarm has not been dealt with
  
  

  // Enable inputs (with pull-up resistors on switch pins)
  for (byte i = 0; i < NUMINPUTS; i++) {
    pinMode(inputs[i]->pin, INPUT_PULLUP);
  }
  Serial.print(F("Alarm Initialized with "));
  Serial.print(NUMINPUTS, DEC);
  Serial.println(F(" inputs"));

  // Allocate memory for the contacts
  for(byte i = 0; i < 12; i++){
    contacts[i] = new Contact();
  }

  // Setup and test the speaker
  pinMode(PIN_SPEAKER, OUTPUT);
  playLongBeepSound();
  playSuccessSound();



  // Wait for the Ethernet shield to boot
  Serial.print(F("Waiting for Ethernet Arduino."));
  while(Wire.requestFrom(2, 1) <= 0){
    Serial.print('.');
    playShortBeepSound();
    delay(300); 
  }
  Serial.println(F("..Done"));
  delay(4000);
  
  // Get data from slave
  loadAndValidateContacts();  
  slaveGetSavedAlarmState();

  // Clear the wire
  while(Wire.available()){
    Wire.read();
  }

  // Ethernet is ready
  playLongBeepSound();



  // Boot GSM Module
  Serial.print(F("Booting GSM Shield."));
  initializeGSMShield();
  bootGSMShield();

  // Setup handler for SMS messages
  //&&&&&&&&&&&&&&&&&&&&&&&&&& Error &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
/* In file included from MegaMaster.ino:22:
C:\Users\Mayan\Documents\Arduino\libraries\SerialGSM/SerialGSM.h:10: error: expected class-name before '{' token
MegaMaster.ino: In function 'void setup()':
MegaMaster:223: error: 'class SerialGSM' has no member named 'registerSMSCallback'
MegaMaster.ino: In function 'void loop()':
MegaMaster:383: error: 'TimeResponded' was not declared in this scope
MegaMaster:383: error: 'AlCallRestartTime' was not declared in this scope
ContactManagementFunctions.ino: In function 'void notifyContactsAlarmState(byte)':
ContactManagementFunctions:114: error: could not convert 'cell.SerialGSM::Call(((char*)(& contacts[((int)i)]->Contact::phone)))' to 'bool'
ContactManagementFunctions:114: error: in argument to unary !
ContactManagementFunctions:121: error: could not convert 'cell.SerialGSM::Hangup()' to 'bool'
ContactManagementFunctions:121: error: in argument to unary !
ContactManagementFunctions:132: error: could not convert 'cell.SerialGSM::Hangup()' to 'bool'
ContactManagementFunctions:132: error: in argument to unary !
ContactManagementFunctions.ino: In function 'void notifyContactsAlarmStillNotAddressed(byte)':
ContactManagementFunctions:168: error: invalid conversion from 'char' to 'const char*'
c:/program files (x86)/arduino/hardware/tools/avr/lib/gcc/../../avr/include/string.h:134: error: too many arguments to function 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions:168: error: at this point in file
ContactManagementFunctions:170: error: invalid conversion from 'int' to 'const char*'
ContactManagementFunctions:170: error: initializing argument 2 of 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions:174: error: invalid conversion from 'char' to 'const char*'
c:/program files (x86)/arduino/hardware/tools/avr/lib/gcc/../../avr/include/string.h:134: error: too many arguments to function 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions:174: error: at this point in file
ContactManagementFunctions:176: error: invalid conversion from 'int' to 'const char*'
ContactManagementFunctions:176: error: initializing argument 2 of 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions.ino: In function 'void notifyContactsSMS(byte, char*)':
ContactManagementFunctions:246: error: could not convert 'cell.SerialGSM::DeleteAllSMS()' to 'bool'
ContactManagementFunctions:246: error: in argument to unary !
DiagnosticFunctions.ino: In function 'void checkGSMProblems()':
DiagnosticFunctions:15: error: 'class SerialGSM' has no member named 'GetErrorCode'
DiagnosticFunctions:17: error: 'class SerialGSM' has no member named 'GetErrorCode'
GSMFunctions.ino: In function 'void initializeGSMShield()':
GSMFunctions:11: error: 'class SerialGSM' has no member named 'begin'
GSMFunctions.ino: In function 'void trySendSMS(char*, char*)':
GSMFunctions:52: error: could not convert 'cell.SerialGSM::SendSMS(cellnumber, outmsg)' to 'bool'
GSMFunctions:52: error: in argument to unary !
GSMFunctions:56: error: could not convert 'cell.SerialGSM::SendSMS(cellnumber, outmsg)' to 'bool'
GSMFunctions:56: error: in argument to unary !
GSMFunctions.ino: In function 'void checkIncomingSMS()':
GSMFunctions:152: error: could not convert 'cell.SerialGSM::DeleteAllSMS()' to 'bool'
GSMFunctions:152: error: in argument to unary !
*/
// &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
  cell.registerSMSCallback(onReceiveSMS);

  Serial.println(F("..Done"));

  // GSM Module is ready
  playLongBeepSound();



  // Let everything stabilize
  delay(1000); 

  playSuccessSound();
}


void loop() {

  Serial.print(F("Cell Status: "));
  Serial.println(cell.GetGSMStatus());

  // Feed the watchdog
  ResetWatchdog();

  // Fill the cell buffer
  cell.ReadLine();

  // Check for a response
  checkIncomingSMS();

  // Diagnostics
  checkGSMProblems();
  checkI2CProblems();

  // Fire every 15 seconds or if contacts have never been loaded
  if (((unsigned long)(millis() - lastContactsCheck) > 15000) || numContacts == 0){

    // Slave communication if not in alarm state
    if(!inAlarmState()){

      //Load contacts if there is an update, or they have never been loaded
      if(slaveGetContactsFileChanged() == 1 || numContacts == 0){
        loadAndValidateContacts();
        Serial.println(F("Getting Contacts"));
      }
      
      Serial.println(F("Checked for Contact updates"));

      // Get Disabled hours
      byte hours = slaveGetAlarmDisabledHours();

      // Check for a change
      if(hours != disabledHours){ 

        disabledHours = hours;

        if(disabledHours == 0){
          // Enable Alarm
          alarmStatus = 1;
          alarmDisabledTime = 0;
          disabledHours = 0;
        }
        else if (0 < disabledHours){
          alarmStatus = 0;      
          alarmDisabledTime = millis();   

          Serial.print(F("Alarm disabled for "));
          Serial.print(disabledHours); 
          Serial.println(F(" hours"));

          notifyContactsSMS(1, (char*) "Alarm has been disabled.");
        }
      }
      
      Serial.println(F("Checked for Alarm Disable"));      
    }

    //Always test cell connectivity and ensure messages are forwarded to the serial output
    cell.FwdSMS2Serial();

    Serial.println(F("Completed 15 sec event."));
    lastContactsCheck = millis();
  }


  // Only update the inputs if the alarm is enabled
  if(alarmStatus == 1){
    // Read the state of the switches into the state arrays
    Serial.println(F("Checking inputs.."));
    checkInputs();
  }
  else{
    // Check how much time is left        
    if ((millis() - alarmDisabledTime)/3600000 > disabledHours){
      alarmStatus = 1; 
      Serial.println(F("Alarm Enabled"));
      notifyContactsSMS(1, (char *)"Alarm has been automatically enabled.");
    }

    Serial.print(F("Alarm turning on in "));
    Serial.print(disabledHours - ((unsigned long)(millis() - alarmDisabledTime) / 3600000));
    Serial.println(F(" hours"));
  }

  // Check switch states
  for (byte i = 0; i < NUMINPUTS; i++) {

    // Handle a new alarm
    if (justPressed[i]) {
      Serial.print(i, DEC);
      Serial.println(" Just pressed"); 

      // Set an alarm for the current machine (i)
      slaveSetAlarm(i);

      playLongBeepSound();

      notifyContactsAlarmState(i); 
      inputs[i]->lastNotificationTime = millis();

      // Clear the flag
      justPressed[i] = 0;
    }

    // Handle a cleared alarm
    if (justReleased[i]) {
      Serial.print(i, DEC);
      Serial.println(F(" Just released"));

      // Clear the alarm for the current machine (i)
      slaveClearAlarm(i);

      notifyContactsAlarmState(i);
      inputs[i]->lastNotificationTime = millis();
      inputs[i]->whoResponded = -1;

      // Clear the flag
      justReleased[i] = 0;
    }

    // Handle an ongoing alarm
    if (pressed[i]) { 
      Serial.print(i, DEC);
      Serial.println(F(" pressed"));

      playAlarmSound();

      // Only notify contacts if no-one has responded
      if(inputs[i]->whoResponded == -1){
        // Check if it is time to notify the contacts again
        if((unsigned long)(millis() - inputs[i]->lastNotificationTime) >= inputs[i]->notificationInterval){
          Serial.println(F("Reminding contacts"));
          notifyContactsAlarmState(i);
          inputs[i]->lastNotificationTime = millis(); 
        }
      }
	//********************************** July 2014 update! ************************************************
	// If someone has responded but it has been over the allotted time since the alarm has
	// started to go off without being resolved. Send a text out to all that whoResponded has
	// been unable to take care of the issue
// &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& Error &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
/*
In file included from MegaMaster.ino:22:
C:\Users\Mayan\Documents\Arduino\libraries\SerialGSM/SerialGSM.h:10: error: expected class-name before '{' token
MegaMaster.ino: In function 'void loop()':
MegaMaster:382: error: 'TimeResponded' was not declared in this scope
MegaMaster:382: error: 'AlCallRestartTime' was not declared in this scope
ContactManagementFunctions.ino: In function 'void notifyContactsAlarmState(byte)':
ContactManagementFunctions:114: error: could not convert 'cell.SerialGSM::Call(((char*)(& contacts[((int)i)]->Contact::phone)))' to 'bool'
ContactManagementFunctions:114: error: in argument to unary !
ContactManagementFunctions:121: error: could not convert 'cell.SerialGSM::Hangup()' to 'bool'
ContactManagementFunctions:121: error: in argument to unary !
ContactManagementFunctions:132: error: could not convert 'cell.SerialGSM::Hangup()' to 'bool'
ContactManagementFunctions:132: error: in argument to unary !
ContactManagementFunctions.ino: In function 'void notifyContactsAlarmStillNotAddressed(byte)':
ContactManagementFunctions:168: error: invalid conversion from 'char' to 'const char*'
c:/program files (x86)/arduino/hardware/tools/avr/lib/gcc/../../avr/include/string.h:134: error: too many arguments to function 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions:168: error: at this point in file
ContactManagementFunctions:170: error: invalid conversion from 'int' to 'const char*'
ContactManagementFunctions:170: error: initializing argument 2 of 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions:174: error: invalid conversion from 'char' to 'const char*'
c:/program files (x86)/arduino/hardware/tools/avr/lib/gcc/../../avr/include/string.h:134: error: too many arguments to function 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions:174: error: at this point in file
ContactManagementFunctions:176: error: invalid conversion from 'int' to 'const char*'
ContactManagementFunctions:176: error: initializing argument 2 of 'char* strncat(char*, const char*, size_t)'
ContactManagementFunctions.ino: In function 'void notifyContactsSMS(byte, char*)':
ContactManagementFunctions:246: error: could not convert 'cell.SerialGSM::DeleteAllSMS()' to 'bool'
ContactManagementFunctions:246: error: in argument to unary !
DiagnosticFunctions.ino: In function 'void checkGSMProblems()':
DiagnosticFunctions:15: error: 'class SerialGSM' has no member named 'GetErrorCode'
DiagnosticFunctions:17: error: 'class SerialGSM' has no member named 'GetErrorCode'
GSMFunctions.ino: In function 'void initializeGSMShield()':
GSMFunctions:11: error: 'class SerialGSM' has no member named 'begin'
GSMFunctions.ino: In function 'void trySendSMS(char*, char*)':
GSMFunctions:52: error: could not convert 'cell.SerialGSM::SendSMS(cellnumber, outmsg)' to 'bool'
GSMFunctions:52: error: in argument to unary !
GSMFunctions:56: error: could not convert 'cell.SerialGSM::SendSMS(cellnumber, outmsg)' to 'bool'
GSMFunctions:56: error: in argument to unary !
GSMFunctions.ino: In function 'void checkIncomingSMS()':
GSMFunctions:152: error: could not convert 'cell.SerialGSM::DeleteAllSMS()' to 'bool'
GSMFunctions:152: error: in argument to unary !
	*/
// &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	
	  if(((unsigned long) (millis() - TimeResponded) >= AlCallRestartTime) && justPressed[i]==0){
		Serial.println("Although someone had taken responsibility for attending to the alarm, the alarm is still active 2 hours later. Group 1 will be called again until another person takes responsibility to address the alarm.");
    
		notifyContactsAlarmResponse(i); 
		
		// reset the alarm to start from scratch
		justPressed[i] = 1;
		//clearing the name of the person who had responded last
		inputs[i]->whoResponded = -1;
		
		
	  }
    }
	
	

	
	
	
	
	
	

  }

  Serial.println(freeRam());
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

