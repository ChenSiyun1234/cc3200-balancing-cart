//*****************************************************************************
// test_safety.c - host unit tests for safety.c (Unity)
//
// Call strings come from fake_hal.h: C coast, 0/1 disable/enable, P controller
// reset, B bus re-open, A alert, L console line.
//*****************************************************************************
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "safety.h"
#include "fake_hal.h"
#include "ref_main_loop.h"

#define TICK_US  5000UL                 // 200 Hz control loop

static tSafety s;

void setUp(void)
{
    Fake_Reset();
    Safety_Init(&s, &g_sFakeHal);
}

void tearDown(void)
{
    TEST_ASSERT_FALSE_MESSAGE(Fake_Overflow(), "fake call buffer overflowed");
}

//*****************************************************************************
// Helpers
//*****************************************************************************
static int FailReads(int n)             // n failed reads while balancing
{
    int iTrip = 0;
    while (n-- > 0) iTrip = Safety_BalanceTick(&s, 0, 0.0f);
    return iTrip;
}

static void FallOver(void)
{
    TEST_ASSERT_TRUE(Safety_BalanceTick(&s, 1, 40.0f));
}

// One fault-stop tick with a good read, then 5 ms pass.
static int FaultTick(float fTilt)
{
    int iRearmed;
    Safety_FaultHold(&s);
    iRearmed = Safety_FaultWatch(&s, 1, fTilt);
    Fake_AdvanceUs(TICK_US);
    return iRearmed;
}

// Fault-stop ticks at fTilt until re-armed; returns ticks used (or -1).
static int TicksUntilRearm(float fTilt, int iMaxTicks)
{
    int i;
    for (i = 1; i <= iMaxTicks; i++)
        if (FaultTick(fTilt)) return i;
    return -1;
}

//*****************************************************************************
// Fall cut-off (35 deg)
//*****************************************************************************
void test_cutoff_is_35_degrees_either_way(void)
{
    TEST_ASSERT_FALSE(Safety_TiltPastLimit(0.0f));
    TEST_ASSERT_FALSE(Safety_TiltPastLimit(35.0f));
    TEST_ASSERT_FALSE(Safety_TiltPastLimit(-35.0f));
    TEST_ASSERT_TRUE(Safety_TiltPastLimit(35.01f));
    TEST_ASSERT_TRUE(Safety_TiltPastLimit(-35.01f));
    TEST_ASSERT_TRUE(Safety_TiltPastLimit(-90.0f));
}

void test_good_read_inside_limit_keeps_balancing(void)
{
    TEST_ASSERT_FALSE(Safety_BalanceTick(&s, 1, 34.9f));
    TEST_ASSERT_FALSE(Safety_BalanceTick(&s, 1, -34.9f));
    TEST_ASSERT_EQUAL_STRING("", Fake_Calls());
}

void test_fall_coasts_and_disarms_the_motors(void)
{
    TEST_ASSERT_TRUE(Safety_BalanceTick(&s, 1, -36.0f));
    TEST_ASSERT_EQUAL_STRING("C0L", Fake_Calls());
    TEST_ASSERT_EQUAL_STRING("FALLEN -> Fault stop\n\r", Fake_LastLog());
}

void test_stale_tilt_is_ignored_on_a_failed_read(void)
{
    TEST_ASSERT_FALSE(Safety_BalanceTick(&s, 0, 80.0f));
    TEST_ASSERT_EQUAL_STRING("C", Fake_Calls());
}

//*****************************************************************************
// IMU glitch counters while balancing (5 and 40)
//*****************************************************************************
void test_each_failed_read_coasts_the_motors(void)
{
    TEST_ASSERT_FALSE(FailReads(4));
    TEST_ASSERT_EQUAL_STRING("CCCC", Fake_Calls());
}

void test_bus_reopens_once_on_the_fifth_failure(void)
{
    FailReads(4);
    Fake_ClearCalls();
    FailReads(1);
    TEST_ASSERT_EQUAL_STRING("CBL", Fake_Calls());
    TEST_ASSERT_EQUAL_STRING("I2C re-init (noise recovery)\n\r", Fake_LastLog());

    Fake_ClearCalls();
    FailReads(35);                                  // failures 6..40
    TEST_ASSERT_EQUAL_INT(0, Fake_Count('B'));
}

