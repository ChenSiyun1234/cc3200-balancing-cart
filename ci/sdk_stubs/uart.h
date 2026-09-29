//*****************************************************************************
// uart.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __UART_H__
#define __UART_H__

#include "hw_types.h"

extern tBoolean UARTCharsAvail(unsigned long ulBase);
extern long UARTCharGetNonBlocking(unsigned long ulBase);

#endif
