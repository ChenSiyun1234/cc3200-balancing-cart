//*****************************************************************************
// safety.h
//
// Fault stop and alert logic for the balance cart, split out of main.c so it
// can be unit-tested on a PC. The module never touches hardware itself:
// main.c gives it a table of hooks (motors, I2C bus, clock, alert, console).
//
// Rules:
//   - Fall: |tilt| > BALANCE_SAFE_TILT_DEG (35 deg) -> fault stop.
//   - IMU glitches while balancing: coast on every failed read, re-open the
//     I2C bus on the 5th failure in a row, fault stop once more than 40 fail
//     in a row (~0.2 s at 200 Hz).
//   - Fault stop: motors off, one alert per fall. Re-arm after the cart is
//     held inside +/-6 deg for more than 1.5 s. While reads keep failing,
//     re-open the bus every 200 failed reads (~1 s).
//*****************************************************************************
#ifndef __SAFETY_H__
#define __SAFETY_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SAFETY_BUS_REOPEN_FAILS    5          // re-open the bus at this failure count
#define SAFETY_FAULT_FAILS         40         // fault stop once the count passes this
#define SAFETY_FAULT_REOPEN_EVERY  200        // in fault stop: re-open every N failures
#define SAFETY_UPRIGHT_DEG         6.0f       // re-arm window is -6 < tilt < +6 deg
#define SAFETY_REARM_US            1500000UL  // time in the window before re-arming

#define SAFETY_ALERT_MSG  "ALERT: balance cart FELL OVER (fault stop)."

// Hardware hooks, filled in by main.c. pfnSendAlert and pfnLog may be NULL.
typedef struct
{
    void          (*pfnMotorCoast)(void);                 // both wheels coast
    void          (*pfnMotorEnable)(unsigned char ucOn);  // H-bridge enable
    void          (*pfnResetControl)(void);               // clear loop integrators
    void          (*pfnBusRecover)(void);                 // close, un-stick, re-open I2C
    int           (*pfnSendAlert)(const char *pcMsg);     // blocking alert
    unsigned long (*pfnTicks)(void);                      // clock sample
    unsigned long (*pfnElapsedUs)(unsigned long ulStart); // us since a sample
    int           (*pfnLog)(const char *pcFormat, ...);   // console
} tSafetyHal;

typedef struct
{
    const tSafetyHal *psHal;
    int           iImuFail;        // failed IMU reads in a row (kept into fault stop)
    int           iWasUpright;     // last good read in fault stop was in the window
    unsigned long ulUprightSince;  // pfnTicks() when the cart entered the window
    int           iAlertSent;      // alert already sent for this fall
} tSafety;

//! Clear all counters and remember the hooks.
extern void Safety_Init(tSafety *psS, const tSafetyHal *psHal);

//! 1 if |tilt| is past the fall cut-off (BALANCE_SAFE_TILT_DEG).
extern int  Safety_TiltPastLimit(float fTiltDeg);

//! Arm the motors: clear the controller, enable the H-bridge and allow one
//! new alert. Used after calibration and on recovery.
extern void Safety_Arm(tSafety *psS);

//! Motors off and start a new fault episode. Safety_BalanceTick() does this
//! on its own; the auto-tune abort path calls it directly.
extern void Safety_Trip(tSafety *psS);

//! BALANCE tick, after Balance_Sample(). iSampleOk = 1 if the read worked.
//! \return 1 if the cart must go to FAULT_STOP now.
extern int  Safety_BalanceTick(tSafety *psS, int iSampleOk, float fTiltDeg);

//! FAULT_STOP tick, part 1 (before sampling): motors off, one alert per fall.
extern void Safety_FaultHold(tSafety *psS);

//! FAULT_STOP tick, part 2 (after sampling).
//! \return 1 if the cart was re-armed and can go back to BALANCE.
extern int  Safety_FaultWatch(tSafety *psS, int iSampleOk, float fTiltDeg);

#ifdef __cplusplus
}
#endif
#endif // __SAFETY_H__