void test_fault_stop_on_the_41st_failure_in_a_row(void)
{
    TEST_ASSERT_FALSE(FailReads(40));
    Fake_ClearCalls();
    TEST_ASSERT_TRUE(FailReads(1));
    TEST_ASSERT_EQUAL_STRING("C0L", Fake_Calls());
    TEST_ASSERT_EQUAL_STRING("IMU lost for 41 ticks -> Fault stop\n\r", Fake_LastLog());
}

void test_good_read_restarts_the_failure_count(void)
{
    FailReads(4);
    TEST_ASSERT_FALSE(Safety_BalanceTick(&s, 1, 0.0f));
    FailReads(4);
    TEST_ASSERT_EQUAL_INT(0, Fake_Count('B'));
    FailReads(1);
    TEST_ASSERT_EQUAL_INT(1, Fake_Count('B'));
}

//*****************************************************************************
// Fault stop: motors off, one alert per fall
//*****************************************************************************
void test_first_fault_tick_sends_the_alert(void)
{
    FallOver();
    Fake_ClearCalls();
    Safety_FaultHold(&s);
    TEST_ASSERT_EQUAL_STRING("C0A", Fake_Calls());
    TEST_ASSERT_EQUAL_STRING("ALERT: balance cart FELL OVER (fault stop).", Fake_LastAlert());
}

void test_only_one_alert_per_fall(void)
{
    int i;
    FallOver();
    for (i = 0; i < 1000; i++) FaultTick(60.0f);    // lying on its side
    TEST_ASSERT_EQUAL_INT(1, Fake_AlertCount());
}

void test_failed_send_is_not_retried(void)
{
    Fake_SetAlertResult(-1);
    FallOver();
    Safety_FaultHold(&s);
    Safety_FaultHold(&s);
    TEST_ASSERT_EQUAL_INT(1, Fake_AlertCount());
}

void test_next_fall_after_recovery_sends_a_new_alert(void)
{
    FallOver();
    TEST_ASSERT_TRUE(TicksUntilRearm(0.0f, 1000) > 0);
    FallOver();
    Safety_FaultHold(&s);
    TEST_ASSERT_EQUAL_INT(2, Fake_AlertCount());
}

void test_arm_resets_control_enables_motors_and_rearms_alert(void)
{
    FallOver();
    Safety_FaultHold(&s);                           // alert 1
    Fake_ClearCalls();
    Safety_Arm(&s);                                 // e.g. after calibration
    TEST_ASSERT_EQUAL_STRING("P1", Fake_Calls());
    FallOver();
    Safety_FaultHold(&s);
    TEST_ASSERT_EQUAL_INT(2, Fake_AlertCount());
}

void test_alert_and_console_hooks_are_optional(void)
{
    Safety_Init(&s, &g_sFakeHalQuiet);
    FallOver();
    Safety_FaultHold(&s);
    TEST_ASSERT_EQUAL_STRING("C0C0", Fake_Calls());
    TEST_ASSERT_EQUAL_INT(0, Fake_AlertCount());
}

//*****************************************************************************
// Re-arm: held inside +/-6 deg for more than 1.5 s
//*****************************************************************************
void test_rearms_only_after_more_than_1500_ms_upright(void)
{
    FallOver();
    Safety_FaultHold(&s);
    TEST_ASSERT_FALSE(Safety_FaultWatch(&s, 1, 0.0f));      // window starts
    Fake_AdvanceUs(1500000UL);
    TEST_ASSERT_FALSE(Safety_FaultWatch(&s, 1, 0.0f));      // exactly 1.5 s
    Fake_AdvanceUs(1UL);
    Fake_ClearCalls();
    TEST_ASSERT_TRUE(Safety_FaultWatch(&s, 1, 0.0f));       // 1.5 s + 1 us
    TEST_ASSERT_EQUAL_STRING("P1L", Fake_Calls());
    TEST_ASSERT_EQUAL_STRING("Recovered -> Balancing\n\r", Fake_LastLog());
}

