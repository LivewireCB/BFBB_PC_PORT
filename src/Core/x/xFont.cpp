#include "xFont.h"

#include "xMath.h"
#include "xstransvc.h"
#include "xModel.h"
#include "xColor.h"
#include "xTimer.h"
#include "xTextAsset.h"
#include "xModelBucket.h"

#include "iTime.h"
#include "zScene.h"

#include <string.h>
#include <stdio.h>

/* xtextbox flags */

#define FLAG_UNK40 0x40
#define FLAG_UNK80 0x80

#define WRAP_WORD 0x0
#define WRAP_CHAR 0x10
#define WRAP_NONE 0x20
#define WRAP_MASK (WRAP_WORD | WRAP_CHAR | WRAP_NONE)

#define XJUSTIFY_LEFT 0x0
#define XJUSTIFY_RIGHT 0x1
#define XJUSTIFY_CENTER 0x2
#define XJUSTIFY_MASK (XJUSTIFY_LEFT | XJUSTIFY_RIGHT | XJUSTIFY_CENTER)

#define YJUSTIFY_TOP 0x0
#define YJUSTIFY_BOTTOM 0x4
#define YJUSTIFY_CENTER 0x8
#define YJUSTIFY_MASK (YJUSTIFY_TOP | YJUSTIFY_BOTTOM | YJUSTIFY_CENTER)


namespace
{
    struct font_asset
    {
        U32 tex_id;
        U16 u;
        U16 v;
        U8 du;
        U8 dv;
        U8 line_size;
        U8 baseline;
        struct
        {
            S16 x;
            S16 y;
        } space; // 0xC
        U32 flags; // 0x10
        U8 char_set[128]; // 0x14
        struct
        {
            U8 offset;
            U8 size;
        } char_pos[127]; // 0x94
    };

    struct font_data
    {
        font_asset* asset;
        U32 index_max;
        U8 char_index[256];
        F32 iwidth;
        F32 iheight;
        basic_rect<F32> tex_bounds[127];
        basic_rect<F32> bounds[127];
        xVec2 dstfrac[127];
        RwTexture* texture;
        RwRaster* raster;
    };

    struct model_cache_entry
    {
        U32 id;
        U32 order;
        xModelInstance* model;
    };

    // clang-format off
    const char* default_font_texture[] =
    {
        "font_sb",
        "font1_sb",
        "font_numbers"
    };

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4838 ) // conversion from 'char' to 'U8' requires a narrowing conversion
#endif

    font_asset default_font_assets[4] = {
    {
        1,
        0, 0, 18, 22,
        14, 0,
        0, 0,
        0x1,
        {
            '~', '{', '}', '#',
            'A', 'B', 'C', 'D',
            'E', 'F', 'G', 'H',
            'I', 'J', 'K', 'L',
            'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T',
            'U', 'V', 'W', 'X',
            'Y', 'Z', '0', '1',
            '2', '3', '4', '5',
            '6', '7', '8', '9',
            '?', '!', '.', ',',
            '-', ':', '_', '"',
            '\'','&', '(', ')',
            '<', '>', '/', '%',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '+'
        },
        {}
    },
    {
        0,
        0, 0, 18, 22,
        14, 0,
        1, 0,
        0,
        {
            'A', 'B', 'C', 'D',
            'E', 'F', 'G', 'H',
            'I', 'J', 'K', 'L',
            'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T',
            'U', 'V', 'W', 'X',
            'Y', 'Z', 'a', 'b',
            'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j',
            'k', 'l', 'm', 'n',
            'o', 'p', 'q', 'r',
            's', 't', 'u', 'v',
            'w', 'x', 'y', 'z',
            '0', '1', '2', '3',
            '4', '5', '6', '7',
            '8', '9', '?', '!',
            '.', ',', ';', ':',
            '+', '-', '=', '/',
            '&', '(', ')', '%',
            '"', '\'','_', '<',
            '>', '*', '[', ']',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '�', '�',
            '�', '�', '�', '~',
            '�', '�', '�', '@',
            '|'
        },
        {}
    },
    {
        1,
        148, 232, 6, 8,
        18, 0,
        0, 0,
        0x9,
        {
            '0', '1', '2', '3',
            '4', '5', '6', '7',
            '8', '9', 'A', 'B',
            'C', 'D', 'E', 'F',
            'G', 'H', 'I', 'J',
            'K', 'L', 'M', 'N',
            'O', 'P', 'Q', 'R',
            'S', 'T', 'U', 'V',
            'W', 'X', 'Y', 'Z',
            ',', '.', '/', '*',
            '+', '-', '=', ':',
            ';', '%', '<', '>',
            '[', ']', '|', '(',
            ')', '_'
        },
        {}
    },
    {
        2,
        0, 0, 32, 32,
        4, 0,
        0, 0,
        0,
        {
            '1', '2', '3', '4',
            '5', '6', '7', '8',
            '9', '0', '/'
        },
        {}
    }
    };

#ifdef _WIN32
#pragma warning( pop )
#endif
    // clang-format on

    font_data active_fonts[4];
    U32 active_fonts_size = 0;

#define VERT_BUFFER_SIZE 120

    RwIm2DVertex vert_buffer[VERT_BUFFER_SIZE];
    U32 vert_buffer_used = 0;

    F32 rcz = 0;
    F32 nsz = 0;

#define MODEL_CACHE_SIZE 8

    model_cache_entry model_cache[MODEL_CACHE_SIZE];
    bool model_cache_inited = false;

    basic_rect<S32> find_bounds(const iColor_tag* bits, const basic_rect<S32>& r, S32 pitch)
    {
        S32 diff = pitch - r.w;
        const iColor_tag* endp = bits + pitch * r.h;
        const iColor_tag* p = bits;
        S32 pmode = (p->r == p->g && p->g == p->b && p->r >= 240);

        S32 minx = r.x + r.w;
        S32 maxx = r.x - 1;
        S32 miny = r.y + r.h;
        S32 maxy = r.y - 1;

        S32 y = r.y;

        while (p != endp)
        {
            const iColor_tag* endline = p + r.w;
            S32 x = r.x;

            while (p != endline)
            {
                if ((pmode && p->a) || (!pmode && p->r))
                {
                    minx = MIN(x, minx);
                    maxx = MAX(x, maxx);
                    miny = MIN(y, miny);
                    maxy = MAX(y, maxy);
                }

                p++;
                x++;
            }

            p += diff;
            y++;
        }

        basic_rect<S32> b;
        b.x = minx;
        b.y = miny;
        b.w = maxx + 1 - minx;
        b.h = maxy + 1 - miny;

        return b;
    }

    bool reset_font_spacing(font_asset& a)
    {
        RwTexture* tex = (RwTexture*)xSTFindAsset(a.tex_id, NULL);
        if (!tex) return false;

        basic_rect<S32> char_bounds;
        char_bounds.w = a.du;
        char_bounds.h = a.dv;

        U8 baseline_count[256];
        memset(baseline_count, 0, sizeof(baseline_count));

        a.baseline = 0;

        S32 width = RwRasterGetWidth(RwTextureGetRaster(tex));
        S32 height = RwRasterGetHeight(RwTextureGetRaster(tex));
        RwImage* image = RwImageCreate(width, height, 32);
        if (!image) return false;

        RwImageAllocatePixels(image);
        RwImageSetFromRaster(image, RwTextureGetRaster(tex));

        const iColor_tag* bits = (const iColor_tag*)RwImageGetPixels(image);
        for (S32 i = 0; a.char_set[i] != '\0'; i++) {
            if (a.flags & 0x4) {
                char_bounds.x = a.u + i / a.line_size * char_bounds.w;
                char_bounds.y = a.v + i % a.line_size * char_bounds.h;
            }
            else {
                char_bounds.x = a.u + i % a.line_size * char_bounds.w;
                char_bounds.y = a.v + i / a.line_size * char_bounds.h;
            }

            const iColor_tag* p = bits + width * char_bounds.y + char_bounds.x;
            basic_rect<S32> r = find_bounds(p, char_bounds, width);

            if (a.flags & 0x8) {
                a.char_pos[i].offset = 0;
                a.char_pos[i].size = char_bounds.w;
            }
            else {
                a.char_pos[i].offset = r.x - char_bounds.x;
                a.char_pos[i].size = r.w;
            }

            S32 baseline = r.y - char_bounds.y + r.h + 1;

            if (++baseline_count[baseline] > baseline_count[a.baseline]) {
                a.baseline = baseline;
            }
        }

        RwImageDestroy(image);
        return true;
    }

    // FIXME: Float conversions seem to need work
    //basic_rect<F32> get_tex_bounds(const font_data& fd, U8 c) WIP
    //    // fix __typeof__ not working
    //{
    //    typedef __typeof__(((struct font_asset){0}).char_pos[0]) char_pos_t;

    //    F32 boundX;
    //    F32 boundY;
    //    F32 boundW;
    //    F32 boundH;
    //    char_pos_t* temp_r8;

    //    if (fd.asset->flags & 0x4)
    //    {
    //        boundX = (F32)(c / fd.asset->line_size);
    //        boundY = (F32)(c % fd.asset->line_size);
    //    }
    //    else
    //    {
    //        boundY = (F32)(c / fd.asset->line_size);
    //        boundX = (F32)(c % fd.asset->line_size);
    //    }

    //    temp_r8 = &fd.asset->char_pos[c];
    //    boundX = (F32)(temp_r8->offset + (fd.asset->du * boundX + (F32)fd.asset->u));
    //    boundW = (F32)temp_r8->size - 0.5f;
    //    boundY = ((F32)fd.asset->dv * boundY) + (F32)fd.asset->v;
    //    boundH = (F32)fd.asset->dv - 0.5f;

    //    basic_rect<F32> result = { boundX, boundY, boundW, boundH };
    //    result.scale((F32)fd.asset->dv, (F32)fd.asset->v);
    //    return result;
    //}

    basic_rect<F32> get_tex_bounds(const font_data& fd, U8 i) NONMATCH("https://decomp.me/scratch/j5ds6")
    {
        const font_asset& a = *fd.asset;
        basic_rect<F32> r;

        if (a.flags & 0x4) {
            r.x = (F32)(i / a.line_size);
            r.y = (F32)(i % a.line_size);
        }
        else {
            r.x = (F32)(i % a.line_size);
            r.y = (F32)(i / a.line_size);
        }

        r.x = a.char_pos[i].offset + (a.du * r.x + a.u);
        r.w = a.char_pos[i].size - 0.5f;
        r.y = a.dv * r.y + a.v;
        r.h = a.dv - 0.5f;
        r.scale(fd.iwidth, fd.iheight);

        return r;
    };

    basic_rect<F32> get_bounds(const font_data& fd, U8 i)
    {
        const font_asset& a = *fd.asset;
        basic_rect<F32> r;

        r.x = 0.0f;
        r.y = (F32)-a.baseline / a.dv;
        r.w = (F32)(a.char_pos[i].size + a.space.x) / (a.du + a.space.x);
        r.h = 1.0f;

        return r;
    }

    bool init_font_data(font_data& fd) NONMATCH("https://decomp.me/scratch/8uMna")
    {
        const font_asset& a = *fd.asset;
        fd.texture = (RwTexture*)xSTFindAsset(a.tex_id, NULL);
        if (!fd.texture) return false;

        RwTextureSetFilterMode(fd.texture, rwFILTERLINEAR);

        fd.raster = RwTextureGetRaster(fd.texture);

        S32 width = RwRasterGetWidth(fd.raster);
        S32 height = RwRasterGetHeight(fd.raster);
        fd.iwidth = 1.0f / width;
        fd.iheight = 1.0f / height;

        memset(fd.char_index, 255, sizeof(fd.char_index));
        fd.index_max = 0;

        while (a.char_set[fd.index_max] != '\0') {
            U8 i = fd.index_max;
            U8 c = a.char_set[i];

            fd.char_index[c] = i;

            if ((a.flags & 0x1) && c >= 'A' && c <= 'Z') {
                fd.char_index[c + 32] = i;
            }
            else if ((a.flags & 0x2) && c >= 'a' && c <= 'z') {
                fd.char_index[c - 32] = i;
            }

            fd.tex_bounds[i] = get_tex_bounds(fd, i);
            fd.bounds[i] = get_bounds(fd, i);
            fd.dstfrac[i].x = (F32)a.char_pos[i].size / (a.char_pos[i].size + a.space.x);
            fd.dstfrac[i].y = (F32)a.dv / (a.dv + a.space.y);
            fd.index_max++;
        }

        size_t tail_index = fd.index_max;

        if (fd.char_index[' '] == 255) {
            fd.char_index[' '] = (U8)tail_index;
            fd.tex_bounds[tail_index].assign(0.0f, 0.0f, 0.0f, 0.0f);
            fd.bounds[tail_index].assign(0.0f, (F32)-a.baseline / a.dv, (a.flags & 0x8) ? 1.0f : 0.5f, 1.0f);
            tail_index++;
        }

        if (fd.char_index['\n'] == 255) {
            fd.char_index['\n'] = (U8)tail_index;
            fd.tex_bounds[tail_index].assign(0.0f, 0.0f, 0.0f, 0.0f);
            fd.bounds[tail_index].assign(0.0f, (F32)-a.baseline / a.dv, 0.0f, 1.0f);
            tail_index++;
        }

        return true;
    }

    void start_tex_render(U32 font)
    {
        RwCamera* cam = RwCameraGetCurrentCamera();
        rcz = 1.0f / RwCameraGetNearClipPlane(cam);
        nsz = RwIm2DGetNearScreenZ();

        const font_data& fd = active_fonts[font];
        xfont::set_render_state(fd.raster);
    }

    void tex_flush()
    {
        if (vert_buffer_used)
        {
            RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert_buffer, vert_buffer_used);
            vert_buffer_used = 0;
        }
    }

    void stop_tex_render()
    {
        tex_flush();
        xfont::restore_render_state();
    }

    void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v, iColor_tag c);

    void tex_render(const basic_rect<F32>& src, const basic_rect<F32>& dst,
                    const basic_rect<F32>& clip, iColor_tag color)
    {
        basic_rect<F32> r = dst;
        basic_rect<F32> rt = src;

        clip.clip(r, rt);

        if (r.empty())
        {
            return;
        }

        if (vert_buffer_used == VERT_BUFFER_SIZE)
        {
            RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert_buffer, VERT_BUFFER_SIZE);
            vert_buffer_used = 0;
        }

        RwIm2DVertex* vert = &vert_buffer[vert_buffer_used];

        r.scale(640.0f, 480.0f);

        set_vert(vert[0], r.x, r.y, rt.x, rt.y, color);
        set_vert(vert[1], r.x, r.y + r.h, rt.x, rt.y + rt.h, color);
        set_vert(vert[2], r.x + r.w, r.y, rt.x + rt.w, rt.y, color);

        vert[3] = vert[2];
        vert[4] = vert[1];

        set_vert(vert[5], r.x + r.w, r.y + r.h, rt.x + rt.w, rt.y + rt.h, color);

        vert_buffer_used += 6;
    }
} // namespace

