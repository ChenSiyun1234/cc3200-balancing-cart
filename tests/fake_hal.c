//*****************************************************************************
// fake_hal.c - recording fakes for the safety.c hooks (see fake_hal.h)
//*****************************************************************************
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "fake_hal.h"

#define TICKS_PER_US   80UL            // CC3200 timer runs at 80 MHz
#define MASK32         0xFFFFFFFFUL    // and is 32 bits wide

static unsigned long s_ulTicks;
static char   s_acCalls[16384];
static size_t s_uCalls;
static char   s_acLogText[16384];
static size_t s_uLogText;
static char   s_acLastLog[160];
static char   s_acLastAlert[160];
static int    s_iAlerts;
static int    s_iAlertResult;
static int    s_iOverflow;

static void Record(char c)
{
    if (s_uCalls + 1 >= sizeof(s_acCalls)) { s_iOverflow = 1; return; }
    s_acCalls[s_uCalls++] = c;
    s_acCalls[s_uCalls] = '\0';
}

//*****************************************************************************
// Hooks
//*****************************************************************************
static void FakeMotorCoast(void)              { Record('C'); }
static void FakeMotorEnable(unsigned char ucOn) { Record(ucOn ? '1' : '0'); }
static void FakeResetControl(void)            { Record('P'); }
static void FakeBusRecover(void)              { Record('B'); }

static int FakeSendAlert(const char *pcMsg)
{
    Record('A');
    s_iAlerts++;
    snprintf(s_acLastAlert, sizeof(s_acLastAlert), "%s", pcMsg);
    return s_iAlertResult;
}

static unsigned long FakeTicks(void)
{
    return s_ulTicks;
}

// Same math as Clock_ElapsedUs() on the 32-bit target.
static unsigned long FakeElapsedUs(unsigned long ulStart)
{
    return ((ulStart - s_ulTicks) & MASK32) / TICKS_PER_US;
}

static int FakeLog(const char *pcFormat, ...)
{
    va_list ap;
    int n;
    va_start(ap, pcFormat);
    n = vsnprintf(s_acLastLog, sizeof(s_acLastLog), pcFormat, ap);
    va_end(ap);
    Record('L');
    if (s_uLogText + strlen(s_acLastLog) + 1 >= sizeof(s_acLogText)) {
        s_iOverflow = 1;
    } else {
        memcpy(&s_acLogText[s_uLogText], s_acLastLog, strlen(s_acLastLog) + 1);
        s_uLogText += strlen(s_acLastLog);
    }
    return n;
}

const tSafetyHal g_sFakeHal = {
    .pfnMotorCoast   = FakeMotorCoast,
    .pfnMotorEnable  = FakeMotorEnable,
    .pfnResetControl = FakeResetControl,
    .pfnBusRecover   = FakeBusRecover,
    .pfnSendAlert    = FakeSendAlert,
    .pfnTicks        = FakeTicks,
    .pfnElapsedUs    = FakeElapsedUs,
    .pfnLog          = FakeLog,
};

const tSafetyHal g_sFakeHalQuiet = {
    .pfnMotorCoast   = FakeMotorCoast,
    .pfnMotorEnable  = FakeMotorEnable,
    .pfnResetControl = FakeResetControl,
    .pfnBusRecover   = FakeBusRecover,
    .pfnSendAlert    = NULL,
    .pfnTicks        = FakeTicks,
    .pfnElapsedUs    = FakeElapsedUs,
    .pfnLog          = NULL,
};

//*****************************************************************************
// Test controls
//*****************************************************************************
void Fake_ClearCalls(void)
{
    s_uCalls = 0;
    s_acCalls[0] = '\0';
    s_uLogText = 0;
    s_acLogText[0] = '\0';
}

void Fake_Reset(void)
{
    Fake_ClearCalls();
    s_ulTicks = MASK32;                 // the timer reloads from 0xFFFFFFFF
    s_acLastLog[0] = '\0';
    s_acLastAlert[0] = '\0';
    s_iAlerts = 0;
    s_iAlertResult = 0;
    s_iOverflow = 0;
}

const char *Fake_Calls(void) { return s_acCalls; }
int Fake_Overflow(void)      { return s_iOverflow; }

int Fake_Count(char cCall)
{
    int n = 0;
    const char *p;
    for (p = s_acCalls; *p; p++)
        if (*p == cCall) n++;
    return n;
}

void Fake_AdvanceUs(unsigned long ulUs)
{
    s_ulTicks = (s_ulTicks - ulUs * TICKS_PER_US) & MASK32;
}

void Fake_SetTicks(unsigned long ulTicks) { s_ulTicks = ulTicks & MASK32; }

void Fake_SetAlertResult(int iResult) { s_iAlertResult = iResult; }
int Fake_AlertCount(void)             { return s_iAlerts; }
const char *Fake_LastAlert(void)      { return s_acLastAlert; }

const char *Fake_LastLog(void)        { return s_acLastLog; }
const char *Fake_LogText(void)        { return s_acLogText; }