void test_rearms_on_the_302nd_upright_tick_at_200_hz(void)
{
    FallOver();
    TEST_ASSERT_EQUAL_INT(302, TicksUntilRearm(3.0f, 1000));  // 301 x 5 ms > 1.5 s
}

void test_window_edges_are_outside(void)
{
    FallOver();
    TEST_ASSERT_EQUAL_INT(-1, TicksUntilRearm(6.0f, 1000));
    TEST_ASSERT_EQUAL_INT(-1, TicksUntilRearm(-6.0f, 1000));
    TEST_ASSERT_EQUAL_INT(302, TicksUntilRearm(-5.99f, 1000));
}

void test_leaving_the_window_restarts_the_timer(void)
{
    int i;
    FallOver();
    for (i = 0; i < 200; i++) TEST_ASSERT_FALSE(FaultTick(2.0f));   // 1.0 s
    TEST_ASSERT_FALSE(FaultTick(7.0f));                              // tipped out
    TEST_ASSERT_EQUAL_INT(302, TicksUntilRearm(2.0f, 1000));         // full wait
}

void test_failed_reads_do_not_restart_the_timer(void)
{
    int i;
    FallOver();
    for (i = 0; i < 200; i++) TEST_ASSERT_FALSE(FaultTick(2.0f));   // 1.0 s
    for (i = 0; i < 20; i++) {                                        // 0.1 s blind
        Safety_FaultHold(&s);
        TEST_ASSERT_FALSE(Safety_FaultWatch(&s, 0, 0.0f));
        Fake_AdvanceUs(TICK_US);
    }
    TEST_ASSERT_EQUAL_INT(82, TicksUntilRearm(2.0f, 1000));          // 0.4 s more
}

void test_new_fall_needs_a_full_window_again(void)
{
    FallOver();
    TEST_ASSERT_TRUE(TicksUntilRearm(0.0f, 1000) > 0);   // recovered, still upright
    FallOver();                                          // falls again
    TEST_ASSERT_EQUAL_INT(302, TicksUntilRearm(0.0f, 1000));
}

void test_trip_from_autotune_starts_a_new_episode(void)
{
    FallOver();
    TEST_ASSERT_TRUE(TicksUntilRearm(0.0f, 1000) > 0);
    Fake_ClearCalls();
    Safety_Trip(&s);                                     // auto-tune abort path
    TEST_ASSERT_EQUAL_STRING("C0", Fake_Calls());
    TEST_ASSERT_EQUAL_INT(302, TicksUntilRearm(0.0f, 1000));
}

void test_rearm_timer_survives_the_clock_wrap(void)
{
    Fake_SetTicks(80UL * 1000UL);       // 1 ms before the 32-bit counter wraps
    FallOver();
    TEST_ASSERT_EQUAL_INT(302, TicksUntilRearm(0.0f, 1000));
}

//*****************************************************************************
// Bus re-open while in fault stop (every 200 failed reads)
//*****************************************************************************
void test_fault_stop_reopens_the_bus_every_200_failures(void)
{
    int i;
    FallOver();
    Fake_ClearCalls();
    for (i = 0; i < 199; i++) Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(0, Fake_Count('B'));
    Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_STRING("BL", Fake_Calls());
    TEST_ASSERT_EQUAL_STRING("I2C re-init + bus recover (fault)\n\r", Fake_LastLog());
    for (i = 0; i < 200; i++) Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(2, Fake_Count('B'));
}

void test_failure_count_carries_over_from_balance(void)
{
    int i;
    FailReads(41);                                   // IMU lost -> fault stop
    Fake_ClearCalls();
    for (i = 0; i < 158; i++) Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(0, Fake_Count('B'));       // count is 199
    Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(1, Fake_Count('B'));       // count reached 200
}

void test_trip_keeps_the_failure_count(void)
{
    int i;
    FailReads(3);                                    // auto-tune starts after a few
    Safety_Trip(&s);                                 // failures, then the cart falls
    Fake_ClearCalls();
    for (i = 0; i < 196; i++) Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(0, Fake_Count('B'));       // count is 199
    Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(1, Fake_Count('B'));       // count reached 200
}

