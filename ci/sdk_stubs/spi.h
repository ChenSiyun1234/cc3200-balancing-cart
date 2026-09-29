//*****************************************************************************
// spi.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __SPI_H__
#define __SPI_H__

#define SPI_MODE_MASTER         0x00000000
#define SPI_SUB_MODE_0          0x00000000
#define SPI_SW_CTRL_CS          0x01000000
#define SPI_4PIN_MODE           0x00000000
#define SPI_TURBO_OFF           0x00000000
#define SPI_CS_ACTIVELOW        0x00000040
#define SPI_WL_8                0x00000380

extern void SPIReset(unsigned long ulBase);
extern void SPIConfigSetExpClk(unsigned long ulBase, unsigned long ulSPIClk,
                               unsigned long ulBitRate, unsigned long ulMode,
                               unsigned long ulSubMode, unsigned long ulConfig);
extern void SPIEnable(unsigned long ulBase);

#endif
