//*****************************************************************************
// common.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __COMMON__H__
#define __COMMON__H__

// The SDK's common.h also holds the Wi-Fi settings (SSID_NAME, SECURITY_KEY)
// for the demos. They stay in your SDK copy and are not needed here.

#define SSID_LEN_MAX            32
#define BSSID_LEN_MAX           6

#define UART_PRINT              Report

#endif
