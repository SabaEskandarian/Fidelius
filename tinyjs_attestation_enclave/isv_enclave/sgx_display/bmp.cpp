/*
 * Bitmap manipulation functions. See `bmp.h` for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>
#include "bmp.h"

/* Ignore the alpha byte when comparing colors?
FIXME: Not all functions that should respect IGNORE_ALPHA does so.
*/
#ifndef IGNORE_ALPHA
#  define IGNORE_ALPHA 1
#endif

/* AGBR DEFINED FOR SGX PROJECT */

/* Experimental ABGR color mode.
 * Normally colors are stored as 0xARGB, but Emscripten uses an
 * 0xABGR format on the canvas, so use -DABGR=1 in your compiler
 * flags if you need to use this mode. */
#ifndef ABGR
#  define ABGR 1
#endif

#if BM_LAST_ERROR
const char *bm_last_error = "";
#  define SET_ERROR(e) bm_last_error = e
#else
#  define SET_ERROR(e)
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#if !defined(WIN32) && 0
/* TODO: Use `alloca()` if it is available */
#define ALLOCA(x) alloca(x)
#define FREEA(x)
#else
#define ALLOCA(x) malloc(x)
#define FREEA(x) free(x)
#endif

/* TODO: C11 defines fopen_s(), strncpy_s(), etc.
At the moment, I only use them if WIN32 is defined.
See __STDC_LIB_EXT1__
*/
#if defined(WIN32) && defined(_MSC_VER)
#  define SAFE_C11
#endif

#define BM_BPP          4 /* Bytes per Pixel */
#define BM_BLOB_SIZE(B) (B->w * B->h * BM_BPP)
#define BM_ROW_SIZE(B)  (B->w * BM_BPP)

#define BM_GET(b, x, y) (*((unsigned int*)(b->data + (y) * BM_ROW_SIZE(b) + (x) * BM_BPP)))
#define BM_SET(b, x, y, c) *((unsigned int*)(b->data + (y) * BM_ROW_SIZE(b) + (x) * BM_BPP)) = (c)

#if !ABGR
#  define BM_SET_RGBA(BMP, X, Y, R, G, B, A) do { \
        int _p = ((Y) * BM_ROW_SIZE(BMP) + (X)*BM_BPP); \
        BMP->data[_p++] = B;\
        BMP->data[_p++] = G;\
        BMP->data[_p++] = R;\
        BMP->data[_p++] = A;\
    } while(0)
#  define BM_GETB(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 0])
#  define BM_GETG(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 1])
#  define BM_GETR(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 2])
#  define BM_GETA(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 3])
#  define SET_COLOR_RGB(bm, r, g, b) bm->color = 0xFF000000 | ((r) << 16) | ((g) << 8) | (b)
#else
#  define BM_SET_RGBA(BMP, X, Y, R, G, B, A) do { \
        int _p = ((Y) * BM_ROW_SIZE(BMP) + (X)*BM_BPP); \
        BMP->data[_p++] = R;\
        BMP->data[_p++] = G;\
        BMP->data[_p++] = B;\
        BMP->data[_p++] = A;\
    } while(0)
#  define BM_GETR(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 0])
#  define BM_GETG(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 1])
#  define BM_GETB(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 2])
#  define BM_GETA(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 3])
#  define SET_COLOR_RGB(bm, r, g, b) bm->color = 0xFF000000 | ((b) << 16) | ((g) << 8) | (r)
#endif

/* N=0 -> B, N=1 -> G, N=2 -> R, N=3 -> A */
#define BM_GETN(B,N,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + (N)])

Bitmap *bm_make_text (int w, int h, unsigned int col, unsigned int bg_col,
                      int bg_alpha, const char * text) {
    const int font_width = 6;
    const int font_height = 8; // Height of xbmf font

    /* Determines the space between top and bottom of text box */
    int y_margin = font_height / 2;
    int def_height = font_height + y_margin * 2;
    int def_width = (int) (def_height * w/h);
    Bitmap *b = bm_create (def_width, def_height);
    /* bg color */
    bm_set_color (b, bg_col);
    bm_set_alpha (b, bg_alpha);
    bm_fillrect(b, 0,0, def_width, def_height);
    /* pen color */
    bm_set_color(b, col);
    bm_set_alpha(b, 255);
    int t_width = bm_text_width (b, text);
    int t_height = bm_text_height (b, text);
    int start = t_width > def_width? def_width - t_width - font_width/2 : 0;
    bm_puts(b, start, (def_height - t_height)/2, text);
    Bitmap *ret = bm_resample(b, w, h);
    /* Resample creates a new bitmap */
    bm_free (b);
    return ret;
}

Bitmap *bm_create(int w, int h) {
    Bitmap *b = (Bitmap *) malloc(sizeof *b);
    if(!b) {
        SET_ERROR("out of memory");
        return NULL;
    }

    assert(w > 0);
    assert(h > 0);

    b->w = w;
    b->h = h;

    b->clip.x0 = 0;
    b->clip.y0 = 0;
    b->clip.x1 = w;
    b->clip.y1 = h;

    b->data = (unsigned char *) malloc(BM_BLOB_SIZE(b));
    if(!b->data) {
        SET_ERROR("out of memory");
        free(b);
        return NULL;
    }
    memset(b->data, 0x00, BM_BLOB_SIZE(b));

    b->font = NULL;
    bm_reset_font(b);

    bm_set_color(b, 0xFFFFFFFF);

    return b;
}

Bitmap *bm_copy(Bitmap *b) {
    Bitmap *out = bm_create(b->w, b->h);
    memcpy(out->data, b->data, BM_BLOB_SIZE(b));
    out->color = b->color;
    out->font = b->font;

    memcpy(&out->clip, &b->clip, sizeof b->clip);

    return out;
}

Bitmap *bm_crop(Bitmap *b, int x, int y, int w, int h) {
    Bitmap *o = bm_create(w, h);
    if(!o) return NULL;
    bm_blit(o, 0, 0, b, x, y, w, h);
    return o;
}

void bm_free(Bitmap *b) {
    if(!b) return;
    if(b->data) free(b->data);
    free(b);
}

Bitmap *bm_bind(int w, int h, unsigned char *data) {
    Bitmap *b = (Bitmap *) malloc(sizeof *b);
    if(!b) {
        SET_ERROR("out of memory");
        return NULL;
    }
    return bm_bind_static(b, data, w, h);
}


Bitmap *bm_bind_static(Bitmap *b, unsigned char *data, int w, int h) {
    b->w = w;
    b->h = h;

    b->clip.x0 = 0;
    b->clip.y0 = 0;
    b->clip.x1 = w;
    b->clip.y1 = h;

    b->data = data;

    b->font = NULL;
    bm_reset_font(b);

    bm_set_color(b, 0xFFFFFFFF);
    return b;
}

void bm_rebind(Bitmap *b, unsigned char *data) {
    b->data = data;
}

void bm_unbind(Bitmap *b) {
    if(!b) return;
    free(b);
}

void bm_flip_vertical(Bitmap *b) {
    int y;
    size_t s = BM_ROW_SIZE(b);
    unsigned char *trow = (unsigned char *) ALLOCA(s);
    if (!trow)
        return;
    for(y = 0; y < b->h/2; y++) {
        unsigned char *row1 = &b->data[y * s];
        unsigned char *row2 = &b->data[(b->h - y - 1) * s];
        memcpy(trow, row1, s);
        memcpy(row1, row2, s);
        memcpy(row2, trow, s);
    }
    FREEA(trow);
}

unsigned int bm_get(Bitmap *b, int x, int y) {
    unsigned int *p;
    assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
    p = (unsigned int*)(b->data + y * BM_ROW_SIZE(b) + x * BM_BPP);
    return *p;
}

void bm_set(Bitmap *b, int x, int y, unsigned int c) {
    unsigned int *p;
    assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
    p = (unsigned int*)(b->data + y * BM_ROW_SIZE(b) + x * BM_BPP);
    *p = c;
}

void bm_clip(Bitmap *b, int x0, int y0, int x1, int y1) {
    if(x0 > x1) {
        int t = x1;
        x1 = x0;
        x0 = t;
    }
    if(y0 > y1) {
        int t = y1;
        y1 = y0;
        y0 = t;
    }
    if(x0 < 0) x0 = 0;
    if(x1 > b->w) x1 = b->w;
    if(y0 < 0) y0 = 0;
    if(y1 > b->h) y1 = b->h;

    b->clip.x0 = x0;
    b->clip.y0 = y0;
    b->clip.x1 = x1;
    b->clip.y1 = y1;
}

void bm_unclip(Bitmap *b) {
    b->clip.x0 = 0;
    b->clip.y0 = 0;
    b->clip.x1 = b->w;
    b->clip.y1 = b->h;
}

void bm_blit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h) {
    int x,y, i, j;

    if(sx < 0) {
        int delta = -sx;
        sx = 0;
        dx += delta;
        w -= delta;
    }

    if(dx < dst->clip.x0) {
        int delta = dst->clip.x0 - dx;
        sx += delta;
        w -= delta;
        dx = dst->clip.x0;
    }

    if(sx + w > src->w) {
        int delta = sx + w - src->w;
        w -= delta;
    }

    if(dx + w > dst->clip.x1) {
        int delta = dx + w - dst->clip.x1;
        w -= delta;
    }

    if(sy < 0) {
        int delta = -sy;
        sy = 0;
        dy += delta;
        h -= delta;
    }

    if(dy < dst->clip.y0) {
        int delta = dst->clip.y0 - dy;
        sy += delta;
        h -= delta;
        dy = dst->clip.y0;
    }

    if(sy + h > src->h) {
        int delta = sy + h - src->h;
        h -= delta;
    }

    if(dy + h > dst->clip.y1) {
        int delta = dy + h - dst->clip.y1;
        h -= delta;
    }

    if(w <= 0 || h <= 0)
        return;
    if(dx >= dst->clip.x1 || dx + w < dst->clip.x0)
        return;
    if(dy >= dst->clip.y1 || dy + h < dst->clip.y0)
        return;
    if(sx >= src->w || sx + w < 0)
        return;
    if(sy >= src->h || sy + h < 0)
        return;

    if(sx + w > src->w) {
        int delta = sx + w - src->w;
        w -= delta;
    }

    if(sy + h > src->h) {
        int delta = sy + h - src->h;
        h -= delta;
    }

    assert(dx >= 0 && dx + w <= dst->clip.x1);
    assert(dy >= 0 && dy + h <= dst->clip.y1);
    assert(sx >= 0 && sx + w <= src->w);
    assert(sy >= 0 && sy + h <= src->h);

    j = sy;
    for(y = dy; y < dy + h; y++) {
        i = sx;
        for(x = dx; x < dx + w; x++) {
            unsigned int c = BM_GET(src, i, j);
            BM_SET(dst, x, y, c);
            i++;
        }
        j++;
    }
}

