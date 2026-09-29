//*****************************************************************************
// prcm.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __PRCM_H__
#define __PRCM_H__

#define PRCM_RUN_MODE_CLK       0x00000001

#define PRCM_GSPI               0x00000003
#define PRCM_GPIOA0             0x00000006
#define PRCM_GPIOA1             0x00000007
#define PRCM_GPIOA2             0x00000008
#define PRCM_GPIOA3             0x00000009
#define PRCM_UARTA0             0x0000000C
#define PRCM_TIMERA0            0x0000000E
#define PRCM_TIMERA1            0x0000000F
#define PRCM_TIMERA2            0x00000010
#define PRCM_I2CA0              0x00000014

extern void PRCMCC3200MCUInit(void);
extern void PRCMPeripheralClkEnable(unsigned long ulPeripheral,
                                    unsigned long ulClkFlags);
extern void PRCMPeripheralReset(unsigned long ulPeripheral);
extern unsigned long PRCMPeripheralClockGet(unsigned long ulPeripheral);

#endif
