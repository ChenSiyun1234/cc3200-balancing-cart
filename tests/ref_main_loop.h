//*****************************************************************************
// ref_main_loop.h - the fault handling of main() at tag eec172-final, kept as
// a reference model for the equivalence test (host only)
//*****************************************************************************
#ifndef REF_MAIN_LOOP_H
#define REF_MAIN_LOOP_H

// Same order as tState in main.c.
enum { ST_INIT_HW = 0, ST_CALIBRATE, ST_BALANCE, ST_FAULT_STOP, ST_AUTOTUNE };

// What the hardware reports on one tick (chosen by the test).
typedef struct
{
    int   iSampleRet;       // Balance_Sample() return value
    float fTilt;            // Balance_GetTilt() after that sample
    int   iCalibrateRet;    // Balance_Calibrate() return value
    int   iAutotuneEnds;    // relay auto-tune finishes this tick
} tTickInputs;

// Reset main()'s locals, as after a power-on.
void Ref_Init(int iSensorsOkAtBoot);

// One pass of the original state switch. Returns the next state.
// *piDrive = 1 when BALANCE would have driven the motors this tick.
int  Ref_Step(int eState, const tTickInputs *psIn, int *piDrive);

// main()'s iSensorsOK flag (CALIBRATE clears it on failure).
int  Ref_SensorsOk(void);

#endif // REF_MAIN_LOOP_H
