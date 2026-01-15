#ifndef XMATHINLINES_H
#define XMATHINLINES_H

#include <types.h>
#include <cmath>

inline F32 SQ(F32 x) { return x * x; }

inline F32 xsqrt(F32 x)
{
    return std::sqrtf(x);
}

inline F32 xexp(F32 x)
{
    return std::expf(x);
}

inline F32 xpow(F32 x, F32 y)
{
    return std::powf(x, y);
}

inline F32 xfmod(F32 x, F32 y)
{
    return std::fmodf(x, y);
}

inline F32 xacos(F32 x)
{
    return std::acosf(x);
}

inline F32 xasin(F32 x)
{
    return std::asinf(x);
}

F32 xAngleClampFast(F32 a);

inline F32 xatan2(F32 y, F32 x)
{
    return xAngleClampFast(std::atan2f(y, x));
}

inline void xsqrtfast(F32& o, F32 fVal)
{
    o = std::sqrtf(fVal);
}

inline U8 LERP(F32 x, U8 y, U8 z)
{
    return (U8)(x * (z - y)) + y;
}

inline F32 LERP(F32 x, F32 y, F32 z)
{
    return (x * (z - y)) + y;
}

inline F32 EASE(F32 rhs)
{
    return rhs * ((rhs * 3.0f) - (rhs * 2.0f) * rhs);
}

inline F32 SMOOTH(F32 x, F32 y, F32 z)
{
    return (z - y) * EASE(x) + y;
}

#endif
