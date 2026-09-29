//*****************************************************************************
// timer.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __TIMER_H__
#define __TIMER_H__

#include "hw_types.h"

#define TIMER_A                 0x000000FF
#define TIMER_B                 0x0000FF00

#define TIMER_CFG_PERIODIC      0x00000022
#define TIMER_CFG_SPLIT_PAIR    0x04000000
#define TIMER_CFG_A_PWM         0x0000000A
#define TIMER_CFG_B_PWM         0x00000A00

extern void TimerConfigure(unsigned long ulBase, unsigned long ulConfig);
extern void TimerPrescaleSet(unsigned long ulBase, unsigned long ulTimer,
                             unsigned long ulValue);
extern void TimerControlLevel(unsigned long ulBase, unsigned long ulTimer,
                              tBoolean bInvert);
extern void TimerLoadSet(unsigned long ulBase, unsigned long ulTimer,
                         unsigned long ulValue);
extern void TimerMatchSet(unsigned long ulBase, unsigned long ulTimer,
                          unsigned long ulValue);
extern void TimerEnable(unsigned long ulBase, unsigned long ulTimer);
extern unsigned long TimerValueGet(unsigned long ulBase, unsigned long ulTimer);

#endif
