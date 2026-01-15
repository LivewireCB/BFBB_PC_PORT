#include "iSystem.h"

#include <types.h>

#include <rwcore.h>

#include "xDebug.h"
#include "xFX.h"
#include "xMath.h"
#include "xSnd.h"
#include "xShadow.h"
#include "xPad.h"
#include "xMemMgr.h"
#include "xstransvc.h"

#include "iSystem.h"
#include "iFile.h"
#include "iTime.h"
#include "iTRC.h"

#include <SDL.h>
#include <SDL_system.h>
#include <SDL_video.h>

#include <rwcore.h>
#include <rpworld.h>
#include <rpcollis.h>
#include <rpskin.h>
#include <rphanim.h>
#include <rpmatfx.h>
#include <rpusrdat.h>
#include <rpptank.h>

#include <stdio.h>
#include <stdlib.h>

#define RES_ARENA_SIZE 0x60000

extern U32 mem_base_alloc;
extern U32 add;
extern U32 size;
extern S32 gEmergencyMemLevel;
//extern OSHeapHandle the_heap;
extern void* bad_val;
extern void* MemoryFunctions[4];
extern U16 last_error;
//extern OSContext* last_context;

#define RES_ARENA_SIZE 0x60000

RwVideoMode sVideoMode;

static SDL_Window* window;
static U32 shouldQuit = 0;

static RwEngineOpenParams openParams;

static RwDebugHandler oldDebugHandler;

static U32 RenderWareInit();
static void RenderWareExit();


static void psDebugMessageHandler(RwDebugType type, const RwChar* str)
{
    printf("%s\n", str);

    if (oldDebugHandler) {
        oldDebugHandler(type, str);
    }
}

void iVSync() WIP
{
    SDL_Delay(1000 / VBLANKS_PER_SEC);
}

static void TRCInit() WIP
{
}


// TODO: reimplement commented out functions
void iSystemInit(U32 options)
{
    shouldQuit = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL! SDL Error: %s\n", SDL_GetError());
        exit(-1);
    }

    window = SDL_CreateWindow(GAME_NAME, FB_XRES, FB_YRES, 0);
    if (!window) {
        fprintf(stderr, "Failed to create SDL window! SDL Error: %s\n", SDL_GetError());
        exit(-1);
    }


    // TODO: MAKE SURE THIS IS EXACTLY EQUIVALENT TO THE OLD REPOS VERSION.
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    openParams.displayID = (void*)hwnd;

    xDebugInit();
    xMemInit();
    iFileInit();
    iTimeInit();
    xPadInit();
    //xSndInit();
    TRCInit();
    RenderWareInit();
    xMathInit();
    xMath3Init();
}

void iSystemExit()
{
    xDebugExit();
    xMathExit();
    RenderWareExit();
    xSndExit();
    xPadKill();
    iFileExit();
    iTimeExit();
    xMemExit();

    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("(With apologies to Jim Morrison) This the end, my only friend, The End.");
    exit(0);
}

static U32 RWAttachPlugins()
{
    if (!RpWorldPluginAttach()) return 1;
    if (!RpCollisionPluginAttach()) return 1;
    if (!RpSkinPluginAttach()) return 1;
    if (!RpHAnimPluginAttach()) return 1;
    if (!RpMatFXPluginAttach()) return 1;
    if (!RpUserDataPluginAttach()) return 1;
    if (!RpPTankPluginAttach()) return 1;

    return 0;
}

static RwTexture* TextureRead(const char* name, const char* maskName);

static U32 RenderWareInit()
{
    if (!RwEngineInit(NULL, 0, RES_ARENA_SIZE)) {
        return 1;
    }

    RwResourcesSetArenaSize(RES_ARENA_SIZE);

#ifdef RWDEBUG
    oldDebugHandler = RwDebugSetHandler(psDebugMessageHandler);
    RwDebugSendMessage(rwDEBUGMESSAGE, GAME_NAME, "Debugging Initialized");
#endif

    if (RWAttachPlugins()) return 1;

    if (!RwEngineOpen(&openParams)) {
        RwEngineTerm();
        return 1;
    }

    RwEngineGetVideoModeInfo(&sVideoMode, RwEngineGetCurrentVideoMode());

    if (!RwEngineStart()) {
        RwEngineClose();
        RwEngineTerm();
        return 1;
    }

    RwTextureSetReadCallBack(TextureRead);

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);

    xShadowInit();
    xFXInit();

    RwTextureSetMipmapping(TRUE);
    RwTextureSetAutoMipmapping(TRUE);

    return 0;
}

