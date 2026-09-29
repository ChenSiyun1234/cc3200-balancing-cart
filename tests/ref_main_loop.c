//*****************************************************************************
// ref_main_loop.c - reference model for the equivalence test (host only)
//
// The INIT_HW, CALIBRATE, BALANCE, FAULT_STOP and AUTOTUNE cases of main() at
// tag eec172-final, copied as they were. Driver calls go to the same fakes the
// safety.c tests use, and sensor results come from the test, so the original
// code and the refactored code can be compared call by call. Lines that only
// touch the OLED, the loop timer (ulPrev), the relay auto-tuner or the motor
// PWM are left out; they are the same in both versions.
//*****************************************************************************
#include "ref_main_loop.h"
#include "fake_hal.h"
#include "hw_config.h"          // ENABLE_WIFI_EMAIL, BALANCE_SAFE_TILT_DEG

//---- Driver calls -> fakes ---------------------------------------------------
#define Motor_Coast()           g_sFakeHal.pfnMotorCoast()
#define Motor_Enable(x)         g_sFakeHal.pfnMotorEnable(x)
#define Balance_ResetPID()      g_sFakeHal.pfnResetControl()
#define Email_Send(m)           g_sFakeHal.pfnSendAlert(m)
#define Clock_Ticks()           g_sFakeHal.pfnTicks()
#define Clock_ElapsedUs(t)      g_sFakeHal.pfnElapsedUs(t)
#define LOG(...)                g_sFakeHal.pfnLog(__VA_ARGS__)
// Close + recover + open always ran together; they count as one bus re-open.
#define I2C_IF_Close()          ((void)0)
#define I2C_BusRecover()        g_sFakeHal.pfnBusRecover()
#define I2C_IF_Open(m)          ((void)(m))
#define I2C_MASTER_MODE_STD     0

//---- Sensor results for this tick --------------------------------------------
static const tTickInputs *s_psIn;
#define Balance_Sample(dt)      ((void)(dt), s_psIn->iSampleRet)
#define Balance_Calibrate()     (s_psIn->iCalibrateRet)
#define Balance_GetTilt()       (s_psIn->fTilt)

// Balance_IsFallen() as it was in balance.c at eec172-final.
static int Balance_IsFallen(void)
{
    float g_fTilt = s_psIn->fTilt;
    float a = g_fTilt < 0 ? -g_fTilt : g_fTilt;
    return (a > BALANCE_SAFE_TILT_DEG);
}

//---- main() locals used by the fault handling ----------------------------------
static int           iSensorsOK;
static unsigned long ulUprightSince;
static int           iWasUpright;
static int           iImuFail;
#if ENABLE_WIFI_EMAIL
static int           iFallEmailSent;
#endif

void Ref_Init(int iSensorsOkAtBoot)
{
    iSensorsOK     = iSensorsOkAtBoot;
    ulUprightSince = 0;
    iWasUpright    = 0;
    iImuFail       = 0;
#if ENABLE_WIFI_EMAIL
    iFallEmailSent = 0;
#endif
}

int Ref_SensorsOk(void) { return iSensorsOK; }

int Ref_Step(int eState, const tTickInputs *psIn, int *piDrive)
{
    float fDt = 0.005f;
    s_psIn   = psIn;
    *piDrive = 0;

        switch (eState)
        {
        //-----------------------------------------------------------------
        case ST_INIT_HW:
            if (!iSensorsOK) { eState = ST_FAULT_STOP; break; }
            LOG("Sensors OK -> calibrating\n\r");
            eState = ST_CALIBRATE;
            break;

        //-----------------------------------------------------------------
        case ST_CALIBRATE:
            // Cart must be held upright & still here (see OLED/console).
            if (Balance_Calibrate() < 0) { iSensorsOK = 0; eState = ST_FAULT_STOP; break; }
            Balance_ResetPID();
            Motor_Enable(1);                       // arm the H-bridge
#if ENABLE_WIFI_EMAIL
            iFallEmailSent = 0;                    // re-arm the fall alert
#endif
            LOG("Calibrated. Balancing.\n\r");
            eState = ST_BALANCE;
            break;

        //-----------------------------------------------------------------
        case ST_BALANCE:
            if (Balance_Sample(fDt) < 0) {         // IMU read glitched this tick
                // Coast while blind (never drive on stale data); after a few fails
                // re-open the bus; only fault if it stays dead for ~0.2 s.
                Motor_Coast();
                iImuFail++;
                if (iImuFail == 5) {
                    I2C_IF_Close();
                    I2C_BusRecover();                   // clock a stuck slave free
                    I2C_IF_Open(I2C_MASTER_MODE_STD);
                    LOG("I2C re-init (noise recovery)\n\r");
                }
                if (iImuFail > 40) {
                    Motor_Enable(0);
                    LOG("IMU lost for %d ticks -> Fault stop\n\r", iImuFail);
                    iWasUpright = 0; eState = ST_FAULT_STOP;
                }
                break;
            }
            iImuFail = 0;                          // good read -> reset glitch count
            if (Balance_IsFallen()) {              // tipped past the safe angle
                Motor_Coast(); Motor_Enable(0);
                LOG("FALLEN -> Fault stop\n\r");
                iWasUpright = 0;
                eState = ST_FAULT_STOP;
                break;
            }
            *piDrive = 1;                          // (cascaded control left out)
            break;

        //-----------------------------------------------------------------
        case ST_FAULT_STOP:
            Motor_Coast(); Motor_Enable(0);
#if ENABLE_WIFI_EMAIL
            // Email once per fall (blocking send ok here: cart is down, motors off).
            if (!iFallEmailSent) {
                Email_Send("ALERT: balance cart FELL OVER (fault stop).");
                iFallEmailSent = 1;
            }
#endif
            // Keep the filter live to auto-recover once the cart is stood back
            // up and held near upright for ~1.5 s.
            if (iSensorsOK && Balance_Sample(fDt) == 0)
            {
                float fT = Balance_GetTilt();
                int   iUpright = (fT > -6.0f && fT < 6.0f);
                iImuFail = 0;
                if (iUpright && !iWasUpright) ulUprightSince = Clock_Ticks();
                iWasUpright = iUpright;
                if (iUpright && Clock_ElapsedUs(ulUprightSince) > 1500000UL)
                {
                    Balance_ResetPID();
                    Motor_Enable(1);
#if ENABLE_WIFI_EMAIL
                    iFallEmailSent = 0;            // re-arm the fall alert
#endif
                    LOG("Recovered -> Balancing\n\r");
                    eState = ST_BALANCE;
                }
            }
            else if (((++iImuFail) % 200) == 0)   // bus still dead: unstick ~1/s
            {
                I2C_IF_Close();
                I2C_BusRecover();                  // clock a stuck slave free
                I2C_IF_Open(I2C_MASTER_MODE_STD);
                LOG("I2C re-init + bus recover (fault)\n\r");
            }
            break;

        //-----------------------------------------------------------------
        case ST_AUTOTUNE:
            if (Balance_Sample(fDt) < 0) break;          // skip glitch ticks
            if (Balance_IsFallen()) {                    // fell during the test
                Motor_Coast(); Motor_Enable(0);
                LOG("AUTOTUNE aborted: cart fell. Check the feedback SIGN "
                    "(MOTOR_*_INVERT) and try again.\n\r");
                iWasUpright = 0; eState = ST_FAULT_STOP;
                break;
            }
            if (psIn->iAutotuneEnds) eState = ST_BALANCE;   // (relay step left out)
            break;
        }

    return eState;
}
