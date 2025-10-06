#include "iModel.h"

#include <types.h>
#include "zAssetTypes.h"
#include "xModelBucket.h"
#include "iCamera.h"
#include "rpskin.h"
#include "rpmatfx.h"

#define MAX2(a, b) ((a) >= (b) ? (a) : (b))
#define MAX3(a, b, c) (MAX2((a), MAX2((b), (c))))

#define IMODEL_MAX_ATOMICS 256
#define IMODEL_MAX_DIRECTIONAL_LIGHTS 4
#define IMODEL_MAX_MATERIALS 16

RpWorld* instance_world;
RwCamera* instance_camera;

static U32 gLastAtomicCount;
static RpAtomic* gLastAtomicList[IMODEL_MAX_ATOMICS];
static RpLight* sEmptyDirectionalLight[IMODEL_MAX_DIRECTIONAL_LIGHTS];
static RpLight* sEmptyAmbientLight;
static RwRGBA sMaterialColor[IMODEL_MAX_MATERIALS];
static RwTexture* sMaterialTexture[IMODEL_MAX_MATERIALS];
static U8 sMaterialAlpha[IMODEL_MAX_MATERIALS];
static U32 sMaterialIdx;
static U32 sMaterialFlags;
static RpAtomic* sLastMaterial;


static RwFrame* GetChildFrameHierarchy(RwFrame* frame, void* data)
{
    RpHAnimHierarchy* hierarchy = *(RpHAnimHierarchy**)data;
    hierarchy = RpHAnimFrameGetHierarchy(frame);
    if (!hierarchy) {
        RwFrameForAllChildren(frame, GetChildFrameHierarchy, data);
        return frame;
    }
    *(RpHAnimHierarchy**)data = hierarchy;
    return NULL;
}

static RpHAnimHierarchy* GetHierarchy(RpAtomic* imodel)
{
    RpHAnimHierarchy* hierarchy = NULL;
    RwFrame* frame = RpAtomicGetFrame(imodel);
    GetChildFrameHierarchy(frame, &hierarchy);
    return hierarchy;
}

void iModelInit() NONMATCH("https://decomp.me/scratch/rkLET")
{
    RwFrame* frame;
    RwRGBAReal black = { 0, 0, 0, 0 };

    if (sEmptyDirectionalLight[0]) return;

    for (S32 i = 0; i < IMODEL_MAX_DIRECTIONAL_LIGHTS; i++) {
        sEmptyDirectionalLight[i] = RpLightCreate(rpLIGHTDIRECTIONAL);
        RpLightSetColor(sEmptyDirectionalLight[i], &black);
        frame = RwFrameCreate();
        RpLightSetFrame(sEmptyDirectionalLight[i], frame);
    }

    sEmptyAmbientLight = RpLightCreate(rpLIGHTAMBIENT);
    RpLightSetColor(sEmptyAmbientLight, &black);
}

RpAtomic* iModelFileNew(void* buffer, U32 size)
{
    RwMemory rwmem;
    rwmem.start = (RwUInt8*)buffer;
    rwmem.length = size;
    return iModelStreamRead(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmem));
}

void iModelUnload(RpAtomic* userdata)
{
    RpClump* clump = RpAtomicGetClump(userdata);
    RwFrame* frame = RpClumpGetFrame(clump);

    if (frame) {
        RwFrame* root = RwFrameGetRoot(frame);
        if (root) frame = root;
        RwFrameDestroyHierarchy(frame);
        RpClumpSetFrame(clump, NULL);
    }

    if (clump) {
        RpClumpDestroy(clump);
    }
}

static RpAtomic* NextAtomicCallback(RpAtomic* atomic, void* data)
{
    RpAtomic** nextModel = (RpAtomic**)data;
    if (*nextModel == atomic) {
        *nextModel = NULL;
    }
    else if (!*nextModel) {
        *nextModel = atomic;
    }
    return atomic;
}

RpAtomic* iModelCacheAtomic(RpAtomic* model)
{
    return model;
}

void iModelRender(RpAtomic* model, RwMatrix* mat)
{
    RpHAnimHierarchy* pHierarchy = GetHierarchy(model);
    static S32 draw_all = 1;

    RwMatrix* pAnimOldMatrix;
    if (pHierarchy) {
        pAnimOldMatrix = pHierarchy->pMatrixArray;
        pHierarchy->pMatrixArray = mat + 1;
    }

    RwFrame* frame = RpAtomicGetFrame(model);
    frame->ltm = *mat;
    RwMatrixUpdate(&frame->ltm);

    if (iModelHack_DisablePrelight) {
        model->geometry->flags &= ~rpGEOMETRYPRELIT;
    }

    RpAtomicRender(iModelCacheAtomic(model)); // BUG: will call iModelCacheAtomic twice with RW release build

    if (iModelHack_DisablePrelight) {
        if (model->geometry->preLitLum) {
            model->geometry->flags |= rpGEOMETRYPRELIT;
        }
    }

    if (pHierarchy) {
        pHierarchy->pMatrixArray = pAnimOldMatrix;
    }
}

static RpAtomic* FindAndInstanceAtomicCallback(RpAtomic* atomic, void* data) NONMATCH("https://decomp.me/scratch/aAfw8")
{
    RpHAnimHierarchy* pHier = GetHierarchy(atomic);
    RpGeometry* pGeom = RpAtomicGetGeometry(atomic);
    RpSkin* pSkin = RpSkinGeometryGetSkin(pGeom);

    if (pSkin && !pHier) {
        pHier = RpHAnimHierarchyCreate(RpSkinGetNumBones(pSkin), NULL, NULL, rpHANIMHIERARCHYLOCALSPACEMATRICES, rpHANIMSTDKEYFRAMESIZE);
        RwFrame* frame = RpAtomicGetFrame(atomic);
        RpHAnimFrameSetHierarchy(frame, pHier);
    }

    if (pHier && pSkin) {
        RpSkinAtomicSetHAnimHierarchy(atomic, pHier);
    }

    if (pHier) {
        pHier->flags = rpHANIMHIERARCHYLOCALSPACEMATRICES;
    }

    if (gLastAtomicCount < IMODEL_MAX_ATOMICS) {
        gLastAtomicList[gLastAtomicCount] = atomic;
        gLastAtomicCount++;
    }

    RwFrame* root = RwFrameGetRoot(RpAtomicGetFrame(atomic));

    RpMaterialList* matList = &pGeom->matList;
    S32 i, numMaterials = rpMaterialListGetNumMaterials(matList);
    for (i = 0; i < numMaterials; i++) {
        RpMaterial* material = rpMaterialListGetMaterial(matList, i);
        if (material && RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTNULL) {
            RpMatFXAtomicEnableEffects(atomic);
#ifdef D3D8_DRVMODEL_H
            RpAtomicSetPipeline(atomic, RpMatFXGetD3D8Pipeline(rpMATFXD3D8ATOMICPIPELINE));
#else
#pragma message ("WARNING: Unknown RW platform, MatFX pipeline unsupported")
#endif
            if (RpSkinGeometryGetSkin(pGeom)) {
                RpSkinAtomicSetType(atomic, rpSKINTYPEMATFX);
            }
            break;
        }
    }

    if (gLastAtomicCount < IMODEL_MAX_ATOMICS) {
        gLastAtomicList[gLastAtomicCount] = atomic;
        gLastAtomicCount++;
    }

    return atomic;
}

static RpAtomic* iModelStreamRead(RwStream* stream) NONMATCH("https://decomp.me/scratch/v3Rdp") WIP
{
    static S32 num_models = 0;

    if (!stream) return NULL;
    if (!RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL)) {
        RwStreamClose(stream, NULL);
        return NULL;
    }

    RpClump* clump = RpClumpStreamRead(stream);
    RwStreamClose(stream, NULL);
    if (!clump) return NULL;

    RwBBox tmpbbox = { 1000.0f, 1000.0f, 1000.0f, -1000.0f, -1000.0f, -1000.0f };

    instance_world = RpWorldCreate(&tmpbbox);
    instance_camera = iCameraCreate(FB_XRES, FB_YRES, 0);
    RpWorldAddCamera(instance_world, instance_camera);

    gLastAtomicCount = 0;
    RpClumpForAllAtomics(clump, FindAndInstanceAtomicCallback, NULL);

    RpWorldRemoveCamera(instance_world, instance_camera);
    iCameraDestroy(instance_camera);
    RpWorldDestroy(instance_world);

    if (gLastAtomicCount > 1) {
        U32 i, maxIndex = 0;
        F32 maxRadius = -1.0f;
        for (i = 0; i < gLastAtomicCount; i++) {
            if (gLastAtomicList[i]->boundingSphere.radius > maxRadius) {
                maxIndex = i;
                maxRadius = gLastAtomicList[i]->boundingSphere.radius;
            }
        }
        for (i = 0; i < gLastAtomicCount; i++) {
            if (i != maxIndex) {
                F32 testRadius = gLastAtomicList[i]->boundingSphere.radius +
                    xVec3Dist((xVec3*)&gLastAtomicList[i]->boundingSphere.center,
                        (xVec3*)&gLastAtomicList[maxIndex]->boundingSphere.center);
                if (testRadius > maxRadius) {
                    maxRadius = testRadius;
                }
            }
        }
        maxRadius *= 1.05f;
        for (i = 0; i < gLastAtomicCount; i++) {
            if (i != maxIndex) {
                gLastAtomicList[i]->boundingSphere.center = gLastAtomicList[maxIndex]->boundingSphere.center;
            }
            gLastAtomicList[i]->boundingSphere.radius = maxRadius;
            gLastAtomicList[i]->interpolator.flags &= ~rpINTERPOLATORDIRTYSPHERE;
        }
    }

    return gLastAtomicList[0];
}