static void RenderWareExit()
{
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
}

static RwTexture* TextureRead(const RwChar* name, const RwChar* maskName)
{
    char tmpname[256];
    S32 npWidth = 0;
    S32 npHeight = 0;
    S32 npDepth = 0;
    S32 npFormat = 0;
    RwImage* img = NULL;
    RwRaster* rast = NULL;
    RwTexture* result;
#ifdef GAMECUBE
    RwGameCubeRasterExtension* ext = NULL;
#endif
    U32 assetid;
    U32 tmpsize;

    sprintf(tmpname, "%s.rw3", name);
    assetid = xStrHash(tmpname);
    result = (RwTexture*)xSTFindAsset(assetid, &tmpsize);

#ifdef GAMECUBE
    if (result && result->raster && result->raster->depth < 8) {
        ext = RwGameCubeRasterGetExtension(result->raster);
        if (!ext || ext->format != 14) {
            result = NULL;
        }
    }
#endif

    if (result) {
        strcpy(result->name, name);
        strcpy(result->mask, maskName);
    }

    return result;
}

void iSystemPollEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            shouldQuit = 1;
#if 1
            exit(0);
#endif
            break;
        }
    }
}

U32 iSystemShouldQuit()
{
    return shouldQuit;
}

U32 iSystemIsFullScreen()
{
    return 0;
}

// TODO: The functions below are all from the actual decomp. The chances of these being reused arent great, but ill leave them here just in case

//void** psGetMemoryFunctions()
//{
//    return MemoryFunctions;
//}

