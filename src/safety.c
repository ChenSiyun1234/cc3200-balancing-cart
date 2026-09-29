//*****************************************************************************
// safety.c  -  fault stop and alert logic (see safety.h)
//
// Moved out of main() with the same thresholds and the same order of motor,
// bus, alert and console calls.
//*****************************************************************************
#include "safety.h"
#include "hw_config.h"      // BALANCE_SAFE_TILT_DEG

#define HAL(psS)            ((psS)->psHal)
#define SLOG(psS, ...)      do { if (HAL(psS)->pfnLog) HAL(psS)->pfnLog(__VA_ARGS__); } while (0)

//*****************************************************************************
void Safety_Init(tSafety *psS, const tSafetyHal *psHal)
{
    psS->psHal          = psHal;
    psS->iImuFail       = 0;
    psS->iWasUpright    = 0;
    psS->ulUprightSince = 0;
    psS->iAlertSent     = 0;
}

//*****************************************************************************
int Safety_TiltPastLimit(float fTiltDeg)
{
    float a = fTiltDeg < 0 ? -fTiltDeg : fTiltDeg;
    return (a > BALANCE_SAFE_TILT_DEG);
}

//*****************************************************************************
void Safety_Arm(tSafety *psS)
{
    HAL(psS)->pfnResetControl();
    HAL(psS)->pfnMotorEnable(1);            // arm the H-bridge
    psS->iAlertSent = 0;                    // re-arm the fall alert
}

void Safety_Trip(tSafety *psS)
{
    HAL(psS)->pfnMotorCoast();
    HAL(psS)->pfnMotorEnable(0);
    psS->iWasUpright = 0;                   // re-arm timer starts fresh
}

//*****************************************************************************
int Safety_BalanceTick(tSafety *psS, int iSampleOk, float fTiltDeg)
{
    if (!iSampleOk)                         // IMU read glitched this tick
    {
        // Coast while blind (never drive on stale data); after a few fails
        // re-open the bus; only fault if it stays dead for ~0.2 s.
        HAL(psS)->pfnMotorCoast();
        psS->iImuFail++;
        if (psS->iImuFail == SAFETY_BUS_REOPEN_FAILS)
        {
            HAL(psS)->pfnBusRecover();
            SLOG(psS, "I2C re-init (noise recovery)\n\r");
        }
        if (psS->iImuFail > SAFETY_FAULT_FAILS)
        {
            HAL(psS)->pfnMotorEnable(0);
            SLOG(psS, "IMU lost for %d ticks -> Fault stop\n\r", psS->iImuFail);
            psS->iWasUpright = 0;
            return 1;
        }
        return 0;
    }

    psS->iImuFail = 0;                      // good read -> reset glitch count
    if (Safety_TiltPastLimit(fTiltDeg))     // tipped past the safe angle
    {
        Safety_Trip(psS);
        SLOG(psS, "FALLEN -> Fault stop\n\r");
        return 1;
    }
    return 0;
}

//*****************************************************************************
void Safety_FaultHold(tSafety *psS)
{
    HAL(psS)->pfnMotorCoast();
    HAL(psS)->pfnMotorEnable(0);

    // One alert per fall (blocking send is fine here: cart is down, motors off).
    // The attempt counts even if the send fails; there is no retry.
    if (!psS->iAlertSent)
    {
        if (HAL(psS)->pfnSendAlert)
            HAL(psS)->pfnSendAlert(SAFETY_ALERT_MSG);
        psS->iAlertSent = 1;
    }
}

int Safety_FaultWatch(tSafety *psS, int iSampleOk, float fTiltDeg)
{
    // The filter stays live so the cart can re-arm once it is stood back up
    // and held near upright for ~1.5 s.
    if (iSampleOk)
    {
        int iUpright = (fTiltDeg > -SAFETY_UPRIGHT_DEG && fTiltDeg < SAFETY_UPRIGHT_DEG);
        psS->iImuFail = 0;
        if (iUpright && !psS->iWasUpright)
            psS->ulUprightSince = HAL(psS)->pfnTicks();
        psS->iWasUpright = iUpright;
        if (iUpright && HAL(psS)->pfnElapsedUs(psS->ulUprightSince) > SAFETY_REARM_US)
        {
            Safety_Arm(psS);
            SLOG(psS, "Recovered -> Balancing\n\r");
            return 1;
        }
    }
    else if (((++psS->iImuFail) % SAFETY_FAULT_REOPEN_EVERY) == 0)
    {
        HAL(psS)->pfnBusRecover();          // bus still dead: unstick ~1/s
        SLOG(psS, "I2C re-init + bus recover (fault)\n\r");
    }
    return 0;
}
