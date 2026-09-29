//*****************************************************************************
// simplelink.h - stand-in for the CC3200 SDK header of the same name.
//
// Declarations only, so CI can compile the firmware without the SDK. Constant
// values are placeholders. The real header comes with CC3200 SDK 1.5.0.
//*****************************************************************************
#ifndef __SIMPLELINK_H__
#define __SIMPLELINK_H__

typedef unsigned char   _u8;
typedef signed char     _i8;
typedef unsigned short  _u16;
typedef signed short    _i16;
typedef unsigned long   _u32;
typedef signed long     _i32;

// Event types, used only through pointers in network_utils.h.
typedef struct SlWlanEvent_s            SlWlanEvent_t;
typedef struct SlNetAppEvent_s          SlNetAppEvent_t;
typedef struct SlHttpServerEvent_s      SlHttpServerEvent_t;
typedef struct SlHttpServerResponse_s   SlHttpServerResponse_t;
typedef struct SlDeviceEvent_s          SlDeviceEvent_t;
typedef struct SlSockEvent_s            SlSockEvent_t;

#define SL_DEVICE_GENERAL_CONFIGURATION             1
#define SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME   11

extern _i32 sl_DevSet(_u8 DeviceSetId, _u8 Option, _u8 ConfigLen,
                      _u8 *pValues);
extern _i16 sl_Send(_i16 sd, const void *pBuf, _i16 Len, _i16 flags);
extern _i16 sl_Recv(_i16 sd, void *pBuf, _i16 Len, _i16 flags);
extern _i16 sl_Close(_i16 sd);

#endif