//U16 my_dsc(U16 dsc)
//{
//    return dsc;
//}
//
//void FloatingPointErrorHandler(U16 last, OSContext* ctxt, U64 unk1, U64 unk2)
//{
//    U32 uVar2;
//    uVar2 = (ctxt->fpscr) & 0xf8 << 0x16 | 0x1f80700;
//    if ((uVar2 & 0x20000000) != 0)
//    {
//        OSReport("FPE: Invalid operation: ");
//
//        if ((uVar2 & 0x1000000) != 0)
//        {
//            OSReport("SNaN\n");
//        }
//        if ((uVar2 & 0x800000) != 0)
//        {
//            OSReport("Infinity - Infinity\n");
//        }
//        if ((uVar2 & 0x400000) != 0)
//        {
//            OSReport("Infinity / Infinity\n");
//        }
//        if ((uVar2 & 0x200000) != 0)
//        {
//            OSReport("0 / 0\n");
//        }
//        if ((uVar2 & 0x100000) != 0)
//        {
//            OSReport("Infinity * 0\n");
//        }
//        if ((uVar2 & 0x80000) != 0)
//        {
//            OSReport("Invalid compare\n");
//        }
//        if ((uVar2 & 0x400) != 0)
//        {
//            OSReport("Software request\n");
//        }
//        if ((uVar2 & 0x200) != 0)
//        {
//            OSReport("Invalid square root\n");
//        }
//        if ((uVar2 & 0x100) != 0)
//        {
//            OSReport("Invalid integer convert\n");
//        }
//    }
//    if ((uVar2 & 0x10000000) != 0)
//    {
//        OSReport("FPE: Overflow\n");
//    }
//    if ((uVar2 & 0x8000000) != 0)
//    {
//        OSReport("FPE: Underflow\n");
//    }
//    if ((uVar2 & 0x4000000) != 0)
//    {
//        OSReport("FPE: Zero division\n");
//    }
//    if ((uVar2 & 0x2000000) != 0)
//    {
//        OSReport("FPE: Inexact result\n");
//    }
//    ctxt->srr0 = ctxt->srr0 + 4;
//}
//
//void MemoryProtectionErrorHandler(U16 last, OSContext* ctx, U64 unk1, U64 unk2)
//{
//    last_error = last;
//    last_context = ctx;
//    if (ctx->fpscr)
//    {
//        null_func();
//    }
//}
//
//// FIXME: Define a bunch of functions :)
//void TRCInit()
//{
//    iTRCDisk::Init();
//    // iTRCDisk::SetPadStopRumblingFunction(iPadStopRumble);
//    // iTRCDisk::SetSndSuspendFunction(iSndSuspend);
//    // iTRCDisk::SetSndResumeFunction(iSndResume);
//    // iTRCDisk::SetSndKillFunction(iSndDIEDIEDIE);
//    // iTRCDisk::SetMovieSuspendFunction(iFMV::Suspend);
//    // iTRCDisk::SetMovieResumeFunction(iFMV::Resume);
//    // ResetButton::SetSndKillFunction(iSndDIEDIEDIE);
//}
//
//S32 RenderWareExit()
//{
//    RwEngineStop();
//    RwEngineClose();
//    return RwEngineTerm();
//}
//
//void iSystemExit()
//{
//    xDebugExit();
//    xMathExit();
//    RenderWareExit();
//    xSndExit();
//    xPadKill();
//    iFileExit();
//    iTimeExit();
//    xMemExit();
//    OSPanic("iSystem.cpp", 0x21d,
//            "(With apologies to Jim Morrison) This the end, my only friend, The End.");
//}
//
//void null_func()
//{
//    mem_base_alloc += 4;
//}
//
//extern "C" {
//void mem_null(U32 param_1, U32 param_2)
//{
//    add = param_1;
//    size = param_2;
//}
//
//void* malloc(U32 __size)
//{
//    if ((S32)__size <= 0)
//    {
//        return NULL;
//    }
//
//    void* result = OSAllocFromHeap(the_heap, __size);
//
//    if (result == NULL)
//    {
//        null_func();
//    }
//
//    return result;
//}
//
//void free(void* __ptr)
//{
//    if (__ptr != NULL)
//    {
//        OSFreeToHeap(the_heap, __ptr);
//    }
//}
//}
//
//void _rwDolphinHeapFree(void* __ptr)
//{
//    if (__ptr == bad_val)
//    {
//        mem_null(0, 0);
//    }
//    if (__ptr != NULL)
//    {
//        if (*(S32*)((S32)__ptr - 4) == 0xDEADBEEF)
//        {
//            free((void*)((S32)__ptr - 32));
//        }
//        else
//        {
//            null_func();
//            if (gEmergencyMemLevel != 0)
//            {
//                xMemPopBase(gEmergencyMemLevel);
//                gEmergencyMemLevel = 0;
//            }
//        }
//    }
//}
//
//S32 iGetMinute()
//{
//    OSTime ticks = OSGetTime();
//    OSCalendarTime td;
//    OSTicksToCalendarTime(ticks, &td);
//    return td.min;
//}
//
//S32 iGetHour()
//{
//    OSTime ticks = OSGetTime();
//    OSCalendarTime td;
//    OSTicksToCalendarTime(ticks, &td);
//    return td.hour;
//}
//
//S32 iGetDay()
//{
//    OSTime ticks = OSGetTime();
//    OSCalendarTime td;
//    OSTicksToCalendarTime(ticks, &td);
//    return td.mday;
//}
//
//S32 iGetMonth()
//{
//    OSTime ticks = OSGetTime();
//    OSCalendarTime td;
//    OSTicksToCalendarTime(ticks, &td);
//    return td.mon + 1;
//}
//
// //Template for future use. TODO
//char* iGetCurrFormattedDate(char* input)
//{
//    return NULL;
//}
//
// //WIP.
//char* iGetCurrFormattedTime(char* input)
//{
//    OSTime ticks = OSGetTime();
//    OSCalendarTime td;
//    OSTicksToCalendarTime(ticks, &td);
//    bool pm = false;
//    // STUFF.
//    char* ret = input;
//    // STUFF.
//    if (pm)
//    {
//        ret[8] = 'P';
//        ret[9] = '.';
//        ret[10] = 'M';
//        ret[11] = '.';
//    }
//    else
//    {
//        ret[8] = 'A';
//        ret[9] = '.';
//        ret[10] = 'M';
//        ret[11] = '.';
//    }
//    ret[12] = '\0';
//    return ret + (0xd - (S32)input);
//}
