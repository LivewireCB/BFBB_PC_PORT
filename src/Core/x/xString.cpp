#include "xString.h"
#include "xMath.h"

#include <types.h>

U32 xStrHash(const char* str)
{
    U32 hash = 0;
    U32 i;

    while (i = *str, i != NULL)
    {
        hash = (i - (i & (S32)i >> 1 & 0x20) & 0xff) + hash * 0x83;
        str++;
    }

    return hash;
}

U32 xStrHash(const char* str, size_t size)
{
    U32 hash = 0;
    U32 i = 0;
    U32 c;

    while (i < size && (c = *str, c != NULL))
    {
        i++;
        str++;
        hash = (c - (c & (S32)c >> 1 & 0x20) & 0xff) + hash * 0x83;
    }

    return hash;
}

U32 xStrHashCat(U32 prefix, const char* str)
{
    U32 hash;
    U32 i;

    while (i = *str, i != NULL)
    {
        str++;
        hash = (i - (i & (S32)i >> 1 & 0x20) & 0xff) + hash * 0x83;
    }

    return hash;
}

char* xStrupr(char* string)
{
    char* p = string;

    while (*p != '\0')
    {
        *p = (*p >= 'a' && *p <= 'z' ? *p - 32 : *p);

        p++;
    }

    return string;
}

char* xStrTok(char* string, const char* control, char** nextoken)
{
    U8* str;
    const U8* ctrl = (const U8*)control;
    U8 map[32];
    S32 count = 0;

    while (count < 32) {
        map[count] = 0;
        count++;
    }

    do {
        map[*ctrl >> 3] |= (U8)(1 << (*ctrl & 0x7));
    } while (*ctrl++);

    if (string) str = (U8*)string;
    else str = (U8*)*nextoken;

    while ((map[*str >> 3] & (1 << (*str & 0x7))) && *str) {
        str++;
    }
    string = (char*)str;

    while (*str) {
        if (map[*str >> 3] & (1 << (*str & 0x7))) {
            *str = 0;
            str++;
            break;
        }
        str++;
    }
    *nextoken = (char*)str;

    return (string == (char*)str) ? NULL : string;
}

char* xStrTokBuffer(const char* string, const char* control, void* buffer)
{
    char** nextoken = (char**)buffer;
    char* dest = (char*)(nextoken + 1);
    const U8* str;
    const U8* ctrl = (const U8*)control;
    U8 map[32];
    S32 count = 0;

    while (count < 32) {
        map[count] = 0;
        count++;
    }

    do {
        map[*ctrl >> 3] |= (U8)(1 << (*ctrl & 0x7));
    } while (*ctrl++);

    if (string) str = (const U8*)string;
    else str = (const U8*)*nextoken;

    while ((map[*str >> 3] & (1 << (*str & 0x7))) && *str) {
        str++;
    }
    string = (const char*)str;

    while (*str) {
        if (map[*str >> 3] & (1 << (*str & 0x7))) {
            str++;
            break;
        }
        *dest = *str;
        dest++;
        str++;
    }
    *dest = 0;
    *nextoken = (char*)str;

    return (string == (const char*)str) ? NULL : (char*)(nextoken + 1);
}

S32 xStricmp(const char* string1, const char* string2)
{
    S32 result = 0;

    while (xtoupper(*string1) == xtoupper(*string2) && result == 0) {
        if (!*string1 || !*string2) {
            result = 1;
        }
        else {
            string1++;
            string2++;
        }
    }

    result = 0;
    if (*string1 != *string2) {
        result = 1;
        if (xtoupper(*string1) < xtoupper(*string2)) result = -1;
    }

    return result;
}

S32 xStrParseFloatList(F32* dest, const char* strbuf, S32 max)
{
    char* str;
    if (!(str = (char*)strbuf)) return 0;

    S32 index = 0;
    S32 digits;
    S32 negate;
    char tmpc;
    char* numstart;
    while (*str && index < max) {
        while (*str == '\t' ||
            *str == ' ' ||
            *str == '+' ||
            *str == '[' ||
            *str == ']' ||
            *str == '{' ||
            *str == '}' ||
            *str == '(' ||
            *str == ')' ||
            *str == ',' ||
            *str == ':' ||
            *str == ';') {
            str++;
        }

        if (!*str) return index;

        if (*str == '-') {
            negate = 1;
            str++;
            while (*str == '\t' || *str == ' ') str++;
        }
        else {
            negate = 0;
        }

        numstart = str;
        digits = 0;
        while ((*str >= '0' && *str <= '9') ||
            *str == '.' ||
            *str == 'e' ||
            *str == 'E' ||
            *str == 'f') {
            if (*str >= '0' && *str <= '9') digits++;
            str++;
        }
        if (digits == 0) return index;

        tmpc = *str;
        *str = '\0';

        dest[index] = xatof(numstart);
        if (negate) {
            dest[index] = -dest[index];
        }

        *str = tmpc;
        index++;
    }

    return index;
}

