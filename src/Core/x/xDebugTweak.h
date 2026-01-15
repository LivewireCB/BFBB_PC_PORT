#pragma once

#include "xString.h"
#include "xVec3.h"


#if defined(DEBUG) || defined(RELEASE)
void debug_mode_tweak()
{
}
#endif

inline void xDebugRemoveTweak(const char* name)
{
}

inline void xDebugAddSelectTweak(const char* name, U32* v, const char** labels, const U32* values,
                                 U32 labels_size, const tweak_callback* cb, void* context,
                                 U32 flags)
{
}

inline void xDebugAddFlagTweak(const char* name, U32* v, U32 mask, const tweak_callback* cb,
                               void* context, U32 flags)
{
}

inline void xDebugAddTweak(const char* name, S16* v, S16 vmin, S16 vmax, const tweak_callback* cb,
                           void* context, U32 flags)
{
}

inline void xDebugAddTweak(const char* name, U32* v, U32 vmin, U32 vmax, const tweak_callback* cb,
                           void* context, U32 flags)
{
}

inline void xDebugAddTweak(const char* name, U8* v, U8 vmin, U8 vmax, const tweak_callback* cb,
                           void* context, U32 flags)
{
}

inline void xDebugAddTweak(const char* name, F32* v, F32 vmin, F32 vmax, const tweak_callback* cb,
                           void* context, U32 flags)
{
}

inline void xDebugAddTweak(const char* name, xVec3* v, const tweak_callback* cb,
                           void* context, U32 flags)
{
}

inline void xDebugAddTweak(const char* name, const char* message, const tweak_callback* cb,
                           void* context, U32 flags)
{
}