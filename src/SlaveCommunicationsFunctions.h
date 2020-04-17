#ifndef SCF_H
#define SCF_H

extern void slaveSetAlarm(byte);
extern void slaveClearAlarm(byte);
extern void slaveSetAlarmResponse(byte, char);
extern byte slaveGetContactsFileChanged(void);
extern byte slaveGetAlarmDisabledHours(void);
extern byte slaveGetSavedAlarmState(void);
unsigned long int slaveGetContactsCheckSum(void);
unsigned long int slaveGetContacts(unsigned long);
#endif