void bm_maskedblit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h) {
    int x,y, i, j;

    if(sx < 0) {
        int delta = -sx;
        sx = 0;
        dx += delta;
        w -= delta;
    }

    if(dx < dst->clip.x0) {
        int delta = dst->clip.x0 - dx;
        sx += delta;
        w -= delta;
        dx = dst->clip.x0;
    }

    if(sx + w > src->w) {
        int delta = sx + w - src->w;
        w -= delta;
    }

    if(dx + w > dst->clip.x1) {
        int delta = dx + w - dst->clip.x1;
        w -= delta;
    }

    if(sy < 0) {
        int delta = -sy;
        sy = 0;
        dy += delta;
        h -= delta;
    }

    if(dy < dst->clip.y0) {
        int delta = dst->clip.y0 - dy;
        sy += delta;
        h -= delta;
        dy = dst->clip.y0;
    }

    if(sy + h > src->h) {
        int delta = sy + h - src->h;
        h -= delta;
    }

    if(dy + h > dst->clip.y1) {
        int delta = dy + h - dst->clip.y1;
        h -= delta;
    }

    if(w <= 0 || h <= 0)
        return;
    if(dx >= dst->clip.x1 || dx + w < dst->clip.x0)
        return;
    if(dy >= dst->clip.y1 || dy + h < dst->clip.y0)
        return;
    if(sx >= src->w || sx + w < 0)
        return;
    if(sy >= src->h || sy + h < 0)
        return;

    if(sx + w > src->w) {
        int delta = sx + w - src->w;
        w -= delta;
    }

    if(sy + h > src->h) {
        int delta = sy + h - src->h;
        h -= delta;
    }

    assert(dx >= 0 && dx + w <= dst->clip.x1);
    assert(dy >= 0 && dy + h <= dst->clip.y1);
    assert(sx >= 0 && sx + w <= src->w);
    assert(sy >= 0 && sy + h <= src->h);

    j = sy;
    for(y = dy; y < dy + h; y++) {
        i = sx;
        for(x = dx; x < dx + w; x++) {
#if IGNORE_ALPHA
            int c = BM_GET(src, i, j) & 0x00FFFFFF;
            if(c != (src->color & 0x00FFFFFF))
                BM_SET(dst, x, y, c);
#else
            int c = BM_GET(src, i, j);
            if(c != src->color)
                BM_SET(dst, x, y, c);
#endif
            i++;
        }
        j++;
    }
}

void bm_blit_ex(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, int mask) {
    int x, y, ssx;
    int ynum = 0;
    int xnum = 0;
#if IGNORE_ALPHA
    unsigned int maskc = bm_get_color(src) & 0x00FFFFFF;
#else
    unsigned int maskc = bm_get_color(src);
#endif
    /*
    Uses Bresenham's algoritm to implement a simple scaling while blitting.
    See the article "Scaling Bitmaps with Bresenham" by Tim Kientzle in the
    October 1995 issue of C/C++ Users Journal

    Or see these links:
        http://www.drdobbs.com/image-scaling-with-bresenham/184405045
        http://www.compuphase.com/graphic/scale.htm
    */

    if(sw == dw && sh == dh) {
        /* Special cases, no scaling */
        if(mask) {
            bm_maskedblit(dst, dx, dy, src, sx, sy, dw, dh);
        } else {
            bm_blit(dst, dx, dy, src, sx, sy, dw, dh);
        }
        return;
    }

    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    /* Clip on the Y */
    y = dy;
    while(y < dst->clip.y0 || sy < 0) {
        ynum += sh;
        while(ynum > dh) {
            ynum -= dh;
            sy++;
        }
        y++;
    }

    if(dy >= dst->clip.y1 || dy + dh < dst->clip.y0)
        return;

    /* Clip on the X */
    x = dx;
    while(x < dst->clip.x0 || sx < 0) {
        xnum += sw;
        while(xnum > dw) {
            xnum -= dw;
            sx++;
            sw--;
        }
        x++;
    }
    dw -= (x - dx);
    dx = x;

    if(dx >= dst->clip.x1 || dx + dw < dst->clip.x0)
        return;

    ssx = sx; /* Save sx for the next row */
    for(; y < dy + dh; y++){
        if(sy >= src->h || y >= dst->clip.y1)
            break;
        xnum = 0;
        sx = ssx;

        assert(y >= dst->clip.y0 && sy >= 0);
        for(x = dx; x < dx + dw; x++) {
            int c;
            if(sx >= src->w || x >= dst->clip.x1)
                break;
            assert(x >= dst->clip.x0 && sx >= 0);
#if IGNORE_ALPHA
            c = BM_GET(src, sx, sy) & 0x00FFFFFF;
#else
            c = BM_GET(src, sx, sy);
#endif
            if(!mask || c != maskc)
                BM_SET(dst, x, y, c);

            xnum += sw;
            while(xnum > dw) {
                xnum -= dw;
                sx++;
            }
        }
        ynum += sh;
        while(ynum > dh) {
            ynum -= dh;
            sy++;
        }
    }
}

