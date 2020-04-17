#ifndef MM_H
#define MM_H

extern void loadAndValidateContacts(void);
extern byte slaveGetSavedAlarmState(void);
extern void checkInputs(void);
extern int freeRam (void);

#include "SerialGSM.h"

// Set the backup contact. They will be alerted if the alarm fails, in addition to group one contacts.
#define BACKUPCONTACT "16479806182" // Mayan


// Begin Common Code
#define NUMINPUTS 3

// Wire Communication Codes
#define COMM_TYPE_REQUEST 30
#define COMM_TYPE_ALARMRESPONSE 40
#define COMM_TYPE_SETALARM 50
#define COMM_TYPE_CLEARALARM 51

// Request ids
#define REQUEST_ID_CONTACTS_CHANGED 31
#define REQUEST_ID_CONTACTS 32
#define REQUEST_ID_CONTACTS_CHECKSUM 33
#define REQUEST_ID_STATUS 34
#define REQUEST_ID_ALARMDISABLEHOURS 35
#define REQUEST_ID_ALARMSTATE 36

#define I2C_STATUS_IDLE 255


//Begin Watchdog Variables
#define WATCHDOG_TIMEOUT_SECONDS 90
#define PIN_RESET_ARDUINO 4
//End Watchdog Variables


// Begin Other Definitions
#define PIN_SPEAKER 12
// End Other Definitions


extern byte alarmStatus;

// Begin Contacts variables
#define CONTACTS_MAX_NUMBER 12
extern SerialGSM cell;
extern int cellStatus;
extern boolean gotSMS;
extern char lastSMS[160];
extern char smsSender[13];
extern int numTimeouts;
// End Cellular Variables

class Contact
{
public:
  byte group;
  char name[9];    //Max  9-1= 8 chars
  char email[39];  //Max 39-1= 38 chars
  char phone[12];  //Max 12-1= 11 chars
};


extern Contact *contacts[CONTACTS_MAX_NUMBER];
// End Contacts variables
// Class to represent a machine input
class Input
{
public:
  char name[13];   //Max 13-1= 12 chars
  byte pin;
  unsigned long notificationInterval;
  unsigned long lastNotificationTime; 
  boolean requiresResponse;
  char whoResponded;  //The contact who has taken responsibility for this alarm
};
extern Input *inputs[NUMINPUTS];
//End Monitoring Variables


// Begin I2C Variables
extern byte wireResponseCode;
extern unsigned long lastI2CFailNotification;
#define I2C_FAIL_NOTIFICATION_PERIOD 21600000

extern boolean wireFailureResponse;

extern byte numContacts;

extern byte previousState[NUMINPUTS];
extern byte currentState[NUMINPUTS];
extern long lastTime;

extern long lastContactsCheck;

extern unsigned long alarmDisabledTime;
extern byte disabledHours;


extern byte pressed[NUMINPUTS], justPressed[NUMINPUTS], justReleased[NUMINPUTS];

// Begin Monitoring Variables
#define DEBOUNCE 50


#endif