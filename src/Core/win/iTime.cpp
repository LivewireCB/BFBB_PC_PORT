#include "iTime.h"
#include "iSystem.h"

#include <types.h>
//#include <dolphin.h>

static iTime sStartupTime;
static F32 sGameTime;

void iTimeInit()
{
    sStartupTime = (iTime)clock();
}

void iTimeExit()
{
}

iTime iTimeGet()
{
    return (iTime)clock() - sStartupTime;
}

F32 iTimeDiffSec(iTime time)
{
    return (F32)time / (GET_BUS_FREQUENCY() / 4);
}

F32 iTimeDiffSec(iTime t0, iTime t1)
{
    return iTimeDiffSec(t1 - t0);
}

F32 iTimeGetGame()
{
    return sGameTime;
}

void iTimeGameAdvance(F32 elapsed)
{
    sGameTime += elapsed;
}

void iTimeSetGame(F32 time)
{
    sGameTime = time;
}

void iProfileClear(U32 sceneID)
{
    // Redacted. :}
}

void iFuncProfileDump()
{
    // Redacted. :}
}

void iFuncProfileParse(char* elfPath, S32 profile)
{
    // Redacted. :}
}