namespace {
    inline S32 tolower(S32 c)
    {
        return c | ((c >> 1) & 0x20);
    }

    inline S32 tolower(char c)
    {
        return tolower((S32)c);
    }
}

S32 imemcmp(const void* d1, const void* d2, size_t size) NONMATCH("https://decomp.me/scratch/E56sp")
{
    const char* s1 = (const char*)d1;
    const char* s2 = (const char*)d2;
    for (size_t i = 0; i < size; i++) {
        S32 c1 = tolower(*s1);
        S32 c2 = tolower(*s2);
        if (c1 != c2) return c1 - c2;
        s2++;
        s1++;
    }
    return 0;
}

S32 icompare(const substr& s1, const substr& s2)
{
    U32 len = MIN(s1.size, s2.size);
    S32 result = imemcmp(s1.text, s2.text, len);
    switch (result)
    {
        case 0:
        if (s1.size == s2.size)
        {
            result = 0;
        }
        else
        {
            result = 1;
            if (s1.size < s2.size)
            {
                result = -1;
            }
        }
        break;
    }
    return result;
}

U32 atox(const substr& s, size_t& read_size) NONMATCH("https://decomp.me/scratch/O0o7q")
{
    const char* p = s.text;
    size_t size = s.size;
    if (!p) return 0;

    U32 total = 0;
    if (size > 8) size = 8;

    read_size = 0;
    while (read_size < size) {
        char c = *p;
        U32 v;
        if (c >= '0' && c <= '9') {
            v = c - 48;
        }
        else if (c >= 'a' && c <= 'f') {
            v = c - 87;
        }
        else if (c >= 'A' && c <= 'F') {
            v = c - 55;
        }
        else {
            break;
        }
        total = (total << 4) + v;
        p++;
        read_size++;
    }

    return total;
}

// Why... WHY???????
const char* find_char(const substr& s, const substr& cs) NONMATCH("https://decomp.me/scratch/e3LMf")
{
    if (!s.text || !cs.text) return NULL;
    const char* p = s.text;
    const char* d = cs.text;
    switch (cs.size) {
    case 1:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d) return p;
        }
        break;
    case 2:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1)) return p;
        }
        break;
    case 3:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2)) return p;
        }
        break;
    case 4:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3)) return p;
        }
        break;
    case 5:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4)) return p;
        }
        break;
    case 6:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4) ||
                c == *(d + 5)) return p;
        }
        break;
    case 7:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4) ||
                c == *(d + 5) ||
                c == *(d + 6)) return p;
        }
        break;
    case 8:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4) ||
                c == *(d + 5) ||
                c == *(d + 6) ||
                c == *(d + 7)) return p;
        }
        break;
    case 9:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4) ||
                c == *(d + 5) ||
                c == *(d + 6) ||
                c == *(d + 7) ||
                c == *(d + 8)) return p;
        }
        break;
    case 10:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4) ||
                c == *(d + 5) ||
                c == *(d + 6) ||
                c == *(d + 7) ||
                c == *(d + 8) ||
                c == *(d + 9)) return p;
        }
        break;
    case 11:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            char c = *p;
            if (c == *d ||
                c == *(d + 1) ||
                c == *(d + 2) ||
                c == *(d + 3) ||
                c == *(d + 4) ||
                c == *(d + 5) ||
                c == *(d + 6) ||
                c == *(d + 7) ||
                c == *(d + 8) ||
                c == *(d + 9) ||
                c == *(d + 10)) return p;
        }
        break;
    default:
        for (S32 i = s.size; i > 0 && *p; i--, p++) {
            const char* s = cs.text;
            while (*s) {
                if (*p == *s) return p;
                s++;
            }
        }
        break;
    }

    return NULL;
}