namespace
{
    void set_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 u, F32 v, iColor_tag c)
    {
        RwIm2DVertexSetScreenX(&vert, x);
        RwIm2DVertexSetScreenY(&vert, y);
        RwIm2DVertexSetScreenZ(&vert, nsz);
        RwIm2DVertexSetU(&vert, u, rcz);
        RwIm2DVertexSetV(&vert, v, rcz);
        RwIm2DVertexSetIntRGBA(&vert, c.r, c.g, c.b, c.a);
    }

    void init_model_cache()
    {
        struct model_pool
        {
            RwMatrix mat[MODEL_CACHE_SIZE];
            xModelInstance model[MODEL_CACHE_SIZE];
        };

        // non-matching: two instructions swapped

        model_cache_inited = true;

        void* data = xMemAlloc(gActiveHeap, sizeof(model_pool), 16);
        memset(data, 0, sizeof(model_pool));

        model_pool& pool = *(model_pool*)data;

        for (U32 i = 0; i < MODEL_CACHE_SIZE; i++)
        {
            xModelInstance& model = pool.model[i];
            model_cache_entry& e = model_cache[i];

            e.order = 0;
            e.id = 0;
            e.model = &model;

            model.Mat = &pool.mat[i];
            model.Flags = 0x1;
            model.BoneCount = 1;
            model.shadowID = 0xDEADBEEF;
        }
    }

    xModelInstance* load_model(U32 id)
    {
        static U32 next_order = 0;
        U32 oldest = 0;

        next_order++;

        for (U32 i = 0; i < MODEL_CACHE_SIZE; i++)
        {
            model_cache_entry& e = model_cache[i];

            if (e.id == id)
            {
                e.order = next_order;
                return e.model;
            }

            if (e.order < model_cache[oldest].order)
            {
                oldest = i;
            }
        }

        RpAtomic* mf = (RpAtomic*)xSTFindAsset(id, NULL);

        if (!mf)
        {
            return NULL;
        }

        // non-matching: instruction order

        model_cache_entry& e = model_cache[oldest];

        e.id = id;
        e.order = next_order;

        xModelInstance& model = *e.model;

        xMat4x3Identity((xMat4x3*)model.Mat);

        model.Data = mf;
        model.Bucket = xModelBucket_GetBuckets(model.Data);
        model.PipeFlags =
            (model.Bucket) ? model.Bucket[0]->PipeFlags : xModelGetPipeFlags(model.Data);

        return &model;
    }
} // namespace

void xfont::init()
{
    active_fonts_size = 0;

    for (size_t i = 0; i < sizeof(default_font_assets) / sizeof(font_asset); i++)
    {
        if (default_font_assets[i].tex_id < sizeof(default_font_texture) / sizeof(const char*))
        {
            default_font_assets[i].tex_id =
                xStrHash(default_font_texture[default_font_assets[i].tex_id]);
        }

        xSTFindAsset(default_font_assets[i].tex_id, NULL);

        if (reset_font_spacing(default_font_assets[i]))
        {
            font_data& fd = active_fonts[active_fonts_size];
            fd.asset = &default_font_assets[i];

            if (init_font_data(fd))
            {
                active_fonts_size++;
            }
        }
    }

    init_model_cache();
}

namespace
{
    struct
    {
        S32 fogenable;
        S32 vertexalphaenable;
        S32 zwriteenable;
        S32 ztestenable;
        U32 srcblend;
        U32 destblend;
        U32 shademode;
        RwRaster* textureraster;
        RwTextureFilterMode filter;
    } oldrs;
} // namespace

void xfont::set_render_state(RwRaster* raster)
{
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, (void*)&oldrs.fogenable);
    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&oldrs.srcblend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&oldrs.destblend);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)&oldrs.vertexalphaenable);
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, (void*)&oldrs.textureraster);
    RwRenderStateGet(rwRENDERSTATESHADEMODE, (void*)&oldrs.shademode);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, (void*)&oldrs.zwriteenable);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)&oldrs.ztestenable); // RwRenderStateSet lol
    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, (void*)&oldrs.filter);

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
}

void xfont::restore_render_state()
{
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)oldrs.fogenable);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)oldrs.srcblend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)oldrs.destblend);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)oldrs.vertexalphaenable);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)oldrs.textureraster);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)oldrs.shademode);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)oldrs.zwriteenable);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)oldrs.ztestenable);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)oldrs.filter);
}

basic_rect<F32> xfont::bounds(char c) const
{
    font_data& fd = active_fonts[id];
    U32 char_index = fd.char_index[c];

    if (fd.char_index[c] == 0xFF)
    {
        // non-matching: scheduling
        return basic_rect<F32>::m_Null;
    }

    basic_rect<F32> r = fd.bounds[char_index];
    r.scale(width, height);
    return r;
}

basic_rect<F32> xfont::bounds(const char* text) const
{
    size_t size;
    return bounds(text, 0x40000000, 1.0e38f, size);
}

basic_rect<F32> xfont::bounds(const char* text, size_t text_size, F32 max_width, size_t& size) const
{
    font_data& fd = active_fonts[id];
    basic_rect<F32> r;

    r.x = 0.0f;
    r.y = fd.bounds[0].y * height;
    r.w = 0.0f;
    r.h = fd.bounds[0].h * height;

    if (!text || !text_size)
    {
        size = 0;
        return r;
    }

    const char* s = text;
    U32 i = 0;

    while (i < text_size && *s != '\0')
    {
        U32 charIndex = fd.char_index[*s];

        if (charIndex != 0xFF)
        {
            F32 dx = fd.bounds[charIndex].w * width;

            if (r.w + dx > max_width)
            {
                break;
            }

            r.w += dx + space;
        }

        i++;
        s++;
    }

    if (r.w > 0.0f)
    {
        r.w -= space;
    }

    size = s - text;
    return r;
}

