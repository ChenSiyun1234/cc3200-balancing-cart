//*****************************************************************************
// pin.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __PIN_H__
#define __PIN_H__

#include "hw_types.h"

#define PIN_01                  0x00000000
#define PIN_02                  0x00000001
#define PIN_04                  0x00000003
#define PIN_05                  0x00000004
#define PIN_06                  0x00000005
#define PIN_07                  0x00000006
#define PIN_08                  0x00000007
#define PIN_15                  0x0000000E
#define PIN_17                  0x00000010
#define PIN_18                  0x00000011
#define PIN_45                  0x0000002C
#define PIN_50                  0x00000031
#define PIN_53                  0x00000034
#define PIN_55                  0x00000036
#define PIN_57                  0x00000038
#define PIN_61                  0x0000003C
#define PIN_62                  0x0000003D
#define PIN_63                  0x0000003E
#define PIN_64                  0x0000003F

#define PIN_MODE_0              0x00000000
#define PIN_MODE_1              0x00000001
#define PIN_MODE_3              0x00000003
#define PIN_MODE_5              0x00000005
#define PIN_MODE_7              0x00000007

extern void PinTypeGPIO(unsigned long ulPin, unsigned long ulPinMode,
                        tBoolean bOpenDrain);
extern void PinTypeI2C(unsigned long ulPin, unsigned long ulPinMode);
extern void PinTypeUART(unsigned long ulPin, unsigned long ulPinMode);
extern void PinTypeTimer(unsigned long ulPin, unsigned long ulPinMode);
extern void PinTypeSPI(unsigned long ulPin, unsigned long ulPinMode);

#endif