void test_good_read_in_fault_stop_restarts_the_count(void)
{
    int i;
    FallOver();
    for (i = 0; i < 150; i++) Safety_FaultWatch(&s, 0, 0.0f);
    Safety_FaultWatch(&s, 1, 50.0f);
    Fake_ClearCalls();
    for (i = 0; i < 199; i++) Safety_FaultWatch(&s, 0, 0.0f);
    TEST_ASSERT_EQUAL_INT(0, Fake_Count('B'));
}

//*****************************************************************************
// Same behavior as main() at tag eec172-final
//
// Random runs drive the old state switch (ref_main_loop.c) and the new one
// (the code below mirrors main.c) with the same sensor results and clock.
// Every hook call, console line and state change must match on every tick.
//*****************************************************************************
typedef struct { uint32_t x; } tRng;

static uint32_t Rng(tRng *r)            // xorshift32
{
    r->x ^= r->x << 13;
    r->x ^= r->x >> 17;
    r->x ^= r->x << 5;
    return r->x;
}
static int   RngInt(tRng *r, int n)     { return (int)(Rng(r) % (uint32_t)n); }
static float RngF(tRng *r, float lo, float hi)
{
    return lo + (hi - lo) * (float)(Rng(r) & 0xFFFFFF) / (float)0x1000000;
}

// State switch as it is in main.c now.
static int New_Step(int eState, int *piSensorsOK, const tTickInputs *psIn, int *piDrive)
{
    int iImuOk;
    *piDrive = 0;
    switch (eState)
    {
    case ST_INIT_HW:
        if (!*piSensorsOK) { eState = ST_FAULT_STOP; break; }
        g_sFakeHal.pfnLog("Sensors OK -> calibrating\n\r");
        eState = ST_CALIBRATE;
        break;
    case ST_CALIBRATE:
        if (psIn->iCalibrateRet < 0) { *piSensorsOK = 0; eState = ST_FAULT_STOP; break; }
        Safety_Arm(&s);
        g_sFakeHal.pfnLog("Calibrated. Balancing.\n\r");
        eState = ST_BALANCE;
        break;
    case ST_BALANCE:
        iImuOk = (psIn->iSampleRet >= 0);
        if (Safety_BalanceTick(&s, iImuOk, psIn->fTilt)) { eState = ST_FAULT_STOP; break; }
        if (!iImuOk) break;
        *piDrive = 1;
        break;
    case ST_FAULT_STOP:
        Safety_FaultHold(&s);
        iImuOk = *piSensorsOK && (psIn->iSampleRet == 0);
        if (Safety_FaultWatch(&s, iImuOk, psIn->fTilt)) eState = ST_BALANCE;
        break;
    case ST_AUTOTUNE:
        if (psIn->iSampleRet < 0) break;
        if (Safety_TiltPastLimit(psIn->fTilt)) {    // Balance_IsFallen()
            Safety_Trip(&s);
            g_sFakeHal.pfnLog("AUTOTUNE aborted: cart fell. Check the feedback SIGN "
                              "(MOTOR_*_INVERT) and try again.\n\r");
            eState = ST_FAULT_STOP;
            break;
        }
        if (psIn->iAutotuneEnds) eState = ST_BALANCE;
        break;
    }
    return eState;
}

static float PickTilt(tRng *r, int iMode, float fPrev)
{
    static const float afEdge[] = { 5.999f, 6.0f, 6.001f, 34.999f, 35.0f, 35.001f, 0.0f };
    float f;
    switch (iMode)
    {
    case 0:  return RngF(r, -8.0f, 8.0f);                           // wobbling
    case 1:  f = RngF(r, 36.0f, 90.0f); return RngInt(r, 2) ? f : -f; // lying down
    case 2:  return RngF(r, -5.5f, 5.5f);                           // held upright
    case 3:  f = afEdge[RngInt(r, 7)]; return RngInt(r, 2) ? f : -f; // edge values
    default: return fPrev + RngF(r, -1.0f, 1.0f);                   // slow drift
    }
}

