#ifndef GSM_H
#define GSM_H
extern void initializeGSMShield(void);
extern void bootGSMShield(void);
extern void trySendSMS(char *, char *);
extern boolean activeDelay(int);
extern void checkIncomingSMS(void);
extern int onReceiveSMS(void);
#endif