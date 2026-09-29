//*****************************************************************************
// rom_map.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __ROM_MAP_H__
#define __ROM_MAP_H__

#define MAP_IntMasterEnable             IntMasterEnable
#define MAP_IntVTableBaseSet            IntVTableBaseSet
#define MAP_IntEnable                   IntEnable
#define MAP_PRCMPeripheralClkEnable     PRCMPeripheralClkEnable
#define MAP_PRCMPeripheralReset         PRCMPeripheralReset
#define MAP_PRCMPeripheralClockGet      PRCMPeripheralClockGet
#define MAP_PinTypeGPIO                 PinTypeGPIO
#define MAP_PinTypeI2C                  PinTypeI2C
#define MAP_PinTypeUART                 PinTypeUART
#define MAP_PinTypeTimer                PinTypeTimer
#define MAP_PinTypeSPI                  PinTypeSPI
#define MAP_GPIODirModeSet              GPIODirModeSet
#define MAP_GPIOPinWrite                GPIOPinWrite
#define MAP_GPIOPinRead                 GPIOPinRead
#define MAP_GPIOIntClear                GPIOIntClear
#define MAP_TimerConfigure              TimerConfigure
#define MAP_TimerPrescaleSet            TimerPrescaleSet
#define MAP_TimerControlLevel           TimerControlLevel
#define MAP_TimerLoadSet                TimerLoadSet
#define MAP_TimerMatchSet               TimerMatchSet
#define MAP_TimerEnable                 TimerEnable
#define MAP_TimerValueGet               TimerValueGet
#define MAP_UtilsDelay                  UtilsDelay
#define MAP_SPIReset                    SPIReset
#define MAP_SPIConfigSetExpClk          SPIConfigSetExpClk
#define MAP_SPIEnable                   SPIEnable
#define MAP_UARTCharsAvail              UARTCharsAvail
#define MAP_UARTCharGetNonBlocking      UARTCharGetNonBlocking

#endif