namespace {
    inline void U8_COLOR_CLAMP(U8& destu8, F32 srcf32) NONMATCH("https://decomp.me/scratch/dl7BD")
    {
        if (srcf32 < 0.0f) srcf32 = 0.0f;
        else if (srcf32 > 255.0f) srcf32 = 255.0f;
        destu8 = (U8)srcf32;
    }
}

static RpMaterial* iModelMaterialMulCB(RpMaterial* material, void* data) NONMATCH("https://decomp.me/scratch/ig80z")
{
    const RwRGBA* rw_col = RpMaterialGetColor(material);
    RwRGBA col = sMaterialColor[sMaterialIdx++] = *rw_col;
    F32 tmp;
    F32* mods = (F32*)data;

    tmp = col.red * mods[0];
    U8_COLOR_CLAMP(col.red, tmp);

    tmp = col.green * mods[1];
    U8_COLOR_CLAMP(col.green, tmp);

    tmp = col.blue * mods[2];
    U8_COLOR_CLAMP(col.blue, tmp);

    RpMaterialSetColor(material, &col);

    return material;
}

void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm) NONMATCH("https://decomp.me/scratch/zDCk9")
{
    RpGeometry* geom = RpAtomicGetGeometry(model);

    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometrySetFlags(geom, RpGeometryGetFlags(geom) | rpGEOMETRYMODULATEMATERIALCOLOR);

    F32 cols[3];
    cols[0] = rm;
    cols[1] = gm;
    cols[2] = bm;

    RpGeometryForAllMaterials(geom, iModelMaterialMulCB, cols);

    sLastMaterial = model;
    sMaterialFlags |= 0x2;
}

static RpMaterial* iModelSetMaterialAlphaCB(RpMaterial* material, void* data)
{
    const RwRGBA* col = RpMaterialGetColor(material);
    sMaterialAlpha[sMaterialIdx++] = col->alpha;

    RwRGBA new_col = *col;
    new_col.alpha = *(U8*)data;

    RpMaterialSetColor(material, &new_col);

    return material;
}

void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha) NONMATCH("https://decomp.me/scratch/Oyo8b")
{
    RpGeometry* geom = RpAtomicGetGeometry(model);

    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometrySetFlags(geom, RpGeometryGetFlags(geom) | rpGEOMETRYMODULATEMATERIALCOLOR);

    sMaterialIdx = 0;

    RpGeometryForAllMaterials(geom, iModelSetMaterialAlphaCB, &alpha);

    sMaterialFlags |= 0x1;
    sLastMaterial = model;
}

static RpMaterial* iModelResetMaterialCB(RpMaterial* material, void* data) NONMATCH("https://decomp.me/scratch/eBpxV")
{
    if ((sMaterialFlags & 0x3) == 0x3) {
        RwRGBA newColor = sMaterialColor[sMaterialIdx];
        newColor.alpha = sMaterialAlpha[sMaterialIdx];
        RpMaterialSetColor(material, &newColor);
    }
    else {
        if (sMaterialFlags & 0x2) {
            RwRGBA newColor = sMaterialColor[sMaterialIdx];
            newColor.alpha = RpMaterialGetColor(material)->alpha;
            RpMaterialSetColor(material, &newColor);
        }
        if (sMaterialFlags & 0x1) {
            RwRGBA newColor = *RpMaterialGetColor(material);
            newColor.alpha = sMaterialAlpha[sMaterialIdx];
            RpMaterialSetColor(material, &newColor);
        }
    }

    if (sMaterialFlags & 0x4) {
        RpMaterialSetTexture(material, sMaterialTexture[sMaterialIdx]);
    }

    sMaterialIdx++;
    return material;
}

void iModelResetMaterial(RpAtomic* model)
{
    if (model != sLastMaterial) {
        sMaterialFlags = 0;
    }

    RpGeometry* geom = RpAtomicGetGeometry(model);

    sMaterialIdx = 0;

    RpGeometryForAllMaterials(geom, iModelResetMaterialCB, NULL);

    sMaterialFlags = 0;
}