/*
Works the same as bm_blit_ex(), but calls the callback for each pixel
typedef int (*bm_blit_fun)(Bitmap *dst, int dx, int dy, int sx, int sy, void *data);
*/
void bm_blit_ex_fun(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, bm_blit_fun fun, void *data){
    int x, y, ssx;
    int ynum = 0;
    int xnum = 0;
    unsigned int maskc = bm_get_color(src) & 0xFFFFFF;

    if(!fun || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    /* Clip on the Y */
    y = dy;
    while(y < dst->clip.y0 || sy < 0) {
        ynum += sh;
        while(ynum > dh) {
            ynum -= dh;
            sy++;
        }
        y++;
    }

    if(dy >= dst->clip.y1 || dy + dh < dst->clip.y0)
        return;

    /* Clip on the X */
    x = dx;
    while(x < dst->clip.x0 || sx < 0) {
        xnum += sw;
        while(xnum > dw) {
            xnum -= dw;
            sx++;
            sw--;
        }
        x++;
    }
    dw -= (x - dx);
    dx = x;

    if(dx >= dst->clip.x1 || dx + dw < dst->clip.x0)
        return;

    ssx = sx; /* Save sx for the next row */
    for(; y < dy + dh; y++){
        if(sy >= src->h || y >= dst->clip.y1)
            break;
        xnum = 0;
        sx = ssx;

        assert(y >= dst->clip.y0 && sy >= 0);
        for(x = dx; x < dx + dw; x++) {
            if(sx >= src->w || x >= dst->clip.x1)
                break;
            if(!fun(dst, x, y, src, sx, sy, maskc, data))
                return;
            xnum += sw;
            while(xnum > dw) {
                xnum -= dw;
                sx++;
            }
        }
        ynum += sh;
        while(ynum > dh) {
            ynum -= dh;
            sy++;
        }
    }
}

void bm_rotate_blit(Bitmap *dst, int ox, int oy, Bitmap *src, int px, int py, double angle, double scale) {
    /*
    "Fast Bitmap Rotation and Scaling" By Steven Mortimer, Dr Dobbs' Journal, July 01, 2001
    http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337
    See also http://www.efg2.com/Lab/ImageProcessing/RotateScanline.htm
    */

#if IGNORE_ALPHA
    unsigned int maskc = bm_get_color(src) & 0x00FFFFFF;
#else
    unsigned int maskc = bm_get_color(src);
#endif
    int x,y;

    int minx = dst->clip.x1, miny = dst->clip.y1;
    int maxx = dst->clip.x0, maxy = dst->clip.y0;

    double sinAngle = sin(angle);
    double cosAngle = cos(angle);

    double dx, dy;
    /* Compute the position of where each corner on the source bitmap
    will be on the destination to get a bounding box for scanning */
    dx = -cosAngle * px * scale + sinAngle * py * scale + ox;
    dy = -sinAngle * px * scale - cosAngle * py * scale + oy;
    if(dx < minx) minx = (int)dx;
    if(dx > maxx) maxx = (int)dx;
    if(dy < miny) miny = (int)dy;
    if(dy > maxy) maxy = (int)dy;

    dx = cosAngle * (src->w - px) * scale + sinAngle * py * scale + ox;
    dy = sinAngle * (src->w - px) * scale - cosAngle * py * scale + oy;
    if(dx < minx) minx = (int)dx;
    if(dx > maxx) maxx = (int)dx;
    if(dy < miny) miny = (int)dy;
    if(dy > maxy) maxy = (int)dy;

    dx = cosAngle * (src->w - px) * scale - sinAngle * (src->h - py) * scale + ox;
    dy = sinAngle * (src->w - px) * scale + cosAngle * (src->h - py) * scale + oy;
    if(dx < minx) minx = (int)dx;
    if(dx > maxx) maxx = (int)dx;
    if(dy < miny) miny = (int)dy;
    if(dy > maxy) maxy = (int)dy;

    dx = -cosAngle * px * scale - sinAngle * (src->h - py) * scale + ox;
    dy = -sinAngle * px * scale + cosAngle * (src->h - py) * scale + oy;
    if(dx < minx) minx = (int)dx;
    if(dx > maxx) maxx = (int)dx;
    if(dy < miny) miny = (int)dy;
    if(dy > maxy) maxy = (int)dy;

    /* Clipping */
    if(minx < dst->clip.x0) minx = dst->clip.x0;
    if(maxx > dst->clip.x1 - 1) maxx = dst->clip.x1 - 1;
    if(miny < dst->clip.y0) miny = dst->clip.y0;
    if(maxy > dst->clip.y1 - 1) maxy = dst->clip.y1 - 1;

    double dvCol = cos(angle) / scale;
    double duCol = sin(angle) / scale;

    double duRow = dvCol;
    double dvRow = -duCol;

    double startu = px - (ox * dvCol + oy * duCol);
    double startv = py - (ox * dvRow + oy * duRow);

    double rowu = startu + miny * duCol;
    double rowv = startv + miny * dvCol;

    for(y = miny; y <= maxy; y++) {
        double u = rowu + minx * duRow;
        double v = rowv + minx * dvRow;
        for(x = minx; x <= maxx; x++) {
            if(u >= 0 && u < src->w && v >= 0 && v < src->h) {
#if IGNORE_ALPHA
                unsigned int c = BM_GET(src, (int)u, (int)v) & 0x00FFFFFF;
#else
                unsigned int c = BM_GET(src, (int)u, (int)v);
#endif
                if(c != maskc) BM_SET(dst, x, y, c);
            }
            u += duRow;
            v += dvRow;
        }
        rowu += duCol;
        rowv += dvCol;
    }
}

void bm_grayscale(Bitmap *b) {
    /* https://en.wikipedia.org/wiki/Grayscale */
    int x, y;
    for(y = 0; y < b->h; y++)
        for(x = 0; x < b->w; x++) {
            unsigned int c =  BM_GET(b, x, y);
            unsigned char R,G,B;
            bm_get_rgb(c, &R, &G, &B);
            c = (2126 * R + 7152 * G + 722 * B)/10000;
            BM_SET(b, x, y, bm_rgb(c, c, c));
        }
}

void bm_smooth(Bitmap *b) {
    Bitmap *tmp = bm_create(b->w, b->h);
    int x, y;

    /* http://prideout.net/archive/bloom/ */
    int kernel[] = {1,4,6,4,1};

    assert(b->clip.y0 < b->clip.y1);
    assert(b->clip.x0 < b->clip.x1);

    for(y = 0; y < b->h; y++)
        for(x = 0; x < b->w; x++) {
            int p, k, c = 0;
            float R = 0, G = 0, B = 0, A = 0;
            for(p = x-2, k = 0; p < x+2; p++, k++) {
                if(p < 0 || p >= b->w)
                    continue;
                R += kernel[k] * BM_GETR(b,p,y);
                G += kernel[k] * BM_GETG(b,p,y);
                B += kernel[k] * BM_GETB(b,p,y);
                A += kernel[k] * BM_GETA(b,p,y);
                c += kernel[k];
            }
            BM_SET_RGBA(tmp, x, y, (unsigned char)(R/c), (unsigned char)(G/c), (unsigned char)(B/c), (unsigned char)(A/c));
        }

    for(y = 0; y < b->h; y++)
        for(x = 0; x < b->w; x++) {
            int p, k, c = 0;
            float R = 0, G = 0, B = 0, A = 0;
            for(p = y-2, k = 0; p < y+2; p++, k++) {
                if(p < 0 || p >= b->h)
                    continue;
                R += kernel[k] * BM_GETR(tmp,x,p);
                G += kernel[k] * BM_GETG(tmp,x,p);
                B += kernel[k] * BM_GETB(tmp,x,p);
                A += kernel[k] * BM_GETA(tmp,x,p);
                c += kernel[k];
            }
            BM_SET_RGBA(tmp, x, y, (unsigned char)(R/c), (unsigned char)(G/c), (unsigned char)(B/c), (unsigned char)(A/c));
        }

    memcpy(b->data, tmp->data, b->w * b->h * 4);
    bm_free(tmp);
}

void bm_apply_kernel(Bitmap *b, int dim, float kernel[]) {
    Bitmap *tmp = bm_create(b->w, b->h);
    int x, y;
    int kf = dim >> 1;

    assert(b->clip.y0 < b->clip.y1);
    assert(b->clip.x0 < b->clip.x1);

    for(y = 0; y < b->h; y++) {
        for(x = 0; x < b->w; x++) {
            int p, q, u, v;
            float R = 0, G = 0, B = 0, A = 0, c = 0;
            for(p = x-kf, u = 0; p <= x+kf; p++, u++) {
                if(p < 0 || p >= b->w)
                    continue;
                for(q = y-kf, v = 0; q <= y+kf; q++, v++) {
                    if(q < 0 || q >= b->h)
                        continue;
                    R += kernel[u + v * dim] * BM_GETR(b,p,q);
                    G += kernel[u + v * dim] * BM_GETG(b,p,q);
                    B += kernel[u + v * dim] * BM_GETB(b,p,q);
                    A += kernel[u + v * dim] * BM_GETA(b,p,q);
                    c += kernel[u + v * dim];
                }
            }
            R /= c; if(R > 255) R = 255;if(R < 0) R = 0;
            G /= c; if(G > 255) G = 255;if(G < 0) G = 0;
            B /= c; if(B > 255) B = 255;if(B < 0) B = 0;
            A /= c; if(A > 255) A = 255;if(A < 0) A = 0;
            BM_SET_RGBA(tmp, x, y, (unsigned char)R, (unsigned char)G, (unsigned char)B, (unsigned char)A);
        }
    }

    memcpy(b->data, tmp->data, b->w * b->h * 4);
    bm_free(tmp);
}

void bm_swap_color(Bitmap *b, unsigned int src, unsigned int dest) {
    /* Why does this function exist again? */
    int x,y;
#if IGNORE_ALPHA
    src |= 0xFF000000; dest |= 0xFF000000;
#endif
    for(y = 0; y < b->h; y++)
        for(x = 0; x < b->w; x++) {
            if(BM_GET(b,x,y) == src) {
                BM_SET(b, x, y, dest);
            }
        }
}

/*
Image scaling functions:
 - bm_resample() : Uses the nearest neighbour
 - bm_resample_blin() : Uses bilinear interpolation.
 - bm_resample_bcub() : Uses bicubic interpolation.
Bilinear Interpolation is better suited for making an image larger.
Bicubic Interpolation is better suited for making an image smaller.
http://blog.codinghorror.com/better-image-resizing/
*/
Bitmap *bm_resample_into(const Bitmap *in, Bitmap *out) {
    int x, y;
    int nw = out->w, nh = out->h;
    for(y = 0; y < nh; y++)
        for(x = 0; x < nw; x++) {
            int sx = x * in->w/nw;
            int sy = y * in->h/nh;
            assert(sx < in->w && sy < in->h);
            BM_SET(out, x, y, BM_GET(in,sx,sy));
        }
    return out;
}

Bitmap *bm_resample(const Bitmap *in, int nw, int nh) {
    Bitmap *out = bm_create(nw, nh);
    if(!out)
        return NULL;
    return bm_resample_into(in, out);
}

/* http://rosettacode.org/wiki/Bilinear_interpolation */
static double lerp(double s, double e, double t) {
    return s + (e-s)*t;
}
static double blerp(double c00, double c10, double c01, double c11, double tx, double ty) {
    return lerp(
        lerp(c00, c10, tx),
        lerp(c01, c11, tx),
        ty);
}

Bitmap *bm_resample_blin_into(const Bitmap *in, Bitmap *out) {
    int x, y;
    int nw = out->w, nh = out->h;
    for(y = 0; y < nh; y++)
        for(x = 0; x < nw; x++) {
            int C[4], c;
            double gx = (double)x * in->w/(double)nw;
            int sx = (int)gx;
            double gy = (double)y * in->h/(double)nh;
            int sy = (int)gy;
            int dx = 1, dy = 1;
            assert(sx < in->w && sy < in->h);
            if(sx + 1 >= in->w){ sx=in->w-1; dx = 0; }
            if(sy + 1 >= in->h){ sy=in->h-1; dy = 0; }
            for(c = 0; c < 4; c++) {
                int p00 = BM_GETN(in,c,sx,sy);
                int p10 = BM_GETN(in,c,sx+dx,sy);
                int p01 = BM_GETN(in,c,sx,sy+dy);
                int p11 = BM_GETN(in,c,sx+dx,sy+dy);
                C[c] = (int)blerp(p00, p10, p01, p11, gx-sx, gy-sy);
            }
#if !ABGR
            BM_SET_RGBA(out, x, y, C[2], C[1], C[0], C[3]);
#else
            BM_SET_RGBA(out, x, y, C[0], C[1], C[2], C[3]);
#endif
        }
    return out;
}

Bitmap *bm_resample_blin(const Bitmap *in, int nw, int nh) {
    Bitmap *out = bm_create(nw, nh);
    if(!out)
        return NULL;
    return out = bm_resample_blin_into(in, out);
}

/*
http://www.codeproject.com/Articles/236394/Bi-Cubic-and-Bi-Linear-Interpolation-with-GLSL
except I ported the GLSL code to straight C
*/
static double triangular_fun(double b) {
    b = b * 1.5 / 2.0;
    if( -1.0 < b && b <= 0.0) {
        return b + 1.0;
    } else if(0.0 < b && b <= 1.0) {
        return 1.0 - b;
    }
    return 0;
}

Bitmap *bm_resample_bcub_into(const Bitmap *in, Bitmap *out) {
    int x, y;
    int nw = out->w, nh = out->h;

    for(y = 0; y < nh; y++)
    for(x = 0; x < nw; x++) {

        double sum[4] = {0.0, 0.0, 0.0, 0.0};
        double denom[4] = {0.0, 0.0, 0.0, 0.0};

        double a = (double)x * in->w/(double)nw;
        int sx = (int)a;
        double b = (double)y * in->h/(double)nh;
        int sy = (int)b;

        int m, n, c, C;
        for(m = -1; m < 3; m++ )
        for(n = -1; n < 3; n++) {
            double f = triangular_fun((double)sx - a);
            double f1 = triangular_fun(-((double)sy - b));
            for(c = 0; c < 4; c++) {
                int i = sx+m;
                int j = sy+n;
                if(i < 0) i = 0;
                if(i >= in->w) i = in->w - 1;
                if(j < 0) j = 0;
                if(j >= in->h) j = in->h - 1;
                C = BM_GETN(in, c, i, j);
                sum[c] = sum[c] + C * f1 * f;
                denom[c] = denom[c] + f1 * f;
            }
        }

#if !ABGR
        BM_SET_RGBA(out, x, y, (unsigned char)(sum[2]/denom[2]), (unsigned char)(sum[1]/denom[1]), (unsigned char)(sum[0]/denom[0]), (unsigned char)(sum[3]/denom[3]));
#else
        BM_SET_RGBA(out, x, y, sum[0]/denom[0], sum[1]/denom[1], sum[2]/denom[2], sum[3]/denom[3]);
#endif
    }
    return out;
}

Bitmap *bm_resample_bcub(const Bitmap *in, int nw, int nh) {
    Bitmap *out = bm_create(nw, nh);
    if(!out)
        return NULL;
    return bm_resample_bcub_into(in, out);
}

void bm_set_alpha(Bitmap *bm, int a) {
    if(a < 0) a = 0;
    if(a > 255) a = 255;
    bm->color = (bm->color & 0x00FFFFFF) | (a << 24);
}

/* Lookup table for bm_atoi()
 * This list is based on the HTML and X11 colors on the
 * Wikipedia's list of web colors:
 * http://en.wikipedia.org/wiki/Web_colors
 * I also felt a bit nostalgic for the EGA graphics from my earliest
 * computer memories, so I added the EGA colors (prefixed with "EGA") from here:
 * http://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter
 *
 * Keep the list sorted because a binary search is used.
 *
 * bm_atoi()'s text parameter is not case sensitive and spaces are
 * ignored, so for example "darkred" and "Dark Red" are equivalent.
 */
static const struct color_map_entry {
    const char *name;
    unsigned int color;
} color_map[] = {
    {"ALICEBLUE", 0xF0F8FF},
    {"ANTIQUEWHITE", 0xFAEBD7},
    {"AQUA", 0x00FFFF},
    {"AQUAMARINE", 0x7FFFD4},
    {"AZURE", 0xF0FFFF},
    {"BEIGE", 0xF5F5DC},
    {"BISQUE", 0xFFE4C4},
    {"BLACK", 0x000000},
    {"BLANCHEDALMOND", 0xFFEBCD},
    {"BLUE", 0x0000FF},
    {"BLUEVIOLET", 0x8A2BE2},
    {"BROWN", 0xA52A2A},
    {"BURLYWOOD", 0xDEB887},
    {"CADETBLUE", 0x5F9EA0},
    {"CHARTREUSE", 0x7FFF00},
    {"CHOCOLATE", 0xD2691E},
    {"CORAL", 0xFF7F50},
    {"CORNFLOWERBLUE", 0x6495ED},
    {"CORNSILK", 0xFFF8DC},
    {"CRIMSON", 0xDC143C},
    {"CYAN", 0x00FFFF},
    {"DARKBLUE", 0x00008B},
    {"DARKCYAN", 0x008B8B},
    {"DARKGOLDENROD", 0xB8860B},
    {"DARKGRAY", 0xA9A9A9},
    {"DARKGREEN", 0x006400},
    {"DARKKHAKI", 0xBDB76B},
    {"DARKMAGENTA", 0x8B008B},
    {"DARKOLIVEGREEN", 0x556B2F},
    {"DARKORANGE", 0xFF8C00},
    {"DARKORCHID", 0x9932CC},
    {"DARKRED", 0x8B0000},
    {"DARKSALMON", 0xE9967A},
    {"DARKSEAGREEN", 0x8FBC8F},
    {"DARKSLATEBLUE", 0x483D8B},
    {"DARKSLATEGRAY", 0x2F4F4F},
    {"DARKTURQUOISE", 0x00CED1},
    {"DARKVIOLET", 0x9400D3},
    {"DEEPPINK", 0xFF1493},
    {"DEEPSKYBLUE", 0x00BFFF},
    {"DIMGRAY", 0x696969},
    {"DODGERBLUE", 0x1E90FF},
    {"EGABLACK", 0x000000},
    {"EGABLUE", 0x0000AA},
    {"EGABRIGHTBLACK", 0x555555},
    {"EGABRIGHTBLUE", 0x5555FF},
    {"EGABRIGHTCYAN", 0x55FFFF},
    {"EGABRIGHTGREEN", 0x55FF55},
    {"EGABRIGHTMAGENTA", 0xFF55FF},
    {"EGABRIGHTRED", 0xFF5555},
    {"EGABRIGHTWHITE", 0xFFFFFF},
    {"EGABRIGHTYELLOW", 0xFFFF55},
    {"EGABROWN", 0xAA5500},
    {"EGACYAN", 0x00AAAA},
    {"EGADARKGRAY", 0x555555},
    {"EGAGREEN", 0x00AA00},
    {"EGALIGHTGRAY", 0xAAAAAA},
    {"EGAMAGENTA", 0xAA00AA},
    {"EGARED", 0xAA0000},
    {"EGAWHITE", 0xAAAAAA},
    {"FIREBRICK", 0xB22222},
    {"FLORALWHITE", 0xFFFAF0},
    {"FORESTGREEN", 0x228B22},
    {"FUCHSIA", 0xFF00FF},
    {"GAINSBORO", 0xDCDCDC},
    {"GHOSTWHITE", 0xF8F8FF},
    {"GOLD", 0xFFD700},
    {"GOLDENROD", 0xDAA520},
    {"GRAY", 0x808080},
    {"GREEN", 0x008000},
    {"GREENYELLOW", 0xADFF2F},
    {"HONEYDEW", 0xF0FFF0},
    {"HOTPINK", 0xFF69B4},
    {"INDIANRED", 0xCD5C5C},
    {"INDIGO", 0x4B0082},
    {"IVORY", 0xFFFFF0},
    {"KHAKI", 0xF0E68C},
    {"LAVENDER", 0xE6E6FA},
    {"LAVENDERBLUSH", 0xFFF0F5},
    {"LAWNGREEN", 0x7CFC00},
    {"LEMONCHIFFON", 0xFFFACD},
    {"LIGHTBLUE", 0xADD8E6},
    {"LIGHTCORAL", 0xF08080},
    {"LIGHTCYAN", 0xE0FFFF},
    {"LIGHTGOLDENRODYELLOW", 0xFAFAD2},
    {"LIGHTGRAY", 0xD3D3D3},
    {"LIGHTGREEN", 0x90EE90},
    {"LIGHTPINK", 0xFFB6C1},
    {"LIGHTSALMON", 0xFFA07A},
    {"LIGHTSEAGREEN", 0x20B2AA},
    {"LIGHTSKYBLUE", 0x87CEFA},
    {"LIGHTSLATEGRAY", 0x778899},
    {"LIGHTSTEELBLUE", 0xB0C4DE},
    {"LIGHTYELLOW", 0xFFFFE0},
    {"LIME", 0x00FF00},
    {"LIMEGREEN", 0x32CD32},
    {"LINEN", 0xFAF0E6},
    {"MAGENTA", 0xFF00FF},
    {"MAROON", 0x800000},
    {"MEDIUMAQUAMARINE", 0x66CDAA},
    {"MEDIUMBLUE", 0x0000CD},
    {"MEDIUMORCHID", 0xBA55D3},
    {"MEDIUMPURPLE", 0x9370DB},
    {"MEDIUMSEAGREEN", 0x3CB371},
    {"MEDIUMSLATEBLUE", 0x7B68EE},
    {"MEDIUMSPRINGGREEN", 0x00FA9A},
    {"MEDIUMTURQUOISE", 0x48D1CC},
    {"MEDIUMVIOLETRED", 0xC71585},
    {"MIDNIGHTBLUE", 0x191970},
    {"MINTCREAM", 0xF5FFFA},
    {"MISTYROSE", 0xFFE4E1},
    {"MOCCASIN", 0xFFE4B5},
    {"NAVAJOWHITE", 0xFFDEAD},
    {"NAVY", 0x000080},
    {"OLDLACE", 0xFDF5E6},
    {"OLIVE", 0x808000},
    {"OLIVEDRAB", 0x6B8E23},
    {"ORANGE", 0xFFA500},
    {"ORANGERED", 0xFF4500},
    {"ORCHID", 0xDA70D6},
    {"PALEGOLDENROD", 0xEEE8AA},
    {"PALEGREEN", 0x98FB98},
    {"PALETURQUOISE", 0xAFEEEE},
    {"PALEVIOLETRED", 0xDB7093},
    {"PAPAYAWHIP", 0xFFEFD5},
    {"PEACHPUFF", 0xFFDAB9},
    {"PERU", 0xCD853F},
    {"PINK", 0xFFC0CB},
    {"PLUM", 0xDDA0DD},
    {"POWDERBLUE", 0xB0E0E6},
    {"PURPLE", 0x800080},
    {"RED", 0xFF0000},
    {"ROSYBROWN", 0xBC8F8F},
    {"ROYALBLUE", 0x4169E1},
    {"SADDLEBROWN", 0x8B4513},
    {"SALMON", 0xFA8072},
    {"SANDYBROWN", 0xF4A460},
    {"SEAGREEN", 0x2E8B57},
    {"SEASHELL", 0xFFF5EE},
    {"SIENNA", 0xA0522D},
    {"SILVER", 0xC0C0C0},
    {"SKYBLUE", 0x87CEEB},
    {"SLATEBLUE", 0x6A5ACD},
    {"SLATEGRAY", 0x708090},
    {"SNOW", 0xFFFAFA},
    {"SPRINGGREEN", 0x00FF7F},
    {"STEELBLUE", 0x4682B4},
    {"TAN", 0xD2B48C},
    {"TEAL", 0x008080},
    {"THISTLE", 0xD8BFD8},
    {"TOMATO", 0xFF6347},
    {"TURQUOISE", 0x40E0D0},
    {"VIOLET", 0xEE82EE},
    {"WHEAT", 0xF5DEB3},
    {"WHITE", 0xFFFFFF},
    {"WHITESMOKE", 0xF5F5F5},
    {"YELLOW", 0xFFFF00},
    {"YELLOWGREEN", 0x9ACD32},
    {NULL, 0}
};

unsigned int bm_atoi(const char *text) {
    unsigned int col = 0;

    if(!text) return 0;

    while(isspace(text[0]))
        text++;

    if(tolower(text[0]) == 'r' && tolower(text[1]) == 'g' && tolower(text[2]) == 'b') {
        /* Color is given like RGB(r,g,b) or RGBA(r,g,b,a) */
        int i = 0,a = 0, c[4];
        text += 3;
        if(text[0] == 'a') {
            a = 1;
            text++;
        }
        if(text[0] != '(') return 0;
        do {
            text++;
            size_t len;
            char buf[10];
            while(isspace(text[0]))
                text++;
            len = strspn(text, "0123456789.");
            if(len >= sizeof buf)
                return 0;
#ifdef SAFE_C11
            strncpy_s(buf, sizeof buf, text, len);
#else
            strncpy(buf,text,len);
#endif
            buf[len] = '\0';
            text += len;

            if(text[0] == '%') {
                double p = atof(buf);
                c[i++] = (int)(p * 255 / 100);
                text++;
            } else {
                if(i == 3) {
                    /* alpha value is given as a value between 0.0 and 1.0 */
                    double p = atof(buf);
                    c[i++] = (int)(p * 255);
                } else {
                    c[i++] = atoi(buf);
                }
            }
            while(isspace(text[0]))
                text++;

        } while(text[0] == ',' && i < 4);

        if(text[0] != ')' || i != (a ? 4 : 3))
            return 0;

        if(a)
            return bm_rgba(c[0], c[1], c[2], c[3]);
        else
            return bm_rgb(c[0], c[1], c[2]);
    } else if(tolower(text[0]) == 'h' && tolower(text[1]) == 's' && tolower(text[2]) == 'l') {
        /* Color is given like HSL(h,s,l) or HSLA(h,s,l,a) */
        int i = 0,a = 0;
        double c[4];
        text += 3;
        if(text[0] == 'a') {
            a = 1;
            text++;
        }
        if(text[0] != '(') return 0;
        do {
            text++;
            size_t len;
            char buf[10];
            while(isspace(text[0]))
                text++;
            len = strspn(text, "0123456789.");
            if(len >= sizeof buf)
                return 0;
#ifdef SAFE_C11
            strncpy_s(buf, sizeof buf, text, len);
#else
            strncpy(buf, text, len);
#endif
            buf[len] = '\0';
            text += len;

            c[i] = atof(buf);
            if(i == 1 || i == 2) {
                if(text[0] == '%')
                    text++;
            }
            i++;

            while(isspace(text[0]))
                text++;

        } while(text[0] == ',' && i < 4);

        if(text[0] != ')' || i != (a ? 4 : 3))
            return 0;

        if(a)
            return bm_hsla(c[0], c[1], c[2], c[3] * 100);
        else
            return bm_hsl(c[0], c[1], c[2]);

    } else if(isalpha(text[0])) {
        const char *q, *p;

        int min = 0, max = ((sizeof color_map)/(sizeof color_map[0])) - 1;
        while(min <= max) {
            int i = (max + min) >> 1, r;

            p = text;
            q = color_map[i].name;

            /* Hacky case insensitive strcmp() that ignores spaces in p */
            while(*p) {
                if(*p == ' ') p++;
                else {
                    if(tolower(*p) != tolower(*q))
                        break;
                    p++; q++;
                }
            }
            r = tolower(*p) - tolower(*q);

            if(r == 0)
                return bm_byte_order(color_map[i].color);
            else if(r < 0) {
                max = i - 1;
            } else {
                min = i + 1;
            }
        }
        /* Drop through: You may be dealing with a color like 'a6664c' */
    } else if(text[0] == '#') {
        text++;
        if(strlen(text) == 3) {
            /* Special case of #RGB that should be treated as #RRGGBB */
            while(text[0]) {
                int c = tolower(text[0]);
                if(c >= 'a' && c <= 'f') {
                    col = (col << 4) + (c - 'a' + 10);
                    col = (col << 4) + (c - 'a' + 10);
                } else {
                    col = (col << 4) + (c - '0');
                    col = (col << 4) + (c - '0');
                }
                text++;
            }
            return bm_byte_order(col);
        }
    } else if(text[0] == '0' && tolower(text[1]) == 'x') {
        text += 2;
    }

    if(tolower(text[0]) == 'g' && tolower(text[1]) == 'r'
        && (tolower(text[2]) == 'a' || tolower(text[2]) == 'e') && tolower(text[3]) == 'y'
        && isdigit(text[4])) {
        /* Color specified like "Gray50", see
            https://en.wikipedia.org/wiki/X11_color_names#Shades_of_gray
            I don't intend to support the other X11 variations. */
        col = atoi(text+4) * 255 / 100;
        return col | (col << 8) | (col << 16);
    }

    if(strlen(text) == 8) {
        /* CSS specifies colors as #RRGGBBAA, but I store it internally as ARGB (or ABGR) */
        while(isxdigit(text[0])) {
            int c = tolower(text[0]);
            if(c >= 'a' && c <= 'f') {
                col = (col << 4) + (c - 'a' + 10);
            } else {
                col = (col << 4) + (c - '0');
            }
            text++;
        }
        col = ((col & 0xFF) << 24) | ((col & 0xFFFFFF00) >> 8);
    } else if(strlen(text) == 6) {
        while(isxdigit(text[0])) {
            int c = tolower(text[0]);
            if(c >= 'a' && c <= 'f') {
                col = (col << 4) + (c - 'a' + 10);
            } else {
                col = (col << 4) + (c - '0');
            }
            text++;
        }
    } else {
        return 0;
    }
    return bm_byte_order(col);
}

unsigned int bm_rgb(unsigned char R, unsigned char G, unsigned char B) {
#if !ABGR
    return 0xFF000000 | ((R) << 16) | ((G) << 8) | (B);
#else
    return 0xFF000000 | ((B) << 16) | ((G) << 8) | (R);
#endif
}
unsigned int bm_rgba(unsigned char R, unsigned char G, unsigned char B, unsigned char A) {
#if !ABGR
    return ((A) << 24) | ((R) << 16) | ((G) << 8) | (B);
#else
    return ((A) << 24) | ((B) << 16) | ((G) << 8) | (R);
#endif
}

void bm_get_rgb(unsigned int col, unsigned char *R, unsigned char *G, unsigned char *B) {
    assert(R);
    assert(G);
    assert(B);
#if !ABGR
    *R = (col >> 16) & 0xFF;
    *G = (col >> 8) & 0xFF;
    *B = (col >> 0) & 0xFF;
#else
    *B = (col >> 16) & 0xFF;
    *G = (col >> 8) & 0xFF;
    *R = (col >> 0) & 0xFF;
#endif
}

unsigned int bm_hsl(double H, double S, double L) {
    /* The formula is based on the one on the wikipedia:
     * https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
     */
    double R = 0, G = 0, B = 0;
    double C, X, m;

    if(H > 0)
        H = fmod(H, 360.0);
    if(S < 0) S = 0;
    if(S > 100.0) S = 100.0;
    S /= 100.0;
    if(L < 0) L = 0;
    if(L > 100.0) L = 100;
    L /= 100.0;

    C = (1.0 - fabs(2.0 * L - 1.0)) * S;
    H = H / 60.0;
    X = C * (1.0 - fabs(fmod(H, 2.0) - 1.0));

    /* Treat H < 0 as H undefined */
    if(H >= 0 && H < 1) {
        R = C; G = X; B = 0;
    } else if (H < 2) {
        R = X; G = C; B = 0;
    } else if (H < 3) {
        R = 0; G = C; B = X;
    } else if (H < 4) {
        R = 0; G = X; B = C;
    } else if (H < 5) {
        R = X; G = 0; B = C;
    } else if (H < 6) {
        R = C; G = 0; B = X;
    }
    m = L - 0.5 * C;

    return bm_rgb((unsigned char)((R + m) * 255.0), (unsigned char)((G + m) * 255.0), (unsigned char)((B + m) * 255.0));
}

unsigned int bm_hsla(double H, double S, double L, double A) {
    unsigned int a = (unsigned int)(A * 255 / 100);
    unsigned int c = bm_hsl(H,S,L);
    return (c & 0x00FFFFFF) | ((a & 0xFF) << 24);
}

void bm_get_hsl(unsigned int col, double *H, double *S, double *L) {
    unsigned char R, G, B, M, m, C;
    assert(H);
    assert(S);
    assert(L);
    bm_get_rgb(col, &R, &G, &B);
    M = MAX(R, MAX(G, B));
    m = MIN(R, MIN(G, B));
    C = M - m;
    if(C == 0) {
        *H = 0;
    } else if (M == R) {
        *H = fmod((double)(G - B)/C, 6);
    } else if (M == G) {
        *H = (double)(B - R)/C + 2.0;
    } else if (M == B) {
        *H = (double)(R - G)/C + 4.0;
    }
    *H = fmod(*H * 60.0, 360);
    if(*H < 0) *H = 360.0 + *H;
    *L = 0.5 * (M + m) / 255.0;
    if(C == 0) {
        *S = 0;
    } else {
        *S = (double)C / (1.0 - fabs(2.0 * *L - 1.0)) / 255.0;
    }
    *L *= 100;
    *S *= 100;
}

void bm_set_color(Bitmap *bm, unsigned int col) {
    bm->color = col;
}

unsigned int bm_byte_order(unsigned int col) {
#if !ABGR
    return col;
#else
    return (col & 0xFF00FF00) | ((col >> 16) & 0x000000FF) | ((col & 0x000000FF) << 16);
#endif
}

unsigned int bm_get_color(Bitmap *bm) {
    return bm->color;
}

unsigned int bm_picker(Bitmap *bm, int x, int y) {
    if(x < 0 || x >= bm->w || y < 0 || y >= bm->h)
        return 0;
    bm->color = bm_get(bm, x, y);
    return bm->color;
}

unsigned int bm_lerp(unsigned int color1, unsigned int color2, double t) {
    int r1, g1, b1;
    int r2, g2, b2;
    int r3, g3, b3;

    if(t <= 0.0) return color1;
    if(t >= 1.0) return color2;

    r1 = (color1 >> 16) & 0xFF; g1 = (color1 >> 8) & 0xFF; b1 = (color1 >> 0) & 0xFF;
    r2 = (color2 >> 16) & 0xFF; g2 = (color2 >> 8) & 0xFF; b2 = (color2 >> 0) & 0xFF;

    r3 = (int)((r2 - r1) * t + r1);
    g3 = (int)((g2 - g1) * t + g1);
    b3 = (int)((b2 - b1) * t + b1);

    return (r3 << 16) | (g3 << 8) | (b3 << 0);
}

int bm_width(Bitmap *b) {
    return b->w;
}

int bm_height(Bitmap *b) {
    return b->h;
}

void bm_clear(Bitmap *b) {
    int i, j;
    for(j = 0; j < b->h; j++)
        for(i = 0; i < b->w; i++) {
            BM_SET(b, i, j, b->color);
        }
}

void bm_putpixel(Bitmap *b, int x, int y) {
    if(x < b->clip.x0 || x >= b->clip.x1 || y < b->clip.y0 || y >= b->clip.y1)
        return;
    BM_SET(b, x, y, b->color);
}

void bm_line(Bitmap *b, int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx, sy;
    int err, e2;

    if(dx < 0) dx = -dx;
    if(dy < 0) dy = -dy;

    if(x0 < x1)
        sx = 1;
    else
        sx = -1;
    if(y0 < y1)
        sy = 1;
    else
        sy = -1;

    err = dx - dy;

    for(;;) {
        /* Clipping can probably be more effective... */
        if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x0, y0, b->color);

        if(x0 == x1 && y0 == y1) break;

        e2 = 2 * err;

        if(e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if(e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void bm_rect(Bitmap *b, int x0, int y0, int x1, int y1) {
    bm_line(b, x0, y0, x1, y0);
    bm_line(b, x1, y0, x1, y1);
    bm_line(b, x1, y1, x0, y1);
    bm_line(b, x0, y1, x0, y0);
}

void bm_fillrect(Bitmap *b, int x0, int y0, int x1, int y1) {
    int x,y;
    if(x1 < x0) {
        x = x0;
        x0 = x1;
        x1 = x;
    }
    if(y1 < y0) {
        y = y0;
        y0 = y1;
        y1 = y;
    }
    for(y = MAX(y0, b->clip.y0); y < MIN(y1 + 1, b->clip.y1); y++) {
        for(x = MAX(x0, b->clip.x0); x < MIN(x1 + 1, b->clip.x1); x++) {
            assert(y >= 0 && y < b->h && x >= 0 && x < b->w);
            BM_SET(b, x, y, b->color);
        }
    }
}

void bm_dithrect(Bitmap *b, int x0, int y0, int x1, int y1) {
    int x,y;
    if(x1 < x0) {
        x = x0;
        x0 = x1;
        x1 = x;
    }
    if(y1 < y0) {
        y = y0;
        y0 = y1;
        y1 = y;
    }
    for(y = MAX(y0, b->clip.y0); y < MIN(y1 + 1, b->clip.y1); y++) {
        for(x = MAX(x0, b->clip.x0); x < MIN(x1 + 1, b->clip.x1); x++) {
            if((x + y) & 0x1) continue;
            assert(y >= 0 && y < b->h && x >= 0 && x < b->w);
            BM_SET(b, x, y, b->color);
        }
    }
}

void bm_circle(Bitmap *b, int x0, int y0, int r) {
    int x = -r;
    int y = 0;
    int err = 2 - 2 * r;
    do {
        int xp, yp;

        /* Lower Right */
        xp = x0 - x; yp = y0 + y;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        /* Lower Left */
        xp = x0 - y; yp = y0 - x;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        /* Upper Left */
        xp = x0 + x; yp = y0 - y;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        /* Upper Right */
        xp = x0 + y; yp = y0 + x;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        r = err;
        if(r > x) {
            x++;
            err += x*2 + 1;
        }
        if(r <= y) {
            y++;
            err += y * 2 + 1;
        }
    } while(x < 0);
}

void bm_fillcircle(Bitmap *b, int x0, int y0, int r) {
    int x = -r;
    int y = 0;
    int err = 2 - 2 * r;
    do {
        int i;
        for(i = x0 + x; i <= x0 - x; i++) {
            /* Maybe the clipping can be more effective... */
            int yp = y0 + y;
            if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
                BM_SET(b, i, yp, b->color);
            yp = y0 - y;
            if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
                BM_SET(b, i, yp, b->color);
        }

        r = err;
        if(r > x) {
            x++;
            err += x*2 + 1;
        }
        if(r <= y) {
            y++;
            err += y * 2 + 1;
        }
    } while(x < 0);
}

void bm_ellipse(Bitmap *b, int x0, int y0, int x1, int y1) {
    int a = abs(x1-x0), b0 = abs(y1-y0), b1 = b0 & 1;
    long dx = 4 * (1 - a) * b0 * b0,
        dy = 4*(b1 + 1) * a * a;
    long err = dx + dy + b1*a*a, e2;

    if(x0 > x1) { x0 = x1; x1 += a; }
    if(y0 > y1) { y0 = y1; }
    y0 += (b0+1)/2;
    y1 = y0 - b1;
    a *= 8*a;
    b1 = 8 * b0 * b0;

    do {
        if(x1 >= b->clip.x0 && x1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x1, y0, b->color);

        if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x0, y0, b->color);

        if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y1 >= b->clip.y0 && y1 < b->clip.y1)
            BM_SET(b, x0, y1, b->color);

        if(x1 >= b->clip.x0 && x1 < b->clip.x1 && y1 >= b->clip.y0 && y1 < b->clip.y1)
            BM_SET(b, x1, y1, b->color);

        e2 = 2 * err;
        if(e2 <= dy) {
            y0++; y1--; err += dy += a;
        }
        if(e2 >= dx || 2*err > dy) {
            x0++; x1--; err += dx += b1;
        }
    } while(x0 <= x1);

    while(y0 - y1 < b0) {
        if(x0 - 1 >= b->clip.x0 && x0 - 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x0 - 1, y0, b->color);

        if(x1 + 1 >= b->clip.x0 && x1 + 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x1 + 1, y0, b->color);
        y0++;

        if(x0 - 1 >= b->clip.x0 && x0 - 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x0 - 1, y1, b->color);

        if(x1 + 1 >= b->clip.x0 && x1 + 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
            BM_SET(b, x1 + 1, y1, b->color);
        y1--;
    }
}

void bm_roundrect(Bitmap *b, int x0, int y0, int x1, int y1, int r) {
    int x = -r;
    int y = 0;
    int err = 2 - 2 * r;
    int rad = r;

    bm_line(b, x0 + r, y0, x1 - r, y0);
    bm_line(b, x0, y0 + r, x0, y1 - r);
    bm_line(b, x0 + r, y1, x1 - r, y1);
    bm_line(b, x1, y0 + r, x1, y1 - r);

    do {
        int xp, yp;

        /* Lower Right */
        xp = x1 - x - rad; yp = y1 + y - rad;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        /* Lower Left */
        xp = x0 - y + rad; yp = y1 - x - rad;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        /* Upper Left */
        xp = x0 + x + rad; yp = y0 - y + rad;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        /* Upper Right */
        xp = x1 + y - rad; yp = y0 + x + rad;
        if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
            BM_SET(b, xp, yp, b->color);

        r = err;
        if(r > x) {
            x++;
            err += x*2 + 1;
        }
        if(r <= y) {
            y++;
            err += y * 2 + 1;
        }
    } while(x < 0);
}

void bm_fillroundrect(Bitmap *b, int x0, int y0, int x1, int y1, int r) {
    int x = -r;
    int y = 0;
    int err = 2 - 2 * r;
    int rad = r;
    do {
        int xp, xq, yp, i;

        xp = x0 + x + rad;
        xq = x1 - x - rad;
        for(i = xp; i <= xq; i++) {
            yp = y1 + y - rad;
            if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
                BM_SET(b, i, yp, b->color);
            yp = y0 - y + rad;
            if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
                BM_SET(b, i, yp, b->color);
        }

        r = err;
        if(r > x) {
            x++;
            err += x*2 + 1;
        }
        if(r <= y) {
            y++;
            err += y * 2 + 1;
        }
    } while(x < 0);

    for(y = MAX(y0 + rad + 1, b->clip.y0); y < MIN(y1 - rad, b->clip.y1); y++) {
        for(x = MAX(x0, b->clip.x0); x <= MIN(x1,b->clip.x1 - 1); x++) {
            assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
            BM_SET(b, x, y, b->color);
        }
    }
}

/* Bezier curve with 3 control points.
 * See http://devmag.org.za/2011/04/05/bzier-curves-a-tutorial/
 * I tried the more optimized version at
 * http://members.chello.at/~easyfilter/bresenham.html
 * but that one had some caveats.
 */
void bm_bezier3(Bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2) {
    int lx = x0, ly = y0, steps;

    /* Compute how far the point x1,y1 deviates from the line from x0,y0 to x2,y2.
     * I make the number of steps proportional to that deviation, but it is not
     * a perfect system.
     */
    double dx = x2 - x0, dy = y2 - y0;
    if(dx == 0 && dy == 0) {
      steps = 2;
    } else {
      /* https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line */
      double denom = sqrt(dx * dx + dy * dy);
      double dist = fabs((y2 - y0) * x1 - (x2 - x0) * y1 + x2 * y0 + y2 * x0)/denom;
      steps = (int)sqrt(dist);
      if(steps == 0) steps = 1;
    }

    double inc = 1.0/steps;
    double t = inc;

    do {
        dx = (1-t)*(1-t)*x0 + 2*(1-t)*t*x1 + t*t*x2;
        dy = (1-t)*(1-t)*y0 + 2*(1-t)*t*y1 + t*t*y2;
        bm_line(b, lx, ly, (int)dx, (int)dy);
        lx = (int)dx;
        ly = (int)dy;
        t += inc;
    } while(t < 1.0);
    bm_line(b, lx, ly, x2, y2);
}

void bm_poly(Bitmap *b, BmPoint points[], unsigned int n) {
    unsigned int i;
    if(n < 2) return;
    for(i = 0; i < n - 1; i++) {
        bm_line(b, points[i].x, points[i].y, points[i+1].x, points[i+1].y);
    }
    bm_line(b, points[0].x, points[0].y, points[i].x, points[i].y);
}

#define MAX_POLY_CORNERS 32

void bm_fillpoly(Bitmap *b, BmPoint points[], unsigned int n) {
    /* http://alienryderflex.com/polygon_fill/
    https://hackernoon.com/computer-graphics-scan-line-polygon-fill-algorithm-3cb47283df6

    You might also be interested in this article:
    http://nothings.org/gamedev/rasterize/
    */
    unsigned int i, j, c = bm_get_color(b);
    int x, y;
    if(n < 2)
        return;
    else if(n == 2) {
        bm_line(b, points[0].x, points[0].y, points[1].x, points[1].y);
        return;
    }

    int nodeX_static[MAX_POLY_CORNERS], *nodeX = nodeX_static;
    unsigned int nodes;

    if(n > MAX_POLY_CORNERS) {
        nodeX = (int*) calloc(n, sizeof *nodeX);
        if(!nodeX) return;
    }

    BmRect area = {b->w, b->h, 0, 0};
    for(i = 0; i < n; i++) {
        x = points[i].x;
        y = points[i].y;
        if(x < area.x0) area.x0 = x;
        if(y < area.y0) area.y0 = y;
        if(x > area.x1) area.x1 = x;
        if(y > area.y1) area.y1 = y;
    }
    if(area.x0 < b->clip.x0) area.x0 = b->clip.x0;
    if(area.y0 < b->clip.y0) area.y0 = b->clip.y0;
    if(area.x1 >= b->clip.x1) area.x1 = b->clip.x1 - 1;
    if(area.y1 >= b->clip.y1) area.y1 = b->clip.y1 - 1;

    for(y = area.y0; y <= area.y1; y++) {
        nodes = 0;
        j = n - 1;

        for(i = 0; i < n; i++) {
            if((points[i].y < y && points[j].y >= y)
                || (points[j].y < y && points[i].y >= y)) {
                nodeX[nodes++] = (int)(points[i].x + (double)(y - points[i].y) * (points[j].x - points[i].x) / (points[j].y - points[i].y));
            }
            j = i;
        }

        assert(nodes < n);
        if(nodes < 1) continue;

        i = 0;
        while(i < nodes - 1) {
            if(nodeX[i] > nodeX[i+1]) {
                int swap = nodeX[i];
                nodeX[i] = nodeX[i + 1];
                nodeX[i + 1] = swap;
                if(i) i--;
            } else {
                i++;
            }
        }

        for(i = 0; i < nodes; i += 2) {
            if(nodeX[i] >= area.x1)
                break;
            if(nodeX[i + 1] > area.x0) {
                if(nodeX[i] < area.x0)
                    nodeX[i] = area.x0;
                if(nodeX[i+1] > area.x1)
                    nodeX[i+1] = area.x1;

                for(x = nodeX[i]; x <= nodeX[i+1]; x++)
                    BM_SET(b, x, y, c);
            }
        }
    }

    if(nodeX != nodeX_static)
        free(nodeX);
}

void bm_fill(Bitmap *b, int x, int y) {
    BmPoint *queue, n;
    int qs = 0, /* queue size */
        mqs = 128; /* Max queue size */
    unsigned int sc, dc; /* Source and Destination colors */

    dc = b->color;
    b->color = BM_GET(b, x, y);
    sc = b->color;

    /* Don't fill if source == dest
     * It leads to major performance problems otherwise
     */
    if(sc == dc)
        return;

    queue = (BmPoint *) calloc(mqs, sizeof *queue);
    if(!queue)
        return;

    n.x = x; n.y = y;
    queue[qs++] = n;

    while(qs > 0) {
        BmPoint w,e, nn;
        int i;

        n = queue[--qs];
        w = n;
        e = n;

        if(BM_GET(b, n.x, n.y) != sc)
            continue;

        while(w.x > b->clip.x0) {
            if(BM_GET(b, w.x-1, w.y) != sc) {
                break;
            }
            w.x--;
        }
        while(e.x < b->clip.x1 - 1) {
            if(BM_GET(b, e.x+1, e.y) != sc) {
                break;
            }
            e.x++;
        }
        for(i = w.x; i <= e.x; i++) {
            assert(i >= 0 && i < b->w);
            BM_SET(b, i, w.y, dc);
            if(w.y > b->clip.y0) {
                if(BM_GET(b, i, w.y - 1) == sc) {
                    nn.x = i; nn.y = w.y - 1;
                    queue[qs++] = nn;
                    if(qs == mqs) {
                        mqs <<= 1;
                        void *tmp = realloc(queue, mqs * sizeof *queue);
                        if (!queue) {
                            free(queue);
                            return;
                        }
                        queue = (BmPoint *) tmp;
                    }
                }
            }
            if(w.y < b->clip.y1 - 1) {
                if(BM_GET(b, i, w.y + 1) == sc) {
                    nn.x = i; nn.y = w.y + 1;
                    queue[qs++] = nn;
                    if(qs == mqs) {
                        mqs <<= 1;
                        void *tmp = realloc(queue, mqs * sizeof *queue);
                        if (!tmp) {
                            free(queue);
                            return;
                        }
                        queue = (BmPoint *) tmp;
                    }
                }
            }
        }
    }
    free(queue);
    b->color = dc;
}

/* Squared distance between colors; so you don't need to get the root if you're
    only interested in comparing distances. */
static unsigned int col_dist_sq(unsigned int color1, unsigned int color2) {
    unsigned int r1, g1, b1;
    unsigned int r2, g2, b2;
    unsigned int dr, dg, db;
    r1 = (color1 >> 16) & 0xFF; g1 = (color1 >> 8) & 0xFF; b1 = (color1 >> 0) & 0xFF;
    r2 = (color2 >> 16) & 0xFF; g2 = (color2 >> 8) & 0xFF; b2 = (color2 >> 0) & 0xFF;
    dr = r1 - r2;
    dg = g1 - g2;
    db = b1 - b2;
    return dr * dr + dg * dg + db * db;
}

static unsigned int closest_color(unsigned int c, unsigned int palette[], size_t n) {
    unsigned int i, m = 0;
    unsigned int md = col_dist_sq(c, palette[m]);
    for(i = 1; i < n; i++) {
        unsigned int d = col_dist_sq(c, palette[i]);
        if(d < md) {
            md = d;
            m = i;
        }
    }
    return palette[m];
}

static void fs_add_factor(Bitmap *b, int x, int y, int er, int eg, int eb, int f) {
    int c, R, G, B;
    if(x < 0 || x >= b->w || y < 0 || y >= b->h)
        return;
    c = bm_get(b, x, y);

    R = ((c >> 16) & 0xFF) + ((f * er) >> 4);
    G = ((c >> 8) & 0xFF) + ((f * eg) >> 4);
    B = ((c >> 0) & 0xFF) + ((f * eb) >> 4);

    if(R > 255) R = 255;
    if(R < 0) R = 0;
    if(G > 255) G = 255;
    if(G < 0) G = 0;
    if(B > 255) B = 255;
    if(B < 0) B = 0;

    BM_SET_RGBA(b, x, y, R, G, B, 0);
}

void bm_reduce_palette(Bitmap *b, unsigned int palette[], unsigned int n) {
    /* Floyd-Steinberg (error-diffusion) dithering
        http://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering */
    int x, y;
    if(!b)
        return;
    for(y = 0; y < b->h; y++) {
        for(x = 0; x < b->w; x++) {
            unsigned int r1, g1, b1;
            unsigned int r2, g2, b2;
            unsigned int er, eg, eb;
            unsigned int newpixel, oldpixel = BM_GET(b, x, y);

            newpixel = closest_color(oldpixel, palette, n);

            bm_set(b, x, y, newpixel);

            r1 = (oldpixel >> 16) & 0xFF; g1 = (oldpixel >> 8) & 0xFF; b1 = (oldpixel >> 0) & 0xFF;
            r2 = (newpixel >> 16) & 0xFF; g2 = (newpixel >> 8) & 0xFF; b2 = (newpixel >> 0) & 0xFF;
            er = r1 - r2; eg = g1 - g2; eb = b1 - b2;

            fs_add_factor(b, x + 1, y    , er, eg, eb, 7);
            fs_add_factor(b, x - 1, y + 1, er, eg, eb, 3);
            fs_add_factor(b, x    , y + 1, er, eg, eb, 5);
            fs_add_factor(b, x + 1, y + 1, er, eg, eb, 1);
        }
    }
}

static int bayer4x4[16] = { /* (1/17) */
    1,  9,  3, 11,
    13, 5, 15,  7,
    4, 12,  2, 10,
    16, 8, 14,  6
};
static int bayer8x8[64] = { /*(1/65)*/
    1,  49, 13, 61,  4, 52, 16, 64,
    33, 17, 45, 29, 36, 20, 48, 32,
    9,  57,  5, 53, 12, 60,  8, 56,
    41, 25, 37, 21, 44, 28, 40, 24,
    3,  51, 15, 63,  2, 50, 14, 62,
    35, 19, 47, 31, 34, 18, 46, 30,
    11, 59,  7, 55, 10, 58,  6, 54,
    43, 27, 39, 23, 42, 26, 38, 22,
};
static void reduce_palette_bayer(Bitmap *b, unsigned int palette[], size_t n, int bayer[], int dim, int fac) {
    /* Ordered dithering: https://en.wikipedia.org/wiki/Ordered_dithering
    The resulting image may be of lower quality than you would get with
    Floyd-Steinberg, but it does have some advantages:
        * the repeating patterns compress better
        * it is better suited for line-art graphics
        * if you were to make a limited palette animation (e.g. animated GIF)
            subsequent frames would be less jittery than error-diffusion.
    */
    int x, y;
    int af = dim - 1; /* mod factor */
    int sub = (dim * dim) / 2 - 1; /* 7 if dim = 4, 31 if dim = 8 */
    if(!b)
        return;
    for(y = 0; y < b->h; y++) {
        for(x = 0; x < b->w; x++) {
            unsigned int R, G, B;
            unsigned int newpixel, oldpixel = BM_GET(b, x, y);

            R = (oldpixel >> 16) & 0xFF; G = (oldpixel >> 8) & 0xFF; B = (oldpixel >> 0) & 0xFF;

            /* The "- sub" below is because otherwise colors are only adjusted upwards,
                causing the resulting image to be brighter than the original.
                This seems to be the same problem this guy http://stackoverflow.com/q/4441388/115589
                ran into, but I can't find anyone else on the web solving it like I did. */
            int f = (bayer[(y & af) * dim + (x & af)] - sub);

            R += R * f / fac;
            if(R > 255) R = 255;
            else if(R < 0) R = 0;
            G += G * f / fac;
            if(G > 255) G = 255;
            else if(G < 0) G = 0;
            B += B * f / fac;
            if(B > 255) B = 255;
            else if(B < 0) B = 0;
            oldpixel = (R << 16) | (G << 8) | B;
            newpixel = closest_color(oldpixel, palette, n);
            BM_SET(b, x, y, newpixel);
        }
    }
}

void bm_reduce_palette_OD4(Bitmap *b, unsigned int palette[], unsigned int n) {
    reduce_palette_bayer(b, palette, n, bayer4x4, 4, 17);
}

void bm_reduce_palette_OD8(Bitmap *b, unsigned int palette[], unsigned int n) {
    reduce_palette_bayer(b, palette, n, bayer8x8, 8, 65);
}

void bm_set_font(Bitmap *b, BmFont *font) {
    b->font = font;
}

BmFont *bm_get_font(Bitmap *b) {
    return b->font;
}

int bm_text_width(Bitmap *b, const char *s) {
    int len = 0, max_len = 0;
    int glyph_width;

    if(!b->font || !b->font->width)
        return 0;

    glyph_width = b->font->width(b->font);
    while(*s) {
        if(*s == '\n') {
            if(len > max_len)
                max_len = len;
            len = 0;
        } else if(*s == '\t') {
            len+=4;
        } else if(isprint(*s)) {
            len++;
        }
        s++;
    }
    if(len > max_len)
        max_len = len;
    return max_len * glyph_width;
}

int bm_text_height(Bitmap *b, const char *s) {
    int height = 1;
    int glyph_height;
    if(!b->font || !b->font->height)
        return 0;
    glyph_height = b->font->height(b->font);
    while(*s) {
        if(*s == '\n') height++;
        s++;
    }
    return height * glyph_height;
}

int bm_putc(Bitmap *b, int x, int y, char c) {
    char text[2] = {c, 0};
    return bm_puts(b, x, y, text);
}

int bm_puts(Bitmap *b, int x, int y, const char *text) {
    if(!b->font || !b->font->puts)
        return 0;
    return b->font->puts(b, x, y, text);
}

int bm_printf(Bitmap *b, int x, int y, const char *fmt, ...) {
    char buffer[256];
    va_list arg;
    if(!b->font || !b->font->puts) return 0;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, arg);
    va_end(arg);
    return bm_puts(b, x, y, buffer);
}

void bm_free_font(BmFont *font) {
    if(font && font->dtor) {
        font->dtor(font);
    }
}

/** XBM FONT FUNCTIONS *********************************************************/

/* --- normal.xbm ------------------------------------------------------------ */
#define normal_width 128
#define normal_height 48
static unsigned char normal_bits[] = {
   0xff, 0xf7, 0xeb, 0xff, 0xf7, 0xff, 0xfb, 0xf7, 0xef, 0xfb, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xeb, 0xeb, 0xc3, 0xd9, 0xf5, 0xf7,
   0xf7, 0xf7, 0xd5, 0xf7, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xf7, 0xff, 0xc1,
   0xf5, 0xe9, 0xf5, 0xff, 0xfb, 0xef, 0xe3, 0xf7, 0xff, 0xff, 0xff, 0xef,
   0xff, 0xf7, 0xff, 0xeb, 0xe3, 0xf7, 0xfb, 0xff, 0xfb, 0xef, 0xf7, 0xc1,
   0xff, 0xc3, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xc1, 0xd7, 0xcb, 0xd5, 0xff,
   0xfb, 0xef, 0xe3, 0xf7, 0xff, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xeb,
   0xe1, 0xcd, 0xed, 0xff, 0xf7, 0xf7, 0xd5, 0xf7, 0xef, 0xff, 0xff, 0xfd,
   0xff, 0xf7, 0xff, 0xff, 0xf7, 0xff, 0xd3, 0xff, 0xef, 0xfb, 0xf7, 0xff,
   0xef, 0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xe3, 0xf7, 0xe3, 0xc1,
   0xef, 0xc1, 0xe3, 0xc1, 0xe3, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3,
   0xdd, 0xf3, 0xdd, 0xdf, 0xe7, 0xfd, 0xdd, 0xdf, 0xdd, 0xdd, 0xff, 0xff,
   0xef, 0xff, 0xfb, 0xdd, 0xcd, 0xf7, 0xdf, 0xef, 0xeb, 0xe1, 0xfd, 0xef,
   0xdd, 0xdd, 0xef, 0xef, 0xf7, 0xe3, 0xf7, 0xdf, 0xd5, 0xf7, 0xe7, 0xe7,
   0xed, 0xdf, 0xe1, 0xf7, 0xe3, 0xc3, 0xff, 0xff, 0xfb, 0xff, 0xef, 0xe7,
   0xd9, 0xf7, 0xfb, 0xdf, 0xc1, 0xdf, 0xdd, 0xfb, 0xdd, 0xdf, 0xff, 0xff,
   0xf7, 0xe3, 0xf7, 0xf7, 0xdd, 0xf7, 0xfd, 0xdd, 0xef, 0xdd, 0xdd, 0xfb,
   0xdd, 0xdd, 0xff, 0xef, 0xef, 0xff, 0xfb, 0xff, 0xe3, 0xe3, 0xc1, 0xe3,
   0xef, 0xe3, 0xe3, 0xfb, 0xe3, 0xe3, 0xef, 0xef, 0xff, 0xff, 0xff, 0xf7,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7,
   0xff, 0xff, 0xff, 0xff, 0xe3, 0xf7, 0xe1, 0xe3, 0xe1, 0xc1, 0xc1, 0xe3,
   0xdd, 0xe3, 0xcf, 0xdd, 0xfd, 0xdd, 0xdd, 0xe3, 0xdd, 0xeb, 0xdd, 0xdd,
   0xdd, 0xfd, 0xfd, 0xdd, 0xdd, 0xf7, 0xdf, 0xdd, 0xfd, 0xc9, 0xdd, 0xdd,
   0xc5, 0xdd, 0xdd, 0xfd, 0xdd, 0xfd, 0xfd, 0xfd, 0xdd, 0xf7, 0xdf, 0xed,
   0xfd, 0xd5, 0xd9, 0xdd, 0xd5, 0xdd, 0xe1, 0xfd, 0xdd, 0xe1, 0xe1, 0xc5,
   0xc1, 0xf7, 0xdf, 0xf1, 0xfd, 0xdd, 0xd5, 0xdd, 0xe5, 0xc1, 0xdd, 0xfd,
   0xdd, 0xfd, 0xfd, 0xdd, 0xdd, 0xf7, 0xdf, 0xed, 0xfd, 0xdd, 0xcd, 0xdd,
   0xfd, 0xdd, 0xdd, 0xdd, 0xdd, 0xfd, 0xfd, 0xdd, 0xdd, 0xf7, 0xdd, 0xdd,
   0xfd, 0xdd, 0xdd, 0xdd, 0xe3, 0xdd, 0xe1, 0xe3, 0xe1, 0xc1, 0xfd, 0xe3,
   0xdd, 0xe3, 0xe3, 0xdd, 0xc1, 0xdd, 0xdd, 0xe3, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xe1, 0xe3, 0xe1, 0xe3, 0xc1, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xc1, 0xe3,
   0xff, 0xe3, 0xf7, 0xff, 0xdd, 0xdd, 0xdd, 0xdd, 0xf7, 0xdd, 0xdd, 0xdd,
   0xdd, 0xdd, 0xdf, 0xfb, 0xfd, 0xef, 0xeb, 0xff, 0xdd, 0xdd, 0xdd, 0xfd,
   0xf7, 0xdd, 0xdd, 0xdd, 0xeb, 0xeb, 0xef, 0xfb, 0xfb, 0xef, 0xdd, 0xff,
   0xe1, 0xdd, 0xe1, 0xe3, 0xf7, 0xdd, 0xdd, 0xdd, 0xf7, 0xf7, 0xf7, 0xfb,
   0xf7, 0xef, 0xff, 0xff, 0xfd, 0xd5, 0xdd, 0xdf, 0xf7, 0xdd, 0xdd, 0xd5,
   0xeb, 0xf7, 0xfb, 0xfb, 0xef, 0xef, 0xff, 0xff, 0xfd, 0xed, 0xdd, 0xdd,
   0xf7, 0xdd, 0xeb, 0xc9, 0xdd, 0xf7, 0xfd, 0xfb, 0xdf, 0xef, 0xff, 0xff,
   0xfd, 0xd3, 0xdd, 0xe3, 0xf7, 0xc3, 0xf7, 0xdd, 0xdd, 0xf7, 0xc1, 0xe3,
   0xff, 0xe3, 0xff, 0xc1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xfd, 0xff,
   0xdf, 0xff, 0xe7, 0xff, 0xfd, 0xf7, 0xef, 0xfd, 0xf3, 0xff, 0xff, 0xff,
   0xe7, 0xff, 0xfd, 0xff, 0xdf, 0xff, 0xdb, 0xff, 0xfd, 0xff, 0xff, 0xfd,
   0xf7, 0xff, 0xff, 0xff, 0xef, 0xe3, 0xe1, 0xc3, 0xc3, 0xe3, 0xfb, 0xe3,
   0xfd, 0xf3, 0xe7, 0xed, 0xf7, 0xe9, 0xe5, 0xe3, 0xff, 0xdf, 0xdd, 0xfd,
   0xdd, 0xdd, 0xf1, 0xdd, 0xe1, 0xf7, 0xef, 0xf5, 0xf7, 0xd5, 0xd9, 0xdd,
   0xff, 0xc3, 0xdd, 0xfd, 0xdd, 0xc1, 0xfb, 0xdd, 0xdd, 0xf7, 0xef, 0xf9,
   0xf7, 0xdd, 0xdd, 0xdd, 0xff, 0xdd, 0xdd, 0xfd, 0xdd, 0xfd, 0xfb, 0xc3,
   0xdd, 0xf7, 0xef, 0xf5, 0xf7, 0xdd, 0xdd, 0xdd, 0xff, 0xc3, 0xe1, 0xc3,
   0xc3, 0xc3, 0xfb, 0xdf, 0xdd, 0xe3, 0xed, 0xed, 0xe3, 0xdd, 0xdd, 0xe3,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xf3, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xcf, 0xf7, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xf7, 0xf7, 0xd3, 0xff,
   0xe1, 0xc3, 0xe5, 0xe3, 0xe1, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xc1, 0xf7,
   0xf7, 0xf7, 0xe5, 0xff, 0xdd, 0xdd, 0xd9, 0xfd, 0xfb, 0xdd, 0xdd, 0xdd,
   0xeb, 0xdd, 0xef, 0xf9, 0xff, 0xcf, 0xff, 0xff, 0xdd, 0xdd, 0xfd, 0xf3,
   0xfb, 0xdd, 0xdd, 0xdd, 0xf7, 0xdd, 0xf7, 0xf7, 0xf7, 0xf7, 0xff, 0xff,
   0xe1, 0xc3, 0xfd, 0xef, 0xdb, 0xcd, 0xeb, 0xd5, 0xeb, 0xc3, 0xfb, 0xf7,
   0xf7, 0xf7, 0xff, 0xff, 0xfd, 0xdf, 0xfd, 0xf1, 0xe7, 0xd3, 0xf7, 0xeb,
   0xdd, 0xdf, 0xc1, 0xcf, 0xf7, 0xf9, 0xff, 0xff, 0xfd, 0xdf, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
/* --------------------------------------------------------------------------- */

#define XBM_FONT_WIDTH 128

typedef struct {
    const unsigned char *bits;
    int spacing;
} XbmFontInfo;

static void xbmf_putc(Bitmap *b, const unsigned char *xbm_bits, int x, int y, unsigned int col, char c) {
    int frow, fcol, byte;
    int i, j;

    if(c < 32 || c > 127)
        return;

    c -= 32;
    frow = (c >> 4);
    fcol = c & 0xF;
    byte = frow * XBM_FONT_WIDTH + fcol;

    for(j = 0; j < 8 && y + j < b->clip.y1; j++) {
        if(y + j >= b->clip.y0) {
            char bits = xbm_bits[byte];
            for(i = 0; i < 8 && x + i < b->clip.x1; i++) {
                if(x + i >= b->clip.x0 && !(bits & (1 << i))) {
                    BM_SET(b, x + i, y + j, col);
                }
            }
        }
        byte += XBM_FONT_WIDTH >> 3;
    }
}

static int xbmf_puts(Bitmap *b, int x, int y, const char *text) {
    XbmFontInfo *info;
    int xs = x, spacing;
    const unsigned char *bits;
    unsigned int col = bm_get_color(b);

    if(!b->font) return 0;

    info = (XbmFontInfo *) b->font->data;
    if(info) {
        spacing = info->spacing;
        bits = info->bits;
    } else {
        spacing = 6;
        bits = normal_bits;
    }

    while(text[0]) {
        if(text[0] == '\n') {
            y += 8;
            x = xs;
        } else if(text[0] == '\t') {
            /* I briefly toyed with the idea of having tabs line up,
             * but it doesn't really make sense because
             * this isn't exactly a character based terminal.
             */
            x += 4 * spacing;
        } else if(text[0] == '\r') {
            /* why would anyone find this useful? */
            x = xs;
        } else {
            xbmf_putc(b, bits, x, y, col, text[0]);
            x += spacing;
        }
        text++;
        if(y > b->h) {
            /* I used to check x >= b->w as well,
            but it doesn't take \n's into account */
            return 1;
        }
    }
    return 1;
}

static int xbmf_width(BmFont *font) {
    XbmFontInfo *info = (XbmFontInfo *) font->data;
    if(!info) return 6;
    return info->spacing;
}
static int xbmf_height(BmFont *font) {
    return 8;
}

static void xbmf_free(BmFont *font) {
    XbmFontInfo *info = (XbmFontInfo *) font->data;
    assert(!strcmp(font->type, "XBM"));
    free(info);
    free(font);
}

BmFont *bm_make_xbm_font(const unsigned char *bits, int spacing) {
    BmFont *font;
    XbmFontInfo *info;
    font = (BmFont *) malloc(sizeof *font);
    if(!font) {
        SET_ERROR("out of memory");
        return NULL;
    }
    info = (XbmFontInfo *) malloc(sizeof *info);
    if(!info) {
        SET_ERROR("out of memory");
        free(font);
        return NULL;
    }

    info->bits = bits;
    info->spacing = spacing;

    font->type = "XBM";
    font->puts = xbmf_puts;
    font->width = xbmf_width;
    font->height = xbmf_height;
    font->dtor = xbmf_free;
    font->data = info;

    return font;
}

void bm_reset_font(Bitmap *b) {
    static BmFont font = {"XBM",xbmf_puts,xbmf_width,xbmf_height,NULL,NULL};
    bm_set_font(b, &font);
}