void xfont::start_render() const
{
    start_tex_render(id);
}

void xfont::stop_render() const
{
    stop_tex_render();
}

namespace
{
    void char_render(U8 c, U32 font_index, const basic_rect<F32>& bounds,
                     const basic_rect<F32>& clip, iColor_tag color)
    {
        font_data& fd = active_fonts[font_index];
        U32 char_index = fd.char_index[c];

        if (char_index >= fd.index_max)
        {
            return;
        }

        basic_rect<F32> dst = fd.bounds[char_index];
        dst.scale(bounds.w, bounds.h);
        dst.x += bounds.x;
        dst.y += bounds.y;
        dst.w *= fd.dstfrac[char_index].x;
        dst.h *= fd.dstfrac[char_index].y;

        tex_render(fd.tex_bounds[char_index], dst, clip, color);
    }

    RwRaster* set_tex_raster(RwRaster* raster)
    {
        RwRaster* r;
        RwRenderStateGet(rwRENDERSTATETEXTURERASTER, (void*)&r);

        if (raster == r)
        {
            return raster;
        }

        tex_flush();
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);

        return r;
    }
} // namespace

void xfont::irender(const char* text, F32 x, F32 y) const
{
    irender(text, 0x40000000, x, y);
}

static const basic_rect<F32> _1107 = {};

void xfont::irender(const char* text, size_t text_size, F32 x, F32 y) const NONMATCH("https://decomp.me/scratch/RLMbg")
{
    if (!text) return;

    const font_data& fd = active_fonts[id];
    set_tex_raster(fd.raster);

    basic_rect<F32> bounds = {};
    bounds.x = x, bounds.y = y, bounds.w = width, bounds.h = height;

    const char* s = text;
    for (size_t i = 0; i < text_size && *s != '\0'; i++, s++) {
        U32 c = *s;
        char_render(c, id, bounds, clip, color);

        U32 charIndex = fd.char_index[c];
        if (charIndex != 255) {
            bounds.x += fd.bounds[charIndex].w * width + space;
        }
    }
}

namespace
{
    substr text_delims = { " \t\n{}=*+:;,", 11 };

    size_t parse_split_tag(xtextbox::split_tag& ti)
    {
        ti.value.size = 0;
        ti.action.size = 0;
        ti.name.size = 0;

        // non-matching: scheduling

        substr s;

        s.text = ti.tag.text;
        s.size = ti.tag.size;

        s.text++;
        s.size--;

        ti.name.text = skip_ws(s);
        s.text = find_char(s, text_delims);

        if (!s.text)
        {
            return 0;
        }

        ti.name.size = s.text - ti.name.text;
        s.size -= ti.name.size;
        ti.action.text = skip_ws(s);

        if (!s.size)
        {
            return 0;
        }

        char c = s.text[0];

        if (c == '\0' || c == '{')
        {
            return 0;
        }

        s.text++;
        s.size--;

        if (c == '}')
        {
            return ti.tag.size - s.size;
        }

        ti.action.size = 1;
        ti.value.text = skip_ws(s);
        s.text = find_char(s, '}');
        s.size -= s.text - ti.value.text;

        if (!s.text)
        {
            return 0;
        }

        ti.value.size = s.text - ti.value.text;

        rskip_ws(ti.value);

        s.text++;
        s.size--;

        return ti.tag.size - s.size;
    }

    const char* parse_next_tag_jot(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb,
                                   const char* text, size_t text_size)
    {
        xtextbox::split_tag ti = {};
        ti.tag.text = text;
        ti.tag.size = text_size;

        size_t size = parse_split_tag(ti);

        if (!size)
        {
            return NULL;
        }

        a.s.text = text;
        a.s.size = size;
        a.flag.invisible = a.flag.ethereal = true;

        if (icompare(ti.name, substr::create("~", 1)) == 0 ||
            icompare(ti.name, substr::create("reset", 5)) == 0)
        {
            a.tag = xtextbox::find_format_tag(ti.value);

            if (a.tag && a.tag->reset_tag)
            {
                a.tag->reset_tag(a, tb, ctb, ti);
            }
        }
        else
        {
            a.tag = xtextbox::find_format_tag(ti.name);

            if (a.tag && a.tag->parse_tag)
            {
                a.tag->parse_tag(a, tb, ctb, ti);
            }
        }

        return text + size;
    }

    const char* parse_next_text_jot(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb,
                                    const char* text, size_t text_size)
    {
        char c = text[0];

        a.s.text = text;
        a.s.size = 1;
        a.flag.merge = true;

        if (c == '\n')
        {
            a.flag.line_break = true;
        }
        else if (c == '\t')
        {
            a.flag.tab = true;
        }
        else if (c == '-')
        {
            a.flag.word_end = true;
        }

        if (is_ws(c))
        {
            a.flag.invisible = a.flag.word_break = true;
        }

        a.bounds = tb.font.bounds(c);
        a.cb = &xtextbox::text_cb;
        a.context = NULL;
        a.context_size = 0;

        return a.s.text + a.s.size;
    }

    const char* parse_next_jot(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const char* text, size_t text_size)
    {
        char c = text[0];
        const char* next;

        if (c != '{' || !(next = parse_next_tag_jot(a, tb, ctb, text, text_size))) {
            next = parse_next_text_jot(a, tb, ctb, text, text_size);
        }

        a.flag.merge = a.flag.merge && !(tb.flags & 0x80);
        return next;
    }

    struct tex_args
    {
        RwRaster* raster;
        F32 rot;
        basic_rect<F32> src;
        basic_rect<F32> dst;
        xVec2 off;
        enum
        {
            SCALE_FONT,
            SCALE_SCREEN,
            SCALE_SIZE,
            SCALE_FONT_WIDTH,
            SCALE_FONT_HEIGHT,
            SCALE_SCREEN_WIDTH,
            SCALE_SCREEN_HEIGHT
        } scale;
    };

    struct model_args
    {
        xModelInstance* model;
        xVec3 rot;
        basic_rect<F32> dst;
        xVec2 off;
        enum
        {
            SCALE_FONT,
            SCALE_SCREEN,
            SCALE_SIZE
        } scale;
    };

    tex_args def_tex_args;
    model_args def_model_args;

    void reset_tex_args(tex_args& ta)
    {
        ta.raster = NULL;
        ta.rot = 0.0f;
        ta.src = ta.dst = basic_rect<F32>::m_Unit;
        ta.off.x = ta.off.y = 1.0f;
        ta.scale = tex_args::SCALE_FONT;
    }

    void load_tex_args(tex_args& ta, const substr& s)
    {
        xtextbox::tag_entry_list el = xtextbox::read_tag(s);
        if (el.size == 0) return;

        const xtextbox::tag_entry* e = &el.entries[0];
        if (e->op == ':' || (e->args_size == 1 && e->args[0].size != 0)) {
            const substr& name = e->args[0];
            U32 id;
            if (name.size == 10 && imemcmp(name.text, "0x", 2) == 0) {
                id = atox(substr::create(name.text + 2, 8));
            }
            else {
                id = xStrHash(name.text, name.size);
            }

            RwTexture* texture = (RwTexture*)xSTFindAsset(id, NULL);
            if (texture &&
                texture->raster &&
                texture->raster->width > 0 &&
                texture->raster->height > 0 &&
                texture->raster->width <= 4096 &&
                texture->raster->height <= 4096) {
                RwTextureSetFilterMode(texture, rwFILTERLINEAR);
                ta.raster = RwTextureGetRaster(texture);
            }
        }

        el.entries++;
        el.size--;

        e = xtextbox::find_entry(el, substr::create("rot", 3));
        if (e && e->op == '=' && e->args_size == 1) {
            xtextbox::read_list(*e, &ta.rot, 1);
        }

        e = xtextbox::find_entry(el, substr::create("src", 3));
        if (e && e->op == '=' && e->args_size == 4) {
            xtextbox::read_list(*e, &ta.src.x, 4);
        }

        e = xtextbox::find_entry(el, substr::create("dst", 3));
        if (e && e->op == '=' && e->args_size == 4) {
            xtextbox::read_list(*e, &ta.dst.x, 4);
        }

        e = xtextbox::find_entry(el, substr::create("off", 3));
        if (e && e->op == '=' && e->args_size == 2) {
            xtextbox::read_list(*e, &ta.off.x, 2);
        }

        e = xtextbox::find_entry(el, substr::create("scale", 5));
        if (e && e->op == '=' && e->args_size == 1) {
            if (icompare(e->args[0], substr::create("font", 4)) == 0) {
                ta.scale = tex_args::SCALE_FONT;
            }
            else if (icompare(e->args[0], substr::create("screen", 6)) == 0) {
                ta.scale = tex_args::SCALE_SCREEN;
            }
            else if (icompare(e->args[0], substr::create("size", 4)) == 0) {
                ta.scale = tex_args::SCALE_SIZE;
            }
            else if (icompare(e->args[0], substr::create("font_width", 10)) == 0) {
                ta.scale = tex_args::SCALE_FONT_WIDTH;
            }
            else if (icompare(e->args[0], substr::create("font_height", 11)) == 0) {
                ta.scale = tex_args::SCALE_FONT_HEIGHT;
            }
            else if (icompare(e->args[0], substr::create("screen_width", 12)) == 0) {
                ta.scale = tex_args::SCALE_SCREEN_WIDTH;
            }
            else if (icompare(e->args[0], substr::create("screen_height", 13)) == 0) {
                ta.scale = tex_args::SCALE_SCREEN_HEIGHT;
            }
            else {
                ta.scale = tex_args::SCALE_FONT;
            }
        }
    }

    void reset_model_args(model_args& ma)
    {
        ma.model = NULL;
        ma.rot = xVec3::m_Null;
        ma.dst = basic_rect<F32>::m_Unit;
        ma.off.x = ma.off.y = 1.0f;
        ma.scale = model_args::SCALE_FONT;
    }

    void load_model_args(model_args& ma, const substr& s)
    {
        xtextbox::tag_entry_list el = xtextbox::read_tag(s);
        if (el.size == 0) return;

        const xtextbox::tag_entry* e = &el.entries[0];
        if (e->op == ':' || (e->args_size == 1 && e->args[0].size != 0)) {
            const substr& name = e->args[0];
            U32 id = xStrHash(name.text, name.size);
            ma.model = load_model(id);
        }

        el.entries++;
        el.size--;

        e = xtextbox::find_entry(el, substr::create("rot", 3));
        if (e && e->op == '=' && e->args_size == 3) {
            xtextbox::read_list(*e, &ma.rot.x, 3);
        }

        e = xtextbox::find_entry(el, substr::create("dst", 3));
        if (e && e->op == '=' && e->args_size == 4) {
            xtextbox::read_list(*e, &ma.dst.x, 4);
        }

        e = xtextbox::find_entry(el, substr::create("off", 3));
        if (e && e->op == '=' && e->args_size == 2) {
            xtextbox::read_list(*e, &ma.off.x, 2);
        }

        e = xtextbox::find_entry(el, substr::create("scale", 5));
        if (e && e->op == '=' && e->args_size == 1) {
            if (icompare(e->args[0], substr::create("screen", 6)) == 0) {
                ma.scale = model_args::SCALE_SCREEN;
            }
            else {
                ma.scale = model_args::SCALE_FONT;
            } // model_args::SCALE_SIZE unused?
        }
    }

    void start_layout(const xtextbox&)
    {
        reset_tex_args(def_tex_args);
        reset_model_args(def_model_args);
    }

    void stop_layout(const xtextbox&)
    {
    }

    void start_render(const xtextbox& tb)
    {
        tb.font.start_render();
    }

    void stop_render(const xtextbox& tb)
    {
        tb.font.stop_render();
    }
} // namespace

void xtextbox::text_render(const jot& j, const xtextbox& tb, F32 x, F32 y)
{
    tb.font.irender(j.s.text, j.s.size, x, y);
}

void xtextbox::set_text(const char* text)
{
    set_text(text, 0x40000000);
}

void xtextbox::set_text(const char* text, size_t text_size)
{
    if (!text || !text_size)
    {
        texts_size = 0;
        text_hash = 0;
        return;
    }

    this->text.text = text;
    this->text.size = text_size;

    set_text(&this->text.text, &this->text.size, 1);
}

void xtextbox::set_text(const char** texts, size_t size)
{
    set_text(texts, NULL, size);
}

void xtextbox::set_text(const char** texts, const size_t* text_sizes, size_t size)
{
    this->texts_size = size;
    this->text_hash = 0;

    if (!size)
    {
        return;
    }

    this->texts = texts;
    this->text_sizes = text_sizes;

    if (!text_sizes)
    {
        for (size_t i = 0; i < size; i++)
        {
            this->text_hash = this->text_hash * 131 + xStrHash(texts[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < size; i++)
        {
            this->text_hash = this->text_hash * 131 + xStrHash(texts[i], text_sizes[i]);
        }
    }
}

namespace tweaker
{
    namespace
    {
        void log_cache(bool)
        {
        }
    } // namespace
} // namespace tweaker

namespace
{
    struct tl_cache_entry
    {
        U32 used;
        iTime last_used;
        xtextbox::layout tl;
    };

#ifdef GAMECUBE
#define TL_CACHE_COUNT 1
#else
#ifdef PS2
#define TL_CACHE_COUNT 3
#else
#define TL_CACHE_COUNT 3 // just made this value up for windows cus idk what it does yet. Considering this is based off GC it should probably be 1
#endif
#endif

    tl_cache_entry tl_cache[TL_CACHE_COUNT];
} // namespace

const size_t MAX_CACHE = 1;


xtextbox::layout& xtextbox::temp_layout(bool cache) const NONMATCH("https://decomp.me/scratch/Ok3UW")
{
    iTime r29 = iTimeFromSec(1.0f);
    iTime cur_time = iTimeGet();
    bool refresh = false;
    size_t index = 0;

    if (cache) {
        if (tl_cache[0].tl.changed(*this)) {
            index = 1;
        }
    }
    else {
        index = 1;
    }

    tweaker::log_cache(index > 1);

    if (index >= 1) {
        refresh = true;
        index = 0;

        for (size_t i = 0; i < MAX_CACHE; i++) {
            if (r29 < cur_time - tl_cache[i].last_used) {
                index = i;
            }
            else {
                S32 used = tl_cache[i].used;
                if (tl_cache[i].tl.dynamics_size != 0) used -= 10;
                if (tl_cache[i].tl.jots_size() > 50) used += 10;
                if (used < 1000000000) index = i;
            }
        }
    }

    tl_cache_entry& e = tl_cache[index];
    if (refresh) {
        e.used = 0;
        e.tl.refresh(*this, true);
    }
    else {
        e.tl.tb = *this;
    }

    if (cache) {
        e.used++;
        e.last_used = cur_time;
    }

    return e.tl;
}

void xtextbox::render(layout& l, S32 begin_jot, S32 end_jot) const
{
    l.render(*this, begin_jot, end_jot);
}

F32 xtextbox::yextent(F32 max, S32& size, const layout& l, S32 begin_jot, S32 end_jot) const
{
    return l.yextent(max, size, begin_jot, end_jot);
}

xtextbox::callback xtextbox::text_cb = { xtextbox::text_render, NULL, NULL };

#define skip_char_ptr(s, p)                                                                        \
    {                                                                                              \
        const char* temp = (s).text;                                                               \
        (s).text = (p) + 1;                                                                        \
        (s).size -= (p) + 1 - temp;                                                                \
    }

#define skip_char(s, c)                                                                            \
    {                                                                                              \
        const char* p = find_char((s), (c));                                                       \
        if (p)                                                                                     \
        {                                                                                          \
            skip_char_ptr((s), p);                                                                 \
        }                                                                                          \
    }

const xtextbox::tag_entry_list _1642 = {};

xtextbox::tag_entry_list xtextbox::read_tag(const substr& s)
{
    size_t args_used = 0;
    size_t entries_used = 0;
    substr it = s;
    const char* d = find_char(it, '{');
    if (d) {
        it.size -= d + 1 - it.text;
        it.text = d + 1;
    }

    substr delims = { "=:+*;}", 6 };
    substr sub_delims = { ",;}", 3 };

    static substr arg_buffer[32];
    static tag_entry entry_buffer[16];

    while (it.size) {
        tag_entry& entry = entry_buffer[entries_used];
        entry.args_size = 0;
        entry.op = 0;
        entry.name.text = it.text;

        const char* d = find_char(it, delims);
        entry.name.size = !d ? it.size : d - it.text;

        trim_ws(entry.name);
        if (entry.name.size) entries_used++;

        if (!d || *d == '}') break;

        it.size -= d + 1 - it.text;
        it.text = d + 1;

        if (*d != ';') {
            entry.op = *d;
            entry.args = arg_buffer + args_used;

            while (it.size) {
                substr& arg = arg_buffer[args_used];
                arg.text = it.text;

                const char* d = find_char(it, sub_delims);
                if (!d) {
                    arg.size = it.size;
                    it.size = 0;
                }
                else {
                    arg.size = d - it.text;
                    it.size -= arg.size + 1;
                    it.text += arg.size + 1;
                }

                trim_ws(arg);
                if (arg.size) {
                    args_used++;
                    entry.args_size++;
                }

                if (!d || *d == '}') {
                    it.size = 0;
                    break;
                }

                if (*d == ';') break;
            }
        }
    }

    xtextbox::tag_entry_list ret = { entry_buffer, entries_used };
    return ret;
}

const xtextbox::tag_entry* xtextbox::find_entry(const tag_entry_list& el, const substr& name) NONMATCH("https://decomp.me/scratch/VCWdJ")
{
    for (size_t i = 0; i < el.size; i++) {
        const tag_entry& e = el.entries[i];
        if (icompare(name, e.name) == 0) {
            return &e;
        }
    }
    return NULL;
}

size_t xtextbox::read_list(const tag_entry& e, F32* v, size_t vsize) NONMATCH("https://decomp.me/scratch/skY56")
{
    size_t total = MIN(vsize, e.args_size);
    for (size_t i = 0; i < total; i++) {
        v[i] = xatof(e.args[i].text);
    }
    return total;
}

size_t xtextbox::read_list(const tag_entry& e, S32* v, size_t vsize) NONMATCH("https://decomp.me/scratch/SC5cl")
{
    size_t total = MIN(vsize, e.args_size);
    for (size_t i = 0; i < total; i++) {
        v[i] = atoi(e.args[i].text);
    }
    return total;
}

void xtextbox::clear_layout_cache()
{
    for (size_t index = 0; index < MAX_CACHE; index++) {
        tl_cache[index].tl.clear();
    }
}

void xtextbox::layout::refresh(const xtextbox& tb, bool force)
{
    if (force || changed(tb))
    {
        this->tb = tb;
        calc(tb, 0);
    }
}

void xtextbox::layout::refresh_end(const xtextbox& tb)
{
    if (this->tb.texts_size > tb.texts_size) {
        size_t start_text = this->tb.texts_size;
        this->tb = tb;
        calc(tb, start_text);
    }
}

void xtextbox::layout::clear()
{
    _jots_size = _lines_size = context_buffer_size = dynamics_size = 0;
    tb = xtextbox::create();
}

void xtextbox::layout::trim_line(jot_line& line)
{
    for (S32 i = line.last - 1; i >= (S32)line.first; i--) {
        jot& a = _jots[i];
        if (!a.flag.ethereal) {
            if (a.flag.invisible) {
                erase_jots(i, i + 1);
                line.last--;
            }
            break;
        }
    }

    for (size_t i = line.first; i < line.last; i++) {
        jot& a = _jots[i];
        if (!a.flag.ethereal) {
            if (a.flag.invisible) {
                erase_jots(i, i + 1);
            }
            break;
        }
    }
}

void xtextbox::layout::erase_jots(size_t first, size_t last)
{
    if (last >= _jots_size) {
        _jots_size = first;
        return;
    }

    size_t offset = last - first;
    _jots_size -= offset;

    for (size_t i = first; i < _jots_size; i++) {
        jot& a = _jots[i];
        a = _jots[i + offset];
    }
}

void xtextbox::layout::merge_line(jot_line& line)
{
    size_t d = line.first;
    for (size_t i = line.first + 1; i != line.last; i++) {
        jot& a1 = _jots[d];
        jot& a2 = _jots[i];
        if (!a1.flag.ethereal &&
            !a2.flag.ethereal &&
            a1.flag.merge &&
            a2.flag.merge &&
            a1.cb == a2.cb) {
            a1.s.size = a2.s.text - a1.s.text + a2.s.size;
            a1.bounds |= a2.bounds;
            a1.intersect_flags(a2);
        }
        else {
            d++;
            if (d != i) {
                _jots[d] = a2;
            }
        }
    }
    d++;
    erase_jots(d, line.last);
    line.last = d;
}

void xtextbox::layout::bound_line(jot_line& line)
{
    line.bounds.w = line.bounds.h = line.baseline = 0.0f;

    for (size_t i = line.first; i != line.last; i++)
    {
        jot& a = _jots[i];

        if (!a.flag.ethereal && -a.bounds.y > line.baseline)
        {
            line.baseline = -a.bounds.y;
        }
    }

    for (size_t i = line.first; i != line.last; i++)
    {
        jot& a = _jots[i];

        if (!a.flag.ethereal)
        {
            a.bounds.x = line.bounds.w;
            line.bounds.w += a.bounds.w;

            F32 total_height = line.baseline + a.bounds.y + a.bounds.h;

            if (total_height > line.bounds.h)
            {
                line.bounds.h = total_height;
            }
        }
    }

    line.page_break = (line.last > line.first && _jots[line.last - 1].flag.page_break);
}

bool xtextbox::layout::fit_line()
{
    jot_line& line = _lines[_lines_size];

    if (line.bounds.w > tb.bounds.w) {
        switch (tb.flags & 0x30) {
        case 0x10:
            if (line.last > line.first + 1) {
                line.last--;
            }
            break;
        case 0x20:
            return false;
        default:
        {
            for (S32 i = line.last - 1; i > (S32)line.first; i--) {
                jot& a = _jots[i];
                if (a.flag.word_break) {
                    line.last = i + 1;
                    break;
                }
                else if (_jots[i - 1].flag.word_end) {
                    line.last = i;
                    break;
                }
            }

            if (line.last <= line.first) {
                line.last = line.first + 1;
            }

            trim_line(line);
            break;
        }
        }
    }

    merge_line(line);
    bound_line(line);

    return true;
}

void xtextbox::layout::next_line()
{
    jot_line& line1 = _lines[_lines_size];

    _lines_size++;

    jot_line& line2 = _lines[_lines_size];

    line2.first = line1.last;
    line2.last = _jots_size;
    line2.bounds.x = 0.0f;
    line2.bounds.y = line1.bounds.y + line1.bounds.h;

    bound_line(line2);
}

void xtextbox::layout::calc(const xtextbox& ctb, size_t start_text)
{
    if (!start_text)
    {
        dynamics_size = 0;
        context_buffer_size = 0;
        _lines_size = 0;
        _jots_size = 0;
    }

    if (!tb.texts_size)
    {
        return;
    }

    start_layout(ctb);

    jot_line& first_line = _lines[_lines_size];
    first_line.first = 0;
    first_line.bounds.w = 0.0f;
    first_line.bounds.x = 0.0f;
    first_line.bounds.y = 0.0f;
    first_line.baseline = 0.0f;

    struct
    {
        const char* s;
        const char* end;
    } text_stack[16];

    size_t text_stack_size = 0;
    size_t size = ((!tb.text_sizes) ? 0x40000000 : tb.text_sizes[start_text]);
    size_t text_index = start_text + 1;
    const char* s = tb.texts[start_text];
    const char* end = s + size;
    const char* r25;

    while (true)
    {
        jot& a = _jots[_jots_size];
        jot_line& line = _lines[_lines_size];

        a.context = &context_buffer[context_buffer_size];
        a.context_size = 0;
        a.reset_flags();
        a.cb = NULL;
        a.tag = NULL;

        if (s == end || *s == '\0')
        {
            if (text_stack_size)
            {
                text_stack_size--;
                s = text_stack[text_stack_size].s;
                end = text_stack[text_stack_size].end;
            }
            else if (text_index < tb.texts_size)
            {
                size = ((!tb.text_sizes) ? 0x40000000 : tb.text_sizes[text_index]);

                s = tb.texts[text_index];
                end = s + size;
                text_index++;
            }
            else
            {
                break;
            }

            a.flag.invisible = a.flag.ethereal = true;
            a.s = substr::create(NULL, 0);

            _jots_size++;
        }
        else
        {
            r25 = parse_next_jot(a, tb, ctb, s, end - s);

            if (a.context == &context_buffer[context_buffer_size])
            {
                context_buffer_size += ALIGN(a.context_size, 4);
            }

            _jots_size++;

            if (a.cb && a.cb->layout_update)
            {
                a.cb->layout_update(a, tb, ctb);
            }

            if (a.flag.stop)
            {
                break;
            }

            if (!a.flag.ethereal)
            {
                a.bounds.x += line.bounds.w;
                line.bounds.w += a.bounds.w;

                if (line.bounds.w >= tb.bounds.w)
                {
                    line.last = _jots_size;

                    if (!fit_line())
                    {
                        break;
                    }

                    next_line();
                }
            }

            if (a.flag.line_break || a.flag.page_break)
            {
                line.last = _jots_size;

                if (!fit_line())
                {
                    break;
                }

                next_line();
            }

            s = r25;

            if (a.flag.insert)
            {
                s = (const char*)a.context;
                text_stack[text_stack_size].s = r25;
                text_stack[text_stack_size].end = end;
                text_stack_size++;
                end = s + a.context_size;
            }
        }
    }

    jot_line& last_line = _lines[_lines_size];

    if (last_line.first < _jots_size)
    {
        last_line.last = _jots_size;

        if (fit_line())
        {
            _lines_size++;
        }
    }

    for (size_t i = 0; i < _jots_size; i++)
    {
        if (_jots[i].flag.dynamic)
        {
            dynamics[dynamics_size++] = (U16)i;
        }
    }

    stop_layout(ctb);
}

void xtextbox::layout::render(const xtextbox& ctb, S32 begin_jot, S32 end_jot)
{
    if (begin_jot < 0) begin_jot = 0;
    if (end_jot < begin_jot) end_jot = _jots_size;
    if (begin_jot >= end_jot) return;

    tb = ctb;
    start_render(ctb);

    S32 begin_line = 0;
    while (true) {
        if (begin_line >= (S32)_lines_size) {
            stop_render(ctb);
            break;
        }

        if ((S32)_lines[begin_line].last <= begin_jot) {
            begin_line++;
        }
        else {
            for (S32 i = 0; i < begin_jot; i++) {
                jot& j = _jots[i];
                if (j.cb && j.cb->render_update) {
                    j.cb->render_update(j, tb, ctb);
                }
            }

            F32 top = _lines[begin_line].bounds.y;
            size_t li = begin_line - 1;
            S32 line_last = -1;
            F32 x, y;

            for (S32 i = begin_jot; i < end_jot; i++) {
                if (i >= line_last) {
                    li++;

                    const jot_line& line = _lines[li];
                    line_last = line.last;
                    x = tb.bounds.x + line.bounds.x;
                    y = tb.bounds.y + line.bounds.y + line.baseline - top;

                    U32 xj = tb.flags & 0x3;
                    if (xj == 2) {
                        x += (tb.bounds.w - line.bounds.w) * 0.5f;
                    }
                    else if (xj == 1) {
                        x += tb.bounds.w - line.bounds.w;
                    }

                    if (line.page_break && end_jot > line_last) {
                        end_jot = line_last;
                    }
                }

                jot& j = _jots[i];
                if (j.cb) {
                    if (j.cb->render_update) {
                        j.cb->render_update(j, tb, ctb);
                    }
                    if (!j.flag.ethereal && !j.flag.invisible && j.cb->render) {
                        j.cb->render(j, tb, x + j.bounds.x, y);
                    }
                }
            }

            stop_render(ctb);
            break;
        }
    }
}

// this is different than the one in xMath.h
#undef min
#define min(a, b) ((a) >= (b) ? (b) : (a))

F32 xtextbox::layout::yextent(F32 max, S32& size, S32 begin_jot, S32 end_jot) const NONMATCH("https://decomp.me/scratch/AQKtE")
{
    size = 0;
    if (begin_jot < 0) begin_jot = 0;
    if (end_jot < begin_jot) end_jot = _jots_size;
    if (begin_jot >= end_jot) return 0.0f;

    S32 begin_line = 0;
    while (true) {
        if (begin_line >= (S32)_lines_size) return 0.0f;

        if ((S32)_lines[begin_line].last <= begin_jot) {
            begin_line++;
        }
        else {
            F32 top = _lines[begin_line].bounds.y;
            max += top;

            S32 i = begin_line;
            while (true) {
                if (i == (S32)_lines_size) break;
                const jot_line& line = _lines[i];
                if (line.bounds.y + line.bounds.h > max) {
                    i--;
                    break;
                }
                if ((S32)line.last >= end_jot) break;
                if (line.page_break) break;
                i++;
            }
            if (i < begin_line) return 0.0f;

            const jot_line& line = _lines[i];

            size = ((S32)line.last >= end_jot ? end_jot : line.last) - begin_jot;
            return line.bounds.y + line.bounds.h - top;
        }
    }
}

bool xtextbox::layout::changed(const xtextbox& ctb)
{
    U32 flags1 = tb.flags & 0x70;
    U32 flags2 = ctb.flags & 0x70;

    if (tb.text_hash != ctb.text_hash ||
        tb.font.id != ctb.font.id ||
        tb.font.width != ctb.font.width ||
        tb.font.height != ctb.font.height ||
        tb.font.space != ctb.font.space ||
        tb.bounds.w != ctb.bounds.w ||
        flags1 != flags2 ||
        tb.line_space != ctb.line_space) {
        return true;
    }

    S32 i = dynamics_size;
    while (i > 0) {
        i--;

        jot& j = _jots[dynamics[i]];
        U32 oldval = xStrHash((const char*)j.context, j.context_size);
        parse_next_jot(j, tb, ctb, j.s.text, j.s.size);
        U32 val = xStrHash((const char*)j.context, j.context_size);

        if (val != oldval) return true;
    }

    return false;
}

namespace
{
    void update_tag_alpha(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.color.a = (U8)((F32&)j.context * 255.0f + 0.5f);
    }

    void update_tag_reset_alpha(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.color.a = ctb.font.color.a;
    }

    void parse_tag_alpha(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += (1 / 255.0f) * tb.font.color.a;
            break;
        case '*':
            v *= (1 / 255.0f) * tb.font.color.a;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_alpha, update_tag_alpha };
        a.cb = &cb;
    }

    void reset_tag_alpha(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                         const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_alpha,
                                               update_tag_reset_alpha };
        a.cb = &cb;
    }

    void update_tag_red(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.color.r = (U8)((F32&)j.context * 255.0f + 0.5f);
    }

    void update_tag_reset_red(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.color.r = ctb.font.color.r;
    }

    void parse_tag_red(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += (1 / 255.0f) * tb.font.color.r;
            break;
        case '*':
            v *= (1 / 255.0f) * tb.font.color.r;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_red, update_tag_red };
        a.cb = &cb;
    }

    void reset_tag_red(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                       const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_red, update_tag_reset_red };
        a.cb = &cb;
    }

    void update_tag_green(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.color.g = (U8)((F32&)j.context * 255.0f + 0.5f);
    }

    void update_tag_reset_green(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.color.g = ctb.font.color.g;
    }

    void parse_tag_green(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += (1 / 255.0f) * tb.font.color.g;
            break;
        case '*':
            v *= (1 / 255.0f) * tb.font.color.g;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_green, update_tag_green };
        a.cb = &cb;
    }

    void reset_tag_green(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                         const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_green,
                                               update_tag_reset_green };
        a.cb = &cb;
    }

    void update_tag_blue(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.color.b = (U8)((F32&)j.context * 255.0f + 0.5f);
    }

    void update_tag_reset_blue(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.color.b = ctb.font.color.b;
    }

    void parse_tag_blue(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += (1 / 255.0f) * tb.font.color.b;
            break;
        case '*':
            v *= (1 / 255.0f) * tb.font.color.b;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_blue, update_tag_blue };
        a.cb = &cb;
    }

    void reset_tag_blue(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                        const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_blue, update_tag_reset_blue };
        a.cb = &cb;
    }

    void update_tag_width(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.width = (F32&)j.context;
    }

    void update_tag_reset_width(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.width = ctb.font.width;
    }

    void parse_tag_width(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.font.width;
            break;
        case '*':
            v *= tb.font.width;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_width, update_tag_width };
        a.cb = &cb;
    }

    void reset_tag_width(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                         const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_width,
                                               update_tag_reset_width };
        a.cb = &cb;
    }

    void update_tag_height(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.height = (F32&)j.context;
    }

    void update_tag_reset_height(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.height = ctb.font.height;
    }

    void parse_tag_height(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.font.height;
            break;
        case '*':
            v *= tb.font.height;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_height, update_tag_height };
        a.cb = &cb;
    }

    void reset_tag_height(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                          const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_height,
                                               update_tag_reset_height };
        a.cb = &cb;
    }

    void update_tag_left_indent(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.left_indent = (F32&)j.context;
    }

    void update_tag_reset_left_indent(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.left_indent = ctb.left_indent;
    }

    void parse_tag_left_indent(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.left_indent;
            break;
        case '*':
            v *= tb.left_indent;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_left_indent, update_tag_left_indent };
        a.cb = &cb;
    }

    void reset_tag_left_indent(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                               const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_left_indent, NULL };
        a.cb = &cb;
    }

    void update_tag_right_indent(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.right_indent = (F32&)j.context;
    }

    void update_tag_reset_right_indent(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.right_indent = ctb.right_indent;
    }

    void parse_tag_right_indent(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.right_indent;
            break;
        case '*':
            v *= tb.right_indent;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_right_indent, update_tag_right_indent };
        a.cb = &cb;
    }

    void reset_tag_right_indent(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                                const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_right_indent, NULL };
        a.cb = &cb;
    }

    void update_tag_tab_stop(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.tab_stop = (F32&)j.context;
    }

    void update_tag_reset_tab_stop(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.tab_stop = ctb.tab_stop;
    }

    void parse_tag_tab_stop(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.tab_stop;
            break;
        case '*':
            v *= tb.tab_stop;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_tab_stop, update_tag_tab_stop };
        a.cb = &cb;
    }

    void reset_tag_tab_stop(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                            const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_tab_stop, NULL };
        a.cb = &cb;
    }

    void update_tag_xspace(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.space = (F32&)j.context;
    }

    void update_tag_reset_xspace(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.space = ctb.font.space;
    }

    void parse_tag_xspace(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.font.space;
            break;
        case '*':
            v *= tb.font.space;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_xspace, update_tag_xspace };
        a.cb = &cb;
    }

    void reset_tag_xspace(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                          const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_xspace,
                                               update_tag_reset_xspace };
        a.cb = &cb;
    }

    void update_tag_yspace(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.line_space = (F32&)j.context;
    }

    void update_tag_reset_yspace(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.line_space = ctb.line_space;
    }

    void parse_tag_yspace(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        F32& v = (F32&)a.context;

        if (ti.value.size == 0 || ti.action.size == 0) return;

        v = xatof(ti.value.text);

        switch (ti.action.text[0]) {
        case '=':
            break;
        case '+':
            v += tb.line_space;
            break;
        case '*':
            v *= tb.line_space;
            break;
        default:
            return;
        }

        if (v < 0.0f) v = 0.0f;
        else if (v > 1.0f) v = 1.0f;

        static xtextbox::callback cb = { NULL, update_tag_yspace, update_tag_yspace };
        a.cb = &cb;
    }

    void reset_tag_yspace(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                          const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_yspace, NULL };
        a.cb = &cb;
    }

    void update_tag_reset_all(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb = ctb;
    }

    void reset_tag_all(xtextbox::jot& j, const xtextbox&, const xtextbox&,
                       const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_all, update_tag_reset_all };
        j.cb = &cb;
    }

    void update_tag_color(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.color = (iColor_tag&)j.context;
    }

    void update_tag_reset_color(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.color = ctb.font.color;
    }

    void parse_tag_color(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        iColor_tag& color = (iColor_tag&)a.context;

        if (ti.value.size < 6 || ti.action.size == 0) return;

        U32 v = atox(ti.value);
        if (ti.value.size < 8) {
            v = (v & 0xFF000000) | (tb.font.color.a << 24);
        }

        switch (ti.action.text[0]) {
        case '=':
            color.a = (v & 0xFF000000) >> 24;
            color.r = (v & 0x00FF0000) >> 16;
            color.g = (v & 0x0000FF00) >> 8;
            color.b = (v & 0x000000FF) >> 0;
            break;
        case '+':
            color.a = MIN(255, ((v & 0xFF000000) >> 24) + tb.font.color.a);
            color.r = MIN(255, ((v & 0x00FF0000) >> 16) + tb.font.color.r);
            color.g = MIN(255, ((v & 0x0000FF00) >> 8) + tb.font.color.g);
            color.b = MIN(255, ((v & 0x000000FF) >> 0) + tb.font.color.b);
            break;
        case '*':
            color.a = ((v & 0xFF000000) >> 24) * tb.font.color.a / 255;
            color.r = ((v & 0x00FF0000) >> 16) * tb.font.color.r / 255;
            color.g = ((v & 0x0000FF00) >> 8) * tb.font.color.g / 255;
            color.b = ((v & 0x000000FF) >> 0) * tb.font.color.b / 255;
            break;
        default:
            return;
        }

        static xtextbox::callback cb = { NULL, update_tag_color, update_tag_color };
        a.cb = &cb;
    }

    void reset_tag_color(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                         const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_color,
                                               update_tag_reset_color };
        a.cb = &cb;
    }

    void update_tag_font(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.font.id = (U32&)j.context;
    }

    void update_tag_reset_font(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.font.id = ctb.font.id;
    }

    void parse_tag_font(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        if (ti.action.size < 1 || ti.action.text[0] != '=' || ti.value.size == 0) return;

        U32& id = (U32&)a.context;
        id = atoi(ti.value.text);
        if (id < active_fonts_size) {
            static xtextbox::callback cb = { NULL, update_tag_font, update_tag_font };
            a.cb = &cb;
        }
    }

    void reset_tag_font(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                        const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_font, update_tag_reset_font };
        a.cb = &cb;
    }

    void update_tag_wrap(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.flags = (tb.flags & ~WRAP_MASK) | ((U32&)j.context & WRAP_MASK);
    }

    void update_tag_reset_wrap(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.flags = (tb.flags & ~WRAP_MASK) | (ctb.flags & WRAP_MASK);
    }

    void parse_tag_wrap(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        if (ti.action.size < 1 || ti.action.text[0] != '=' || ti.value.size != 4) return;

        U32& flags = (U32&)a.context;
        if (imemcmp(ti.value.text, "word", 4) == 0) {
            flags = 0;
        }
        else if (imemcmp(ti.value.text, "char", 4) == 0) {
            flags = 0x10;
        }
        else if (imemcmp(ti.value.text, "none", 4) == 0) {
            flags = 0x20;
        }
        else {
            return;
        }

        static xtextbox::callback cb = { NULL, update_tag_wrap, update_tag_wrap };
        a.cb = &cb;
    }

    void reset_tag_wrap(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                        const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, update_tag_reset_wrap, NULL };
        a.cb = &cb;
    }

    void update_tag_xjustify(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.flags = (tb.flags & ~XJUSTIFY_MASK) | ((U32&)j.context & XJUSTIFY_MASK);
    }

    void update_tag_reset_xjustify(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.flags = (tb.flags & ~XJUSTIFY_MASK) | (ctb.flags & XJUSTIFY_MASK);
    }

    void parse_tag_xjustify(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        if (ti.action.size < 1 || ti.action.text[0] != '=') return;

        U32& flags = (U32&)a.context;
        if (icompare(ti.value, substr::create("left", 4)) == 0) {
            flags = 0;
        }
        else if (icompare(ti.value, substr::create("center", 6)) == 0) {
            flags = 0x2;
        }
        else if (icompare(ti.value, substr::create("right", 5)) == 0) {
            flags = 0x1;
        }
        else {
            return;
        }

        static xtextbox::callback cb = { NULL, update_tag_xjustify, update_tag_xjustify };
        a.cb = &cb;
    }

    void reset_tag_xjustify(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                            const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, NULL, update_tag_reset_xjustify };
        a.cb = &cb;
    }

    void update_tag_yjustify(const xtextbox::jot& j, xtextbox& tb, const xtextbox&)
    {
        tb.flags = (tb.flags & ~YJUSTIFY_MASK) | ((U32&)j.context & YJUSTIFY_MASK);
    }

    void update_tag_reset_yjustify(const xtextbox::jot&, xtextbox& tb, const xtextbox& ctb)
    {
        tb.flags = (tb.flags & ~YJUSTIFY_MASK) | (ctb.flags & YJUSTIFY_MASK);
    }

    void parse_tag_yjustify(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        if (ti.action.size < 1 || ti.action.text[0] != '=') return;

        U32& flags = (U32&)a.context;
        if (icompare(ti.value, substr::create("top", 4)) == 0) { // BUG: "top" is 3 chars, not 4
            flags = 0;
        }
        else if (icompare(ti.value, substr::create("center", 4)) == 0) { // BUG: "center" is 6 chars, not 4
            flags = 0x8;
        }
        else if (icompare(ti.value, substr::create("bottom", 4)) == 0) { // BUG: "bottom" is 6 chars, not 4
            flags = 0x4;
        }
        else {
            return;
        }

        static xtextbox::callback cb = { NULL, update_tag_yjustify, update_tag_yjustify };
        a.cb = &cb;
    }

    void reset_tag_yjustify(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                            const xtextbox::split_tag&)
    {
        static const xtextbox::callback cb = { NULL, NULL, update_tag_reset_yjustify };
        a.cb = &cb;
    }

    void parse_tag_open_curly(xtextbox::jot& a, const xtextbox& tb, const xtextbox&,
                              const xtextbox::split_tag& ti)
    {
        a.s.text = ti.tag.text;
        a.s.size = 1;

        unsigned char c = a.s.text[0];

        a.reset_flags();
        a.bounds = tb.font.bounds(c);
        a.cb = tb.cb;
        a.context = NULL;
    }

    void parse_tag_newline(xtextbox::jot& a, const xtextbox& tb, const xtextbox&,
                           const xtextbox::split_tag&)
    {
        a.flag.line_break = true;
        a.flag.ethereal = false;
        a.bounds = tb.font.bounds('\n');
    }

    void parse_tag_tab(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                       const xtextbox::split_tag&)
    {
        a.flag.tab = true;
    }

    void parse_tag_word_break(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                              const xtextbox::split_tag&)
    {
        a.flag.word_break = true;
    }

    void parse_tag_page_break(xtextbox::jot& a, const xtextbox&, const xtextbox&,
                              const xtextbox::split_tag&)
    {
        a.flag.page_break = true;
    }

    struct model_tag_context
    {
        xModelInstance* model;
        xVec3 rot;
        basic_rect<F32> dst;
        xSphere o;
    };

    void render_tag_model(const xtextbox::jot& j, const xtextbox& tb, F32 x, F32 y)
    {
        model_tag_context& mtc = *(model_tag_context*)j.context;

        basic_rect<F32> dst = mtc.dst;
        dst.move(x, y);

        xVec3 from = { 0, 0, 1.0f };
        xVec3 to = { 0, 0, -0.001f };

        xMat4x3 frame;

        xMat3x3Euler(&frame, &mtc.rot);

        F32 scale = 1.001f * ((mtc.o.r <= 0.0f) ? 1.0f : 0.5f / mtc.o.r);

        frame.right *= scale;
        frame.up *= scale;
        frame.at *= scale;
        frame.pos = mtc.o.center;
        frame.flags = 0;

        xModelSetFrame(mtc.model, &frame);
        // Ternary probably from some macro
        xModelSetMaterialAlpha(mtc.model, (true) ? tb.font.color.a : 0);

        tex_flush();

        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);

        xModelRender2D(*mtc.model, dst, from, to);

        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    }

    void parse_tag_model(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        load_model_args(def_model_args, ti.tag);
        if (!def_model_args.model) return;

        model_tag_context& mtc = *(model_tag_context*)a.context;
        mtc.model = def_model_args.model;
        mtc.rot = def_model_args.rot;
        mtc.rot *= DEG2RAD(1.0f);
        mtc.dst = def_model_args.dst;

        const xSphere* o = xModelGetLocalSBound(mtc.model);
        mtc.o = *o;
        mtc.dst.y -= def_model_args.off.y;

        a.bounds.assign(0.0f, -def_model_args.off.y, def_model_args.off.x, def_model_args.off.y);

        switch (def_model_args.scale) {
        case model_args::SCALE_SCREEN:
            break;
        case model_args::SCALE_SIZE:
            mtc.dst.scale(mtc.o.r);
            a.bounds.scale(mtc.o.r);
            break;
        default:
            mtc.dst.scale(tb.font.width, tb.font.height);
            a.bounds.scale(tb.font.width, tb.font.height);
            break;
        }

        a.reset_flags();
        a.context_size = sizeof(model_tag_context);

        static xtextbox::callback cb = { render_tag_model, NULL, NULL };
        a.cb = &cb;
    }

    void reset_tag_model(xtextbox::jot&, const xtextbox&, const xtextbox&,
                         const xtextbox::split_tag&)
    {
        reset_model_args(def_model_args);
    }

    struct tex_tag_context
    {
        RwRaster* raster;
        F32 rot;
        basic_rect<F32> src;
        basic_rect<F32> dst;
    };

    void render_tag_tex(const xtextbox::jot& j, const xtextbox& tb, F32 x, F32 y)
    {
        tex_tag_context& ttc = *(tex_tag_context*)j.context;

        set_tex_raster(ttc.raster);

        basic_rect<F32> dst = ttc.dst;
        dst.move(x, y);

        tex_render(ttc.src, dst, tb.font.clip, tb.font.color);
    }

    xVec2 get_texture_size(RwRaster& raster);

    void parse_tag_tex(xtextbox::jot& a, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti) NONMATCH("https://decomp.me/scratch/5tB1p")
    {
        load_tex_args(def_tex_args, ti.tag);
        if (!def_tex_args.raster) return;

        tex_tag_context& ttc = *(tex_tag_context*)a.context;
        ttc.raster = def_tex_args.raster;
        ttc.rot = def_tex_args.rot;
        ttc.src = def_tex_args.src;
        ttc.dst = def_tex_args.dst;
        ttc.dst.y -= def_tex_args.off.y;

        a.bounds.assign(0.0f, -def_tex_args.off.y, def_tex_args.off.x, def_tex_args.off.y);

        xVec2 scale;
        switch (def_tex_args.scale) {
        case tex_args::SCALE_SCREEN:
            scale.assign(1.0f, 1.0f);
            break;
        case tex_args::SCALE_SIZE:
            scale = get_texture_size(*ttc.raster);
            break;
        case tex_args::SCALE_FONT_WIDTH:
            scale = get_texture_size(*ttc.raster);
            scale.y *= tb.font.width / scale.x;
            scale.x = tb.font.width;
            break;
        case tex_args::SCALE_FONT_HEIGHT:
            scale = get_texture_size(*ttc.raster);
            scale.x *= tb.font.height / scale.y;
            scale.y = tb.font.height;
            break;
        case tex_args::SCALE_SCREEN_WIDTH:
            scale = get_texture_size(*ttc.raster);
            scale.y *= 1.0f / scale.x;
            scale.x = 1.0f;
            break;
        case tex_args::SCALE_SCREEN_HEIGHT:
            scale = get_texture_size(*ttc.raster);
            scale.x *= 1.0f / scale.y;
            scale.y = 1.0f;
            break;
        default:
            scale.assign(tb.font.width, tb.font.height);
            break;
        }

        ttc.dst.scale(scale.x, scale.y);

        a.bounds.scale(scale.x, scale.y);
        a.reset_flags();
        a.context_size = sizeof(tex_tag_context);

        static xtextbox::callback cb = { render_tag_tex, NULL, NULL };
        a.cb = &cb;
    }
} // namespace

namespace
{
    xVec2 get_texture_size(RwRaster& raster)
    {
        xVec2 ret = {
        (F32)RwRasterGetWidth(&raster) / FB_XRES,
        (F32)RwRasterGetHeight(&raster) / FB_YRES
        };
        return ret;
    }

    void reset_tag_tex(xtextbox::jot&, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
    {
        reset_tex_args(def_tex_args);
    }

    void parse_tag_insert(xtextbox::jot& j, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        j.reset_flags();
        j.flag.invisible = j.flag.ethereal = true;

        if (ti.action.size != 1 || ti.action.text[0] != ':' || ti.value.size == 0) return;

        U32 id = xStrHash(ti.value.text, ti.value.size);
        if (id == 0) return;

        xTextAsset* ta = (xTextAsset*)xSTFindAsset(id, NULL);
        if (!ta) return;

        j.context = (void*)xTextGetString(ta);
        j.context_size = ta->len;
        j.flag.insert = true;
    }

    void parse_tag_insert_hash(xtextbox::jot& j, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        j.reset_flags();
        j.flag.invisible = j.flag.ethereal = true;

        if (ti.action.size != 1 || ti.action.text[0] != ':' || ti.value.size == 0) return;

        U32 id = atox(ti.value);
        if (id == 0) return;

        xTextAsset* ta = (xTextAsset*)xSTFindAsset(id, NULL);
        if (!ta) return;

        j.context = (void*)xTextGetString(ta);
        j.context_size = ta->len;
        j.flag.insert = true;
    }

    void parse_tag_pop(xtextbox::jot&, const xtextbox&, const xtextbox&,
                       const xtextbox::split_tag& ti)
    {
        if (ti.action.size != 1 || ti.action.text[0] != ':' || ti.value.size == 0)
        {
            return;
        }
    }

    void parse_tag_timer(xtextbox::jot& j, const xtextbox& tb, const xtextbox& ctb, const xtextbox::split_tag& ti)
    {
        j.reset_flags();
        j.flag.invisible = j.flag.ethereal = true;

        if (ti.action.size != 1 || ti.action.text[0] != ':' || ti.value.size == 0) return;

        U32 id = xStrHash(ti.value.text, ti.value.size);
        if (id == 0) return;

        const xTimer* ta = (const xTimer*)zSceneFindObject(id);
        if (!ta) return;

        j.flag.insert = j.flag.dynamic = true;

        char buffer[64];
        U32 sec = (U32)ta->secondsLeft + 1;
        U32 mn = sec / 60;
        if (mn) {
            sprintf(buffer, "%d:%02d", mn, sec % 60);
        }
        else {
            sprintf(buffer, "%02d", sec);
        }

        char* text = (char*)j.context;
        sprintf(text, "%.*s", 15, buffer);
        j.context_size = 16;
    }

    // clang-format off
    xtextbox::tag_type format_tags_buffer[2][128] =
    {
        SUBSTR(""), parse_tag_open_curly, NULL, NULL,
        SUBSTR("a"), parse_tag_alpha, reset_tag_alpha, NULL,
        SUBSTR("all"), NULL, reset_tag_all, NULL,
        SUBSTR("alpha"), parse_tag_alpha, reset_tag_alpha, NULL,
        SUBSTR("b"), parse_tag_blue, reset_tag_blue, NULL,
        SUBSTR("blue"), parse_tag_blue, reset_tag_blue, NULL,
        SUBSTR("c"), parse_tag_color, reset_tag_color, NULL,
        SUBSTR("color"), parse_tag_color, reset_tag_color, NULL,
        SUBSTR("f"), parse_tag_font, reset_tag_font, NULL,
        SUBSTR("font"), parse_tag_font, reset_tag_font, NULL,
        SUBSTR("g"), parse_tag_green, reset_tag_green, NULL,
        SUBSTR("green"), parse_tag_green, reset_tag_green, NULL,
        SUBSTR("h"), parse_tag_height, reset_tag_height, NULL,
        SUBSTR("height"), parse_tag_height, reset_tag_height, NULL,
        SUBSTR("i"), parse_tag_insert, NULL, NULL,
        SUBSTR("ih"), parse_tag_insert_hash, NULL, NULL,
        SUBSTR("insert"), parse_tag_insert, NULL, NULL,
        SUBSTR("left_indent"), parse_tag_left_indent, reset_tag_left_indent, NULL,
        SUBSTR("li"), parse_tag_left_indent, reset_tag_left_indent, NULL,
        SUBSTR("model"), parse_tag_model, reset_tag_model, NULL,
        SUBSTR("n"), parse_tag_newline, NULL, NULL,
        SUBSTR("newline"), parse_tag_newline, NULL, NULL,
        SUBSTR("page_break"), parse_tag_page_break, NULL, NULL,
        SUBSTR("pb"), parse_tag_page_break, NULL, NULL,
        SUBSTR("pop"), parse_tag_pop, NULL, NULL,
        SUBSTR("r"), parse_tag_red, reset_tag_red, NULL,
        SUBSTR("red"), parse_tag_red, reset_tag_red, NULL,
        SUBSTR("ri"), parse_tag_right_indent, reset_tag_right_indent, NULL,
        SUBSTR("right_indent"), parse_tag_right_indent, reset_tag_right_indent, NULL,
        SUBSTR("t"), parse_tag_tab, NULL, NULL,
        SUBSTR("tab"), parse_tag_tab, NULL, NULL,
        SUBSTR("tab_stop"), parse_tag_tab_stop, reset_tag_tab_stop, NULL,
        SUBSTR("tex"), parse_tag_tex, reset_tag_tex, NULL,
        SUBSTR("timer"), parse_tag_timer, NULL, NULL,
        SUBSTR("ts"), parse_tag_tab_stop, reset_tag_tab_stop, NULL,
        SUBSTR("w"), parse_tag_width, reset_tag_width, NULL,
        SUBSTR("wb"), parse_tag_word_break, NULL, NULL,
        SUBSTR("width"), parse_tag_width, reset_tag_width, NULL,
        SUBSTR("word_break"), parse_tag_word_break, NULL, NULL,
        SUBSTR("wrap"), parse_tag_wrap, reset_tag_wrap, NULL,
        SUBSTR("xj"), parse_tag_xjustify, reset_tag_xjustify, NULL,
        SUBSTR("xjustify"), parse_tag_xjustify, reset_tag_xjustify, NULL,
        SUBSTR("xs"), parse_tag_xspace, reset_tag_xspace, NULL,
        SUBSTR("xspace"), parse_tag_xspace, reset_tag_xspace, NULL,
        SUBSTR("yj"), parse_tag_yjustify, reset_tag_yjustify, NULL,
        SUBSTR("yjustify"), parse_tag_yjustify, reset_tag_yjustify, NULL,
        SUBSTR("ys"), parse_tag_yspace, reset_tag_yspace, NULL,
        SUBSTR("yspace"), parse_tag_yspace, reset_tag_yspace, NULL
    };
    // clang-format on

    xtextbox::tag_type* format_tags = format_tags_buffer[0];
    U32 format_tags_size = 48;
} // namespace

void xtextbox::register_tags(const tag_type* t, size_t size)
{
    const tag_type *s1, *s2, *end1, *end2;

    end1 = (s1 = format_tags) + format_tags_size;
    s2 = t;
    end2 = t + size;

    tag_type* d = (s1 == format_tags_buffer[0]) ? format_tags_buffer[1] : format_tags_buffer[0];

    format_tags = d;

    while (s1 < end1 && s2 < end2)
    {
        S32 c = icompare(s1->name, s2->name);

        if (c < 0)
        {
            *d = *s1;
            s1++;
        }
        else if (c > 0)
        {
            *d = *s2;
            s2++;
        }
        else
        {
            *d = *s2;
            s1++;
            s2++;
        }

        d++;
    }

    while (s1 < end1)
    {
        *d = *s1;
        d++;
        s1++;
    }

    while (s2 < end2)
    {
        *d = *s2;
        d++;
        s2++;
    }

    format_tags_size = d - format_tags;
}

xtextbox::tag_type* xtextbox::find_format_tag(const substr& s, S32& index)
{
    S32 start = 0;
    S32 end = format_tags_size;

    while (start != end)
    {
        index = (start + end) / 2;

        tag_type& t = format_tags[index];
        S32 c = icompare(s, t.name);

        if (c < 0)
        {
            end = index;
        }
        else if (c > 0)
        {
            start = index + 1;
        }
        else
        {
            return &t;
        }
    }

    index = -1;
    return NULL;
}

namespace
{
    void set_rect_verts(RwIm2DVertex* verts, F32 x, F32 y, F32 w, F32 h, iColor_tag c, F32 rcz,
                        F32 nsz);
    void set_rect_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 z, iColor_tag c, F32 rcz);
} // namespace

void render_fill_rect(const basic_rect<F32>& bounds, iColor_tag color)
{
    if (bounds.empty()) return;

    F32 rcz, nsz;
    RwCamera* cam;
    RwIm2DVertex vert[4];

    cam = RwCameraGetCurrentCamera();
    rcz = 1.0f / RwCameraGetNearClipPlane(cam);
    nsz = RwIm2DGetNearScreenZ();

    xfont::set_render_state(NULL);

    basic_rect<F32> r = bounds;
    r.scale(FB_XRES, FB_YRES);

    set_rect_verts(vert, r.x, r.y, r.w, r.h, color, rcz, nsz);
    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vert, 4);

    xfont::restore_render_state();
}

namespace
{
    void set_rect_verts(RwIm2DVertex* verts, F32 x, F32 y, F32 w, F32 h, iColor_tag c, F32 rcz,
                        F32 nsz)
    {
        set_rect_vert(verts[0], x, y, nsz, c, rcz);
        set_rect_vert(verts[1], x, y + h, nsz, c, rcz);
        set_rect_vert(verts[2], x + w, y, nsz, c, rcz);
        set_rect_vert(verts[3], x + w, y + h, nsz, c, rcz);
    }

    void set_rect_vert(RwIm2DVertex& vert, F32 x, F32 y, F32 z, iColor_tag c, F32 rcz)
    {
        RwIm2DVertexSetScreenX(&vert, x);
        RwIm2DVertexSetScreenY(&vert, y);
        RwIm2DVertexSetScreenZ(&vert, z);
        RwIm2DVertexSetIntRGBA(&vert, c.r, c.g, c.b, c.a);
        RwIm2DVertexSetRecipCameraZ(&vert, rcz);
    }
} // namespace

//basic_rect<F32>& basic_rect<F32>::assign(F32 x, F32 y, F32 w, F32 h)
//{
//    this->x = x;
//    this->y = y;
//    this->w = w;
//    this->h = h;
//    return *this;
//}
 
xVec2& xVec2::assign(F32 x, F32 y)
{
    this->x = x;
    this->y = y;
    return *this;
}

substr substr::create(const char* text, size_t size)
{
    substr s = { text, size };
    return s;
}

size_t rskip_ws(substr& s)
{
    return rskip_ws(s.text, s.size);
}

size_t rskip_ws(const char*& text, size_t& size)
{
    while (size && is_ws(text[size - 1]))
    {
        size--;
    }

    return size;
}

bool is_ws(char c)
{
    return (c == ' ' || c == '\t' || c == '\n');
}

const char* find_char(const substr& s, char c)
{
    if (!s.text)
    {
        return NULL;
    }

    const char* text = s.text;
    S32 size = s.size;

    while (size > 0 && *text != '\0')
    {
        if (*text == c)
        {
            return text;
        }

        size--;
        text++;
    }

    return NULL;
}

const char* skip_ws(substr& s)
{
    return skip_ws(s.text, s.size);
}

const char* skip_ws(const char*& text, size_t& size)
{
    size_t i = 0;

    while (i < size && *text != '\0')
    {
        if (!is_ws(*text))
        {
            size -= i;
            break;
        }

        text++;
        i++;
    }

    return text;
}

size_t atox(const substr& s)
{
    size_t read_size;
    return atox(s, read_size);
}

size_t trim_ws(substr& s)
{
    return trim_ws(s.text, s.size);
}

size_t trim_ws(const char*& text, size_t& size)
{
    skip_ws(text, size);
    return rskip_ws(text, size);
}

xtextbox::tag_type* xtextbox::find_format_tag(const substr& s)
{
    S32 index;
    return find_format_tag(s, index);
}

size_t xtextbox::layout::jots_size() const
{
    return _jots_size;
}

void xtextbox::jot::intersect_flags(const jot& other)
{
    *(U16*)&flag &= *(U16*)&other.flag;
}

void xtextbox::jot::reset_flags()
{
    *(U16*)&flag = 0;
}

xSphere* xModelGetLocalSBound(xModelInstance* model)
{
    return (xSphere*)RpAtomicGetBoundingSphere(model->Data);
}
