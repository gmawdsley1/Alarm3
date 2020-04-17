#ifndef CMF
#define CMF
extern void notifyContactsSMS(byte,char*);
extern int isInContactList(char*);
extern void notifyContactsAlarmState(byte);
extern int notifyContactsAlarmResponse(int);
#endif