#include "iSnd.h"

#include <types.h>

#include "intrin.h"

#include "xCutscene.h"
#include "xSnd.h"
#include "xMath.h"

vinfo voices[58];
char soundInited;
S32 SoundFlags;
extern F32 _1262;
extern F32 _1263;
volatile S32 fc;


// TODO: investigate return type could be bool or S32?
S32 iSndIsPlaying(U32 assetID, U32 parid) WIP
{
    return false;
}

// TODO: Implement all of these; or dont, i dont care about sound

void arq_callback(long)
{
    if (!soundInited)
    {
        return;
    }
    SoundFlags = 0;
}

void iSndExit() WIP
{
    soundInited = 0;
    //AXQuit(); 
}

//not sure where this type is from.
void iSndSetEnvironmentalEffect(isound_effect)
{
    return;
}

void iSndInitSceneLoaded()
{
}

//U32 iVolFromX(F32 param1)
//{
//    float f = MAX(param1, 1e-20f);
//
//    S32 i = 43.43f * xlog(f);
//    S32 comp = MIN(i, 0);
//
//    if (comp < -0x388)
//    {
//        return -0x388;
//    }
//    else
//    {
//        return MIN(i, 0);
//    }
//}
//
//void iSndVolUpdate(xSndVoiceInfo* info, vinfo* vinfo)
//{
//    MIXUnMute(vinfo->voice);
//    xSndInternalUpdateVoicePos(info);
//    if ((info->flags & 8) != 0)
//    {
//        iSndCalcVol3d(info, vinfo);
//    }
//    else
//    {
//        iSndCalcVol(info, vinfo);
//    }
//}
//
//void iSndUpdateSounds()
//{
//    if (!soundInited)
//    {
//        return;
//    }
//
//    for (int i = 0; i < 58; i++)
//    {
//        if (voices[i].voice != NULL)
//        {
//            iSndVolUpdate(&gSnd.voice[i + 6], &voices[i]);
//        }
//    }
//}

void iSndStartStereo(U32 id1, U32 id2, F32 pitch)
{
}

void iSndStereo(U32 i) WIP RIMP // removed the os calls
{
    if (i == 0)
    {
        //OSSetSoundMode(0);
        gSnd.stereo = 0;
    }
    else
    {
        //OSSetSoundMode(1);
        gSnd.stereo = 1;
    }
}

//void iSndWaitForDeadSounds()
//{
//    fc = 0;
//    int i = 0;
//    // Can't get 0x8c to get stored in r31
//    while ((i = fc) < 0x8c) //for (int i = 0; (i = fc) < 0x8c; )
//    {
//        i = fc;
//        while (fc < i + 0xe)
//            ;
//        iSndUpdate();
//    }
//}

void iSndSuspendCD(U32)
{
}

//void iSndMessWithEA(sDSPADPCM* param1)
//{
//    if (param1 != NULL)
//    {
//        param1->buffer[5] = SampleToNybbleAddress(param1->buffer[0] - 1);
//    }
//}
//
//U32 SampleToNybbleAddress(U32 sample)
//{
//    U32 a = __mulhwu(0x24924925, sample);
//    U32 b = (sample - a) >> 1;
//
//    a = b + a;
//    b = (a >> 3);
//    a = (a << 1) & 0xfffffff0;
//    a = a + (sample - (b * 0xe)) + 2;
//
//    return a;
//}

void sndloadcb(tag_xFile* tag)
{
    SoundFlags = 0;
}

void iSndSetExternalCallback(void (*func_ptr)(U32))
{
}

//void iSndMyAXFree(_AXVPB** param1)
//{
//    if (param1 != NULL && *param1 != NULL)
//    {
//        AXFreeVoice(*param1);
//        *param1 = NULL;
//    }
//}

void iSndPause(U32 snd, U32 pause) WIP
{
}

void iSndInit() WIP
{
}   

void iSndUpdate() WIP
{
}

void iSndStop(U32 snd) WIP
{
}   

void iSndSetPitch(U32 snd, F32 pitch) WIP
{
}

void iSndSetVol(U32 snd, F32 vol) WIP
{
}

F32 iSndGetVol(U32 snd) WIP
{
    return 0.0f;
}

S32 iSndLoadSounds(void*) WIP
{
    return 0;
}