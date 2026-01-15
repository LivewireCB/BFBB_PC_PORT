#ifndef XTEXTASSET_H
#define XTESTASSET_H

#include <types.h>

struct xTextAsset
{
    U32 len;
};

#define xTextAssetGetText(t) ((char*)((xTextAsset*)(t) + 1))
#define xTextGetString(asset) ((const char*)((xTextAsset*)(asset) + 1))

#endif
