#include "xModelBucket.h"

#include "iCamera.h"

#include <types.h>
#include <stdlib.h>

WIP
// check that these are supposed to be statics

 static RpWorld* sBucketDummyWorld;
 static RwCamera* sBucketDummyCamera;
 static S32 xModelBucketEnabled;
 static S32 sBucketCurr;
 static S32 sBucketCount;
 static xModelBucket* sBucketList;
 static S32 sBucketClipCullCurr;
 static S32 sBucketClipCullCount;
 static xModelBucket** sBucketClipCullList;
 static S32 sAlphaCurr;
 static S32 sAlphaStart;
 static S32 sAlphaCount;
 static xModelAlphaBucket* sAlphaList;
 static F32 xModelBucket_f675; // 100000.0f
 static F32 xModelBucket_f676; // -100000.0f

//S32 CmpAlphaBucket(const void* _a, const void* _b)
//{
//    xModelAlphaBucket* a = (xModelAlphaBucket*)_a;
//    xModelAlphaBucket* b = (xModelAlphaBucket*)_b;
//    if (a->Layer > b->Layer)
//    {
//        return 1;
//    }
//    if (a->Layer < b->Layer)
//    {
//        return -1;
//    }
//    if (a->SortValue < b->SortValue)
//    {
//        return 1;
//    }
//    if (a->SortValue > b->SortValue)
//    {
//        return -1;
//    }
//    return 0;
//}
//
//void xModelBucket_PreCountReset()
//{
//    sBucketCurr = 0;
//    sBucketCount = 0;
//    sBucketList = NULL;
//    sBucketClipCullCurr = 0;
//    sBucketClipCullCount = 0;
//    sBucketClipCullList = NULL;
//    sAlphaCurr = 0;
//    sAlphaCount = 0;
//    sAlphaList = NULL;
//}
//
//void xModelBucket_PreCountAlloc(S32 maxAlphaModels)
//{
//    sAlphaCount = maxAlphaModels;
//    sBucketList = (xModelBucket*)xMemAllocSize(sBucketCount * sizeof(xModelBucket));
//    sBucketClipCullList =
//        (xModelBucket**)xMemAllocSize(sBucketClipCullCount * sizeof(xModelBucket*));
//    sAlphaList = (xModelAlphaBucket*)xMemAllocSize(sAlphaCount * sizeof(xModelAlphaBucket));
//    RwBBox boundingBox;
//    boundingBox.sup.x = xModelBucket_f675;
//    boundingBox.sup.y = xModelBucket_f675;
//    boundingBox.sup.z = xModelBucket_f675;
//    boundingBox.inf.x = xModelBucket_f676;
//    boundingBox.inf.y = xModelBucket_f676;
//    boundingBox.inf.z = xModelBucket_f676;
//    sBucketDummyWorld = RpWorldCreate(&boundingBox);
//    sBucketDummyCamera = iCameraCreate(0, 0, 0);
//    RpWorldAddCamera(sBucketDummyWorld, sBucketDummyCamera);
//}
//
//void xModelBucket_Init()
//{
//    if (sBucketDummyCamera != NULL)
//    {
//        RpWorldRemoveCamera(sBucketDummyWorld, sBucketDummyCamera);
//        iCameraDestroy(sBucketDummyCamera);
//        RpWorldDestroy(sBucketDummyWorld);
//        sBucketDummyCamera = NULL;
//        sBucketDummyWorld = NULL;
//    }
//}
//
//void xModelBucket_Begin()
//{
//    for (S32 i = 0; i < sBucketCount; i++)
//    {
//        sBucketList[i].List = NULL;
//    }
//    xModelBucketEnabled = 1;
//}
//
//// Scheduling!
//void xModelBucket_RenderAlphaBegin()
//{
//    sAlphaStart = 0;
//    xModelBucketEnabled = 0;
//    if (sAlphaCurr)
//    {
//        qsort(sAlphaList, sAlphaCurr, sizeof(xModelAlphaBucket), CmpAlphaBucket);
//    }
//}

xModelBucket** xModelBucket_GetBuckets(RpAtomic* data)
{
    for (S32 i = 0; i < sBucketClipCullCount; i += 2) {
        if (sBucketClipCullList[i]->OriginalData == data) {
            return &sBucketClipCullList[i];
        }
    }
    return NULL;
}