static int PickSample(tRng *r, int iMode)
{
    static const int aiOdd[] = { -2, -1, 0, 1 };
    switch (iMode)
    {
    case 0:  return (RngInt(r, 200) == 0) ? -1 : 0;     // healthy bus
    case 1:  return (RngInt(r, 10) < 3) ? -1 : 0;       // noisy bus
    case 2:  return -1;                                 // dead bus
    default: return aiOdd[RngInt(r, 4)];                // odd return values
    }
}

typedef struct { long lFalls, lImuLost, lBusBal, lBusFault, lRearms, lAlerts, lAbort, lCalFail; } tSeen;

static void RunRandom(uint32_t ulSeed, long lTicks, tSeen *psSeen)
{
    char acRefCalls[2048], acRefLog[2048], acMsg[256];
    tRng r;
    tTickInputs in;
    int iStRef = ST_INIT_HW, iStNew = ST_INIT_HW, iOkNew;
    int iTiltMode = 0, iTiltLeft = 0, iSampMode = 0, iSampLeft = 0;
    long t;

    r.x = ulSeed;
    Fake_Reset();
    iOkNew = (ulSeed % 5 != 0);                 // some runs boot with sensors failed
    Ref_Init(iOkNew);
    Safety_Init(&s, &g_sFakeHal);
    memset(&in, 0, sizeof(in));

    for (t = 0; t < lTicks; t++)
    {
        int iDriveRef, iDriveNew;

        if (iTiltLeft-- <= 0) { iTiltMode = RngInt(&r, 5); iTiltLeft = 1 + RngInt(&r, 400); }
        if (iSampLeft-- <= 0) {
            int k = RngInt(&r, 20);
            iSampMode = (k < 12) ? 0 : (k < 16) ? 1 : (k < 19) ? 2 : 3;
            iSampLeft = 1 + RngInt(&r, 300);
        }
        in.fTilt         = PickTilt(&r, iTiltMode, in.fTilt);
        in.iSampleRet    = PickSample(&r, iSampMode);
        in.iCalibrateRet = (RngInt(&r, 8) == 0) ? -1 : 0;
        in.iAutotuneEnds = (RngInt(&r, 100) == 0);

        // Auto-tune request ('t' key) and power cycles, the same for both.
        if (iStRef == ST_BALANCE && RngInt(&r, 500) == 0)
            iStRef = iStNew = ST_AUTOTUNE;
        if (RngInt(&r, 20000) == 0) {
            iOkNew = (RngInt(&r, 4) != 0);
            Ref_Init(iOkNew);
            Safety_Init(&s, &g_sFakeHal);
            iStRef = iStNew = ST_INIT_HW;
        }

        Fake_ClearCalls();
        iStRef = Ref_Step(iStRef, &in, &iDriveRef);
        snprintf(acRefCalls, sizeof(acRefCalls), "%s", Fake_Calls());
        snprintf(acRefLog, sizeof(acRefLog), "%s", Fake_LogText());

        Fake_ClearCalls();
        iStNew = New_Step(iStNew, &iOkNew, &in, &iDriveNew);

        if (strcmp(acRefCalls, Fake_Calls()) != 0 || strcmp(acRefLog, Fake_LogText()) != 0 ||
            iStRef != iStNew || iDriveRef != iDriveNew || Ref_SensorsOk() != iOkNew)
        {
            snprintf(acMsg, sizeof(acMsg), "seed %lu tick %ld: calls '%.60s' vs '%.60s', state %d vs %d",
                     (unsigned long)ulSeed, t, acRefCalls, Fake_Calls(), iStRef, iStNew);
            TEST_FAIL_MESSAGE(acMsg);
        }

        if (strstr(Fake_LogText(), "FALLEN"))          psSeen->lFalls++;
        if (strstr(Fake_LogText(), "IMU lost"))        psSeen->lImuLost++;
        if (strstr(Fake_LogText(), "noise recovery"))  psSeen->lBusBal++;
        if (strstr(Fake_LogText(), "(fault)"))         psSeen->lBusFault++;
        if (strstr(Fake_LogText(), "Recovered"))       psSeen->lRearms++;
        if (strstr(Fake_LogText(), "AUTOTUNE aborted")) psSeen->lAbort++;
        psSeen->lAlerts += Fake_Count('A');
        if (iStNew == ST_FAULT_STOP && !iOkNew)         psSeen->lCalFail++;

        // Mostly 5 ms; sometimes jitter, a blocking alert send, or a long gap
        // that wraps the 32-bit clock (~53.7 s).
        {
            int k = RngInt(&r, 10000);
            if (k < 100)       Fake_AdvanceUs((unsigned long)RngInt(&r, 20000));
            else if (k < 130)  Fake_AdvanceUs(1000000UL + (unsigned long)RngInt(&r, 19000000));
            else if (k < 132)  Fake_AdvanceUs(60000000UL);
            else               Fake_AdvanceUs(TICK_US);
        }
    }
}

