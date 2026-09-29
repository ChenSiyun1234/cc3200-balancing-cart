//*****************************************************************************
// fake_hal.h - recording fakes for the safety.c hooks (host tests only)
//
// Every hook call is appended to a string, one letter per call:
//   C = motors coast          0 / 1 = H-bridge disable / enable
//   P = controller reset      B = I2C bus re-open
//   A = alert sent            L = console line
// The fake clock counts down at 80 ticks/us and wraps at 32 bits, like the
// CC3200 timer behind clock.c.
//*****************************************************************************
#ifndef FAKE_HAL_H
#define FAKE_HAL_H

#include "safety.h"

extern const tSafetyHal g_sFakeHal;       // every hook
extern const tSafetyHal g_sFakeHalQuiet;  // no alert and no console hook

void        Fake_Reset(void);             // clear calls, counters and clock
void        Fake_ClearCalls(void);        // clear the call string and log text
const char *Fake_Calls(void);
int         Fake_Count(char cCall);       // how often one call letter appears
int         Fake_Overflow(void);          // 1 if the call or log buffer filled up

void        Fake_AdvanceUs(unsigned long ulUs);
void        Fake_SetTicks(unsigned long ulTicks);

void        Fake_SetAlertResult(int iResult);   // what the alert hook returns
int         Fake_AlertCount(void);
const char *Fake_LastAlert(void);

const char *Fake_LastLog(void);
const char *Fake_LogText(void);           // all console lines since the last clear

#endif // FAKE_HAL_H
