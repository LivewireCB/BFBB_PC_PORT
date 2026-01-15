#include "iMath.h"

#include <cmath>
#include <stddef.h>

F32 isin(F32 x)
{
    return std::sinf(x);
}

// TODO: investigate these. I commented them out to get a build, and they dont seem important at this moment cus seils repo doesnt have them

//#ifndef INLINE
//float std::sinf(float x)
//{
//    return (float)sin((double)x);
//}
//#endif

F32 icos(F32 x)
{
    return std::cosf(x);
}

//#ifndef INLINE
//float std::cosf(float x)
//{
//    return (float)cos((double)x);
//}
//#endif

F32 itan(F32 x)
{
    return std::tanf(x);
}

//#ifndef INLINE
//float std::tanf(float x)
//{
//    return (float)tan((double)x);
//}
//#endif
