//*****************************************************************************
// rom.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __ROM_H__
#define __ROM_H__

// No ROM_ entry points here: rom_map.h maps every MAP_ call to the plain
// driverlib function.

#endif
