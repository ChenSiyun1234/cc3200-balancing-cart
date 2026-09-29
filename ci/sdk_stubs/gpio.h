//*****************************************************************************
// gpio.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __GPIO_H__
#define __GPIO_H__

#define GPIO_DIR_MODE_IN        0x00000000
#define GPIO_DIR_MODE_OUT       0x00000001
#define GPIO_RISING_EDGE        0x00000004

extern void GPIODirModeSet(unsigned long ulPort, unsigned char ucPins,
                           unsigned long ulPinIO);
extern void GPIOPinWrite(unsigned long ulPort, unsigned char ucPins,
                         unsigned char ucVal);
extern long GPIOPinRead(unsigned long ulPort, unsigned char ucPins);
extern void GPIOIntClear(unsigned long ulPort, unsigned long ulIntFlags);

#endif
