//*****************************************************************************
// hw_memmap.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __HW_MEMMAP_H__
#define __HW_MEMMAP_H__

#define GPIOA0_BASE             0x40004000
#define GPIOA1_BASE             0x40005000
#define GPIOA2_BASE             0x40006000
#define GPIOA3_BASE             0x40007000
#define UARTA0_BASE             0x4000C000
#define TIMERA0_BASE            0x40030000
#define TIMERA1_BASE            0x40031000
#define TIMERA2_BASE            0x40032000
#define GSPI_BASE               0x44021000

#endif
