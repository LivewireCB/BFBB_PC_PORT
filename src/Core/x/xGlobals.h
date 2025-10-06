#ifndef XGLOBALS_H
#define XGLOBALS_H

#include "xCamera.h"
#include "xPad.h"
#include "xUpdateCull.h"
#include "iCamera.h"
#include "iTime.h"

#include <rwcore.h>
#include <rpworld.h>

struct xGlobals
{
    xCamera camera;
    _tagxPad* pad0;
    _tagxPad* pad1;
    _tagxPad* pad2;
    _tagxPad* pad3;
    S32 profile;
    char profFunc[6][128];
    xUpdateCullMgr* updateMgr;
    S32 sceneFirst;
    char sceneStart[32];
    RpWorld* currWorld;
    iFogParams fog;
    iFogParams fogA;
    iFogParams fogB;
    iTime fog_t0;
    iTime fog_t1;
    S32 option_vibration;
    U32 QuarterSpeed;
    F32 update_dt;
    S32 useHIPHOP;
    U8 NoMusic;
    U8 currentActivePad;
    U8 firstStartPressed;
    U32 minVSyncCnt;
    U8 dontShowPadMessageDuringLoadingOrCutScene;
    U8 autoSaveFeature;
};

#endif
