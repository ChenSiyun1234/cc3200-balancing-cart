//*****************************************************************************
// interrupt.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "hw_types.h"

extern tBoolean IntMasterEnable(void);
extern void IntVTableBaseSet(unsigned long ulVtableBase);
extern void IntEnable(unsigned long ulInterrupt);

#endif