void test_random_runs_match_the_original_main_loop(void)
{
    tSeen sSeen;
    uint32_t ulSeed;
    memset(&sSeen, 0, sizeof(sSeen));
    for (ulSeed = 1; ulSeed <= 20; ulSeed++)
        RunRandom(ulSeed * 2654435761UL, 50000L, &sSeen);

    // Make sure the runs reached every branch.
    TEST_ASSERT_TRUE(sSeen.lFalls > 0);
    TEST_ASSERT_TRUE(sSeen.lImuLost > 0);
    TEST_ASSERT_TRUE(sSeen.lBusBal > 0);
    TEST_ASSERT_TRUE(sSeen.lBusFault > 0);
    TEST_ASSERT_TRUE(sSeen.lRearms > 0);
    TEST_ASSERT_TRUE(sSeen.lAlerts > 0);
    TEST_ASSERT_TRUE(sSeen.lAbort > 0);
    TEST_ASSERT_TRUE(sSeen.lCalFail > 0);
    printf("  1,000,000 ticks: %ld falls, %ld IMU-lost stops, %ld + %ld bus re-opens, "
           "%ld re-arms, %ld alerts, %ld auto-tune aborts\n",
           sSeen.lFalls, sSeen.lImuLost, sSeen.lBusBal, sSeen.lBusFault,
           sSeen.lRearms, sSeen.lAlerts, sSeen.lAbort);
}

//*****************************************************************************
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cutoff_is_35_degrees_either_way);
    RUN_TEST(test_good_read_inside_limit_keeps_balancing);
    RUN_TEST(test_fall_coasts_and_disarms_the_motors);
    RUN_TEST(test_stale_tilt_is_ignored_on_a_failed_read);

    RUN_TEST(test_each_failed_read_coasts_the_motors);
    RUN_TEST(test_bus_reopens_once_on_the_fifth_failure);
    RUN_TEST(test_fault_stop_on_the_41st_failure_in_a_row);
    RUN_TEST(test_good_read_restarts_the_failure_count);

    RUN_TEST(test_first_fault_tick_sends_the_alert);
    RUN_TEST(test_only_one_alert_per_fall);
    RUN_TEST(test_failed_send_is_not_retried);
    RUN_TEST(test_next_fall_after_recovery_sends_a_new_alert);
    RUN_TEST(test_arm_resets_control_enables_motors_and_rearms_alert);
    RUN_TEST(test_alert_and_console_hooks_are_optional);

    RUN_TEST(test_rearms_only_after_more_than_1500_ms_upright);
    RUN_TEST(test_rearms_on_the_302nd_upright_tick_at_200_hz);
    RUN_TEST(test_window_edges_are_outside);
    RUN_TEST(test_leaving_the_window_restarts_the_timer);
    RUN_TEST(test_failed_reads_do_not_restart_the_timer);
    RUN_TEST(test_new_fall_needs_a_full_window_again);
    RUN_TEST(test_trip_from_autotune_starts_a_new_episode);
    RUN_TEST(test_rearm_timer_survives_the_clock_wrap);

    RUN_TEST(test_fault_stop_reopens_the_bus_every_200_failures);
    RUN_TEST(test_failure_count_carries_over_from_balance);
    RUN_TEST(test_trip_keeps_the_failure_count);
    RUN_TEST(test_good_read_in_fault_stop_restarts_the_count);

    RUN_TEST(test_random_runs_match_the_original_main_loop);

    return UNITY_END();
}
