#ifndef XDEBUG_H
#define XDEBUG_H

#include "xFont.h"

#include <types.h>

extern U32 gFrameCount;

struct uint_data
{
    U32 value_def;
    U32 value_min;
    U32 value_max;
};

struct float_data
{
    F32 value_def;
    F32 value_min;
    F32 value_max;
};

struct bool_data
{
    U8 value_def;
};

struct select_data
{
    U32 value_def;
    U32 labels_size;
    char** labels;
    void* values;
};

struct flag_data
{
    U32 value_def;
    U32 mask;
};

struct raw_data
{
    U8 pad[16];
};

struct int_data
{
    S32 value_def;
    S32 value_min;
    S32 value_max;
};

struct tweak_callback;
struct tweak_info
{
    substr name;
    void* value;
    tweak_callback* cb;
    void* context;
    U8 type;
    U8 value_size;
    U16 flags;
    union
    {
        int_data int_context;
        uint_data uint_context;
        float_data float_context;
        bool_data bool_context;
        select_data select_context;
        flag_data flag_context;
        raw_data all_context;
    };
};

struct tweak_callback
{
    void (*on_change)(tweak_info&);
    void (*on_select)(tweak_info&);
    void (*on_unselect)(tweak_info&);
    void (*on_start_edit)(tweak_info&);
    void (*on_stop_edit)(tweak_info&);
    void (*on_expand)(tweak_info&);
    void (*on_collapse)(tweak_info&);
    void (*on_update)(tweak_info&);
    void (*convert_mem_to_tweak)(tweak_info&, void*);
    void (*convert_tweak_to_mem)(tweak_info&, void*);
};

void xprintf(const char* msg, ...);
S32 xDebugModeAdd(const char* mode, void(*debugFunc)());
void xDebugInit();
void xDebugUpdate();
void xDebugExit();
void xDebugTimestampScreen();

void xDebugAddTweak(const char*, F32*, F32, F32, const tweak_callback*, void*, U32);
void xDebugRemoveTweak(const char*);
void xDebugUpdate();


// PC PORT REPO STUFF BELOW

#include "iDebug.h"
#include <assert.h>

enum en_VERBOSE_MSGLEVEL
{
    DBML_NONE,
    DBML_RELDISP,
    DBML_DISP,
    DBML_USER,
    DBML_ERR,
    DBML_TIME,
    DBML_WARN,
    DBML_VALID,
    DBML_INFO,
    DBML_DBG,
    DBML_TEST,
    DBML_VDBG,
    DBML_SPEW
};

struct xSB
{
    char* buf;
    U32 max;
    char* cur;
    char* disp;
};

typedef void(*xDebugMsgNotifyCallback)(en_VERBOSE_MSGLEVEL, char*);
typedef void(*xDebugModeCallback)();

#if defined(DEBUG) || defined(RELEASE)
bool xDebugIDontWantYourXprintsOnMyScreen();
void DBStartup(en_VERBOSE_MSGLEVEL setlvl, char* logfile, char* errfile, xDebugMsgNotifyCallback msgnotify);
void DBShutdown();
S32 DBChkMsgLvl(en_VERBOSE_MSGLEVEL msglvl);
void DBprintf(en_VERBOSE_MSGLEVEL msglvl, const char* fmt, ...);
void xDebugVSync();
void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr);
void xDebug_assert2(const char* fmt, ...);
U32 xDebugBoing();
S32 xDebugModeGet();
void xDebugStackTrace();
void debug_mode_page();
#else
#define DBprintf
#endif

#ifdef DEBUG
void xDebugValidateFailed();
#endif

#ifdef DEBUG
#define _xVALIDATE(expr, exprstr)                                                                            \
MACROSTART                                                                                           \
    if (!(expr)) {                                                                             \
        static int been_here;                                                                  \
        if (!been_here) {                                                                      \
            DBprintf(DBML_VALID, "%s(%d) : (" exprstr ") in '%s'\n", __FILE__, __LINE__, __FUNCTION__);\
            xDebugValidateFailed();                                                            \
            been_here = 1;                                                                     \
        }                                                                                      \
    }                                                                                          \
MACROEND
#define xVALIDATE(expr) _xVALIDATE(expr, #expr)
#define xVALIDATEMSG(expr, msg) _xVALIDATE(expr, msg)

#define xVALIDATEEQ(_l, _r)                                                                            \
MACROSTART                                                                                           \
    static int been_here;                                                                  \
    if (!been_here) {                                                                      \
        size_t l = (_l);                                                                           \
        size_t r = (_r);                                                                           \
        if (l != r) {                                                                              \
            DBprintf(DBML_VALID, "%s(%d) : (" #_l "==" #_r ") in '%s'\n", __FILE__, __LINE__, __FUNCTION__);\
            xDebugValidateFailed();                                                            \
            been_here = 1;                                                                     \
        }                                                                                    \
    }                                                                                      \
MACROEND

#define xVALIDATEFAIL() _xVALIDATE(0, "*always*")
#else
#define xVALIDATE(expr)
#define xVALIDATEMSG(expr, msg)
#define xVALIDATEEQ(_l, _r)
#define xVALIDATEFAIL()
#endif

#if defined(DEBUG) || defined(RELEASE)
#define _xASSERTFMT(expr, exprstr, msg, ...)                                                       \
MACROSTART                                                                                           \
    if (!(expr)) {                                                                             \
        xDebug_assert2_info(__FUNCTION__, __FILE__, __LINE__, exprstr);                        \
        xDebug_assert2(msg, __VA_ARGS__);                                                      \
        xDebugStackTrace();                                                                    \
        if (xDebugBoing()) iDebugBreak();                                                      \
    }                                                                                          \
MACROEND

#define _xASSERTMSG(expr, exprstr, msg)                                                          \
MACROSTART                                                                                           \
    if (!(expr)) {                                                                             \
        xDebug_assert2_info(__FUNCTION__, __FILE__, __LINE__, exprstr);                        \
        xDebug_assert2(msg);                                                                   \
        xDebugStackTrace();                                                                    \
        if (xDebugBoing()) iDebugBreak();                                                      \
    }                                                                                          \
MACROEND

#define xASSERTFMT(expr, msg, ...) _xASSERTFMT(expr, #expr, msg, __VA_ARGS__)
#define xASSERTMSG(expr, msg) xASSERTFMT(expr, "%s", msg)
#define xASSERTMSG2(expr, msg) _xASSERTMSG(expr, #expr, msg)
#define xASSERT(expr) xASSERTFMT(expr, "%s", #expr)

#define xASSERTEQ(l, r)                                                                            \
MACROSTART                                                                                           \
    size_t il = (l);                                                                           \
    size_t ir = (r);                                                                           \
    xASSERTMSG(il == ir, #l "==" #r);                                                    \
MACROEND

#define xASSERTFAILFMT(msg, ...) _xASSERTFMT(0, "*always*", msg, __VA_ARGS__)
#define xASSERTFAILMSG(msg) _xASSERTFMT(0, "*always*", "%s", msg)
#define xASSERTFAIL() xASSERTFAILMSG("*always*")
#else
#define xASSERTFMT(expr, msg, ...)
#define xASSERTMSG(expr, msg)
#define xASSERTMSG2(expr, msg)
#define xASSERT(expr)
#define xASSERTEQ(l, r)
#define xASSERTFAILFMT(msg, ...)
#define xASSERTFAILMSG(msg)
#define xASSERTFAIL()
#endif

inline const char* xbtoa(U32 b)
{
    return b ? "true " : "false";
}


#endif
