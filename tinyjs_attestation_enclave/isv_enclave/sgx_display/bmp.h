/**
 * Bitmap API (bmp.h/bmp.c)
 * ========================
 * ![toc-]
 *
 * Low-level routines to manipulate bitmap graphic objects in memory and files on disk.
 *
 * * It supports BMP, GIF, PCX and TGA files without any third party dependencies.
 * * PNG support is optional through [libpng][]. Use `-DUSEPNG` when compiling.
 * * JPG support is optional through [libjpeg][]. Use `-DUSEJPG` when compiling.
 *
 * [libpng]: http://www.libpng.org/pub/png/libpng.html
 * [libjpeg]: http://www.ijg.org/
 *
 * License
 * -------
 *
 * ```
 * MIT License
 *
 * Copyright (c) 2017 Werner Stoop
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ```
 *
 * API
 * ---
 */

#ifndef BMP_H
#define BMP_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef BM_LAST_ERROR
#  define BM_LAST_ERROR 1
#endif

/**
 * ### Globals
 */

/**
 * #### `extern const char *bm_last_error;`
 * The last error of any `bm_*` function where available.
 *
 * It can be disabled in by defining `BM_LAST_ERROR` as 0.
 */
#if BM_LAST_ERROR
extern const char *bm_last_error;
#endif

/**
 * ### Structures
 */

/**
 * #### `typedef struct BmPoint BmPoint;`
 * A point, with `x` and `y` coordinates
 */
typedef struct BmPoint {
    int x, y;
} BmPoint;

/**
 * #### `typedef struct BmRect BmRect;`
 * Rectangle structure.
 * `(x0,y0)` is inclusive.
 * `(x1,y1)` is exclusive.
 */
typedef struct BmRect {
    int x0, y0;
    int x1, y1;
} BmRect;

/**
 * #### `typedef struct bitmap Bitmap;`
 * Structure containing a bitmap image.
 *
 * The internal format is `0xAARRGGBB` little endian.
 * Meaning that `p[0]` contains B, `p[1]` contains G,
 * `p[2]` contains R and `p[3]` contains A
 * and the data buffer is an array of bytes BGRABGRABGRABGRABGRA...
 *
 * The member `color` contains the color that will be used for drawing
 * primitives, and for transparency while blitting.
 *
 * The member `font` is a pointer to a `BmFont` structure that is used
 * to render text. See the [Font Routines][] section for more details.
 *
 * The member `clip` is a `BmRect` that defines the clipping rectangle
 * when drawing primitives and text.
 */
typedef struct bitmap {
    /* Dimesions of the bitmap */
    int w, h;

    /* The actual pixel data in RGBA format */
    unsigned char *data;

    /* Color for the pen of the canvas */
    unsigned int color;

    /* Font object for rendering text */
    struct bitmap_font *font;

    /* Clipping rectangle */
    BmRect clip;
} Bitmap;

/**
 * #### `typedef struct bitmap_font BmFont;`
 *
 * Structure that represents the details about a font.
 * See the section on [Font Routines][] for more details.
 *
 * It has these members:
 * * `const char *type` - a text description of the type of font
 * * `int (*puts)(Bitmap *b, int x, int y, const char *text)` -
 *   Pointer to the function that will actually render the text.
 * * `int (*width)(struct bitmap_font *font)` - Function that returns the
 *   width (in pixels) of a single character in the font.
 * * `int (*height)(struct bitmap_font *font)` - Function that returns the
 *   height (in pixels) of a single character in the font.
 * * `void (*dtor)(struct bitmap_font *font)` - Destructor function that
 *   deallocates all memory allocated to the `BmFont` object.
 * * `void *data` - Additional data that may be required by the font.
 */
typedef struct bitmap_font {
    const char *type;
    int (*puts)(Bitmap *b, int x, int y, const char *text);
    int (*width)(struct bitmap_font *font);
    int (*height)(struct bitmap_font *font);
    void (*dtor)(struct bitmap_font *font);
    void *data;
} BmFont;

/**
 * ### Creating and Destroying bitmaps
 */


/**
 * #### Make text for a single line output right aligned
 */
Bitmap *bm_make_text (int w, int h, unsigned int col, unsigned int bg_col,
                      int bg_alpha, const char *text);

/**
 * #### `Bitmap *bm_create(int w, int h)`
 *
 * Creates a bitmap of the specified dimensions `w` &times; `h`.
 */
Bitmap *bm_create(int w, int h);

/**
 * #### `void bm_free(Bitmap *b)`
 *
 * Destroys a bitmap previously created with `bm_create()`
 */
void bm_free(Bitmap *b);

/**
 * #### `Bitmap *bm_copy(Bitmap *b)`
 *
 * Creates a duplicate of the bitmap `b`.
 */
Bitmap *bm_copy(Bitmap *b);

/**
 * #### `Bitmap *bm_crop(Bitmap *b, int x, int y, int w, int h)`
 *
 * Crops the bitmap `b` to the region defined by `{x,y,w,h}`
 */
Bitmap *bm_crop(Bitmap *b, int x, int y, int w, int h);

/**
 * #### `int bm_width(Bitmap *b)`
 *
 * Retrieves the width of the bitmap `b`
 */
int bm_width(Bitmap *b);

/**
 * #### `int bm_height(Bitmap *b)`
 *
 * Retrieves the height of the bitmap `b`
 */
int bm_height(Bitmap *b);

/**
 * #### `Bitmap *bm_bind(int w, int h, unsigned char *data)`
 *
 * Creates a bitmap structure bound to an existing array
 * of pixel data (for example, an OpenGL texture or a SDL surface). The
 * `data` must be an array of `w` &times; `h` &times; 4 bytes of ARGB pixel data.
 *
 * The returned `Bitmap*` must be destroyed with `bm_unbind()`
 * rather than `bm_free()`.
 */
Bitmap *bm_bind(int w, int h, unsigned char *data);

/**
 * #### `void bm_rebind(Bitmap *b, unsigned char *data)`
 *
 * Changes the data referred to by a bitmap structure previously
 * created with a call to `bm_bind()`.
 * The new data must be of the same dimensions as specified
 * in the original `bm_bind()` call.
 */
void bm_rebind(Bitmap *b, unsigned char *data);

/**
 * #### `void bm_unbind(Bitmap *b)`
 *
 * Deallocates the memory of a bitmap structure previously created
 * through `bm_bind()`.
 */
void bm_unbind(Bitmap *b);

/**
 * #### `Bitmap *bm_bind_static(Bitmap *b, unsigned char *data, int w, int h)`
 *
 * Binds a `Bitmap` structure to an array of `w` &times; `h` &times; 4 bytes.
 *
 * The intended use case is to manipulate bitmaps in local variables in functions.
 * This function does not allocate memory for the `Bitmap` structure, so the
 * return value should _not_ be freed by `bm_free()` or `bm_unbind()`.
 *
 * The following example creates a temporary copy if a bitmap `orig` which will
 * be removed automatically when the calling function returns, and does not require
 * the overhead of `malloc()` and `free()` as the other Bitmap create/bind functions.
 *
 * ```c
 * Bitmap b;
 * unsigned char buffer[WIDTH * HEIGHT * 4];
 * bm_bind_static(&b, buffer, WIDTH, HEIGHT);
 * memcpy(b.data, orig->data, WIDTH * HEIGHT * 4);
 * ```
 */
Bitmap *bm_bind_static(Bitmap *b, unsigned char *data, int w, int h);

/**
 * ### Clipping and Buffer Manipulation Functions
 */

/**
 * #### `void bm_clip(Bitmap *b, int x0, int y0, int x1, int y1)`
 *
 * Sets the clipping rectangle on the bitmap from `x0,y0` (inclusive)
 * to `x1,y1` exclusive.
 */
void bm_clip(Bitmap *b, int x0, int y0, int x1, int y1);

/**
 * #### `void bm_unclip(Bitmap *b)`
 *
 * Resets the bitmap `b`'s clipping rectangle.
 */
void bm_unclip(Bitmap *b);

/**
 * #### `void bm_flip_vertical(Bitmap *b)`
 *
 * Flips the bitmap vertically.
 */
void bm_flip_vertical(Bitmap *b);

/**
 * ### Pixel Functions
 */

/**
 * #### `unsigned int bm_get(Bitmap *b, int x, int y)`
 *
 * Retrieves the value of the pixel at `x,y` as an integer.
 *
 * The return value is in the form `0xAABBGGRR`
 */
unsigned int bm_get(Bitmap *b, int x, int y);

/**
 * #### `void bm_set(Bitmap *b, int x, int y, unsigned int c)`
 *
 * Sets the value of the pixel at `x,y` to the color `c`.
 *
 * `c` is in the form `0xAABBGGRR`.
 */
void bm_set(Bitmap *b, int x, int y, unsigned int c);

/**
 * ### Color functions
 * Functions for manipulating colors in the image.
 */

/**
 * #### `void bm_set_color(Bitmap *bm, unsigned int col)`
 *
 * Sets the color of the pen to a color represented
 * by an integer, like `0xAARRGGBB`
 */
void bm_set_color(Bitmap *bm, unsigned int col);

/**
 * #### `unsigned int bm_get_color(Bitmap *bc)`
 *
 * Retrieves the pen color.
 */
unsigned int bm_get_color(Bitmap *bm);

/**
 * #### `void bm_set_alpha(Bitmap *bm, int a)`
 *
 * Sets the alpha value of the pen to `a`
 */
void bm_set_alpha(Bitmap *bm, int a);

/**
 * #### `unsigned int bm_picker(Bitmap *bm, int x, int y)`
 *
 * Sets the color of the pen to the color of the pixel at <x,y>
 * on the bitmap.
 *
 * The pen color can then be retrieved through `bm_get_color()`.
 *
 * It returns the integer representation of the color.
 */
unsigned int bm_picker(Bitmap *bm, int x, int y);

/**
 * #### `unsigned int bm_atoi(const char *text)`
 *
 * Converts a text string like "#FF00FF" or "white" to
 * an integer of the form `0xFF00FF`.
 * The `text` parameter is not case sensitive and spaces are
 * ignored, so for example "darkred" and "Dark Red" are equivalent.
 *
 * The shorthand "#RGB" format is also supported
 * (eg. "#0fb", which is the same as "#00FFBB").
 *
 * Additionally, it also supports the CSS syntax for "RGB(r,g,b)",
 * "RGBA(r,g,b,a)", "HSL(h,s,l)" and "HSLA(h,s,l,a)".
 *
 * The list of supported colors are based on the wikipedia's
 * list of HTML and X11 [Web colors](http://en.wikipedia.org/wiki/Web_colors).
 *
 * It returns 0 (black) if the string couldn't be parsed.
 */
unsigned int bm_atoi(const char *text);

/**
 * #### `unsigned int bm_rgb(unsigned char R, unsigned char G, unsigned char B)`
 *
 * Builds a color from the specified `(R,G,B)` values
 */
unsigned int bm_rgb(unsigned char R, unsigned char G, unsigned char B);

/**
 * #### `unsigned int bm_rgba(unsigned char R, unsigned char G, unsigned char B, unsigned char A)`
 *
 * Builds a color from the specified `(R,G,B,A)` values
 */
unsigned int bm_rgba(unsigned char R, unsigned char G, unsigned char B, unsigned char A);

/**
 * #### `void bm_get_rgb(unsigned int col, unsigned char *R, unsigned char *G, unsigned char *B)`
 *
 * Decomposes a color `col` into its `R,G,B` components.
 */
void bm_get_rgb(unsigned int col, unsigned char *R, unsigned char *G, unsigned char *B);

/**
 * #### `unsigned int bm_hsl(double H, double S, double L)`
 *
 * Creates a color from the given Hue/Saturation/Lightness values.
 * See <https://en.wikipedia.org/wiki/HSL_and_HSV> for more information.
 *
 * Hue (`H`) is given as an angle in degrees from 0&deg; to 360&deg;.
 * Saturation (`S`) and Lightness (`L`) are given as percentages from 0 to 100%.
 */
unsigned int bm_hsl(double H, double S, double L);

/**
 * #### `unsigned int bm_hsla(double H, double S, double L, double A)`
 *
 * Creates a color from the given Hue/Saturation/Lightness and alpha values.
 *
 * Hue (`H`) is given as an angle in degrees from 0&deg; to 360&deg;.
 * Saturation (`S`) and Lightness (`L`) and Alpha (`A`) are given as percentages from 0 to 100%.
 */
unsigned int bm_hsla(double H, double S, double L, double A);

/**
 * #### `bm_get_hsl(unsigned int col, double *H, double *S, double *L)`
 *
 * Decomposes a color `col` into its Hue/Saturation/Lightness components.
 *
 * Hue (`H`) is given as an angle in degrees from 0&deg; to 360&deg;.
 * Saturation (`S`) and Lightness (`L`) are given as percentages from 0 to 100%.
 */
void bm_get_hsl(unsigned int col, double *H, double *S, double *L);

/**
 * #### `unsigned int bm_byte_order(unsigned int col)`
 *
 * Fixes the input color to be in the proper byte order.
 *
 * The input color should be in the format `0xAARRGGBB`. The output
 * will be in either `0xAARRGGBB` or `0xAABBGGRR` depending on how the
 * library was compiled.
 */
unsigned int bm_byte_order(unsigned int col);

/**
 * #### `unsigned int bm_lerp(unsigned int color1, unsigned int color2, double t)`
 *
 * Computes the color that is a distance `t` along the line between
 * `color1` and `color2`.
 *
 * If `t` is 0 it returns `color1`. If `t` is 1.0 it returns `color2`.
 */
unsigned int bm_lerp(unsigned int color1, unsigned int color2, double t);

/**
 * ### Blitting Functions
 */

/**
 * #### `void bm_blit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h)`
 *
 * Blits an area of `w` &times; `h` pixels at `sx,sy` on the source bitmap `src` to
 * `dx,dy` on the destination bitmap `dst`.
 */
void bm_blit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h);

/**
 * #### `void bm_maskedblit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h)`
 *
 * Blits an area of `w` &times; `h` pixels at `sx,sy` on the `src` bitmap to
 * `dx,dy` on the `dst` bitmap.
 *
 * Pixels on the `src` bitmap that matches the `src` bitmap color are not blitted.
 * The alpha value of the pixels on the `src` bitmap is not taken into account.
 */
void bm_maskedblit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h);

/**
 * #### `void bm_blit_ex(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, int mask)`
 *
 * Extended blit function. Blits an area of `sw` &times; `sh` pixels at `sx,sy` from the `src` bitmap to
 * `dx,dy` on the `dst` bitmap into an area of `dw` &times; `dh` pixels, stretching or shrinking the blitted area as neccessary.
 *
 * If `mask` is non-zero, pixels on the `src` bitmap that matches the `src` bitmap color are not blitted.
 * Whether the alpha value of the pixels is taken into account depends on whether IGNORE_ALPHA is enabled.
 */
void bm_blit_ex(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, int mask);

/**
 * #### `typedef int (*bm_blit_fun)(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, void *data);`
 *
 * Prototype for the callback function to be passed to `bm_blit_ex_fun()`.
 *
 * `dst` is the destination bitmap and `dx,dy` is where the pixel is too be plotted.
 *
 * `src` is the source bitmap and `sx,sy` is where the color of the pixel is to be obtained from.
 *
 * `mask` is the mask color of the source bitmap. The function can decide whether or not to blit the pixel based on this.
 *
 * It should return 1 on success. If it returns 0 `bm_blit_ex_fun()` will terminate immediately.
 */
typedef int (*bm_blit_fun)(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int mask, void *data);

/**
 * #### `void bm_blit_ex_fun(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, bm_blit_fun fun, void *data);`
 *
 * Blits a scaled bitmap from `src` to `dst`, but calls a callback function `fun`
 * for each pixel to handle the plotting instead of plotting the pixel directly.
 *
 * This can be used for some specific post-processing after the position of a pixel is determined
 * on the source and destination, but before the pixel is actually plotted.
 *
 * `data` is passed straight through to the callback function.
 *
 * The other parameters are the same as for `bm_blit_ex()`.
 */
void bm_blit_ex_fun(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, bm_blit_fun fun, void *data);

/**
 * #### `void bm_rotate_blit(Bitmap *dst, int ox, int oy, Bitmap *src, int px, int py, double angle, double scale);`
 *
 * Rotates a source bitmap `src` around a pivot point `px,py` and blits it onto a destination bitmap `dst`.
 *
 * The bitmap is positioned such that the point `px,py` on the source is at the offset `ox,oy` on the destination.
 *
 * The `angle` is clockwise, in radians. The bitmap is also scaled by the factor `scale`.
 */
void bm_rotate_blit(Bitmap *dst, int ox, int oy, Bitmap *src, int px, int py, double angle, double scale);

/**
 * ### Filter Functions
 */

/** #### `void bm_grayscale(Bitmap *b)`
 *
 * Converts an image to grascale.
 */
void bm_grayscale(Bitmap *b);

/**
 * #### `void bm_smooth(Bitmap *b)`
 *
 * Smoothes the bitmap `b` by applying a 5&times;5 Gaussian filter.
 */
void bm_smooth(Bitmap *b);

/**
 * #### `void bm_apply_kernel(Bitmap *b, int dim, float kernel[])`
 *
 * Applies a `dim` &times; `dim` kernel to the image.
 *
 * ```c
 * float smooth_kernel[] = { 0.0, 0.1, 0.0,
 *                           0.1, 0.6, 0.1,
 *                           0.0, 0.1, 0.0};
 * bm_apply_kernel(screen, 3, smooth_kernel);
 * ```
 */
void bm_apply_kernel(Bitmap *b, int dim, float kernel[]);

/**
 * #### `Bitmap *bm_resample(const Bitmap *in, int nw, int nh)`
 *
 * Creates a new bitmap of dimensions `nw` &times; `nh` that is a scaled
 * using the Nearest Neighbour method the input bitmap.
 *
 * The input bimap remains untouched.
 */
Bitmap *bm_resample(const Bitmap *in, int nw, int nh);

/**
 * #### `Bitmap *bm_resample_blin(const Bitmap *in, int nw, int nh)`
 *
 * Creates a new bitmap of dimensions `nw` &times; `nh` that is a scaled
 * using Bilinear Interpolation from the input bitmap.
 *
 * The input bimap remains untouched.
 *
 * _Bilinear Interpolation is better suited for making an image larger._
 */
Bitmap *bm_resample_blin(const Bitmap *in, int nw, int nh);

/**
 * #### `Bitmap *bm_resample_bcub(const Bitmap *in, int nw, int nh)`
 *
 * Creates a new bitmap of dimensions `nw` &times; `nh` that is a scaled
 * using Bicubic Interpolation from the input bitmap.
 *
 * The input bimap remains untouched.
 *
 * _Bicubic Interpolation is better suited for making an image smaller._
 */
Bitmap *bm_resample_bcub(const Bitmap *in, int nw, int nh);

/**
 * #### `Bitmap *bm_resample_into(const Bitmap *in, Bitmap *out)`
 *
 * Resamples a bitmap `in` to fit into a bitmap `out` using nearest neighbour.
 */
Bitmap *bm_resample_into(const Bitmap *in, Bitmap *out);

/**
 * #### `Bitmap *bm_resample_blin_into(const Bitmap *in, Bitmap *out)`
 *
 * Resamples a bitmap `in` to fit into a bitmap `out` using bilinear interpolation.
 */
Bitmap *bm_resample_blin_into(const Bitmap *in, Bitmap *out);

/**
 * #### `Bitmap *bm_resample_bcub_into(const Bitmap *in, Bitmap *out)`
 *
 * Resamples a bitmap `in` to fit into a bitmap `out` using bicubic interpolation.
 */
Bitmap *bm_resample_bcub_into(const Bitmap *in, Bitmap *out);

/**
 * #### `void bm_swap_color(Bitmap *b, unsigned int src, unsigned int dest)`
 *
 * Replaces all pixels of color `src` in bitmap `b` with the color `dest`.
 */
void bm_swap_color(Bitmap *b, unsigned int src, unsigned int dest);

/**
 * #### `void bm_reduce_palette(Bitmap *b, unsigned int palette[], size_t n)`
 *
 * Reduces the colors in the bitmap `b` to the colors in `palette`
 * by applying [Floyd-Steinberg dithering](http://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering)
 *
 * `palette` is an array of integers containing the new palette and
 * `n` is the number of entries in the palette.
 */
void bm_reduce_palette(Bitmap *b, unsigned int palette[], unsigned int n);

/**
 * #### `void bm_reduce_palette_OD4(Bitmap *b, unsigned int palette[], size_t n)`
 *
 * Reduces the colors in the bitmap `b` to the colors in `palette`
 * by applying [ordered dithering](https://en.wikipedia.org/wiki/Ordered_dithering)
 * and a 4x4 Bayer matrix.
 *
 * `palette` is an array of integers containing the new palette and
 * `n` is the number of entries in the palette.
 */
void bm_reduce_palette_OD4(Bitmap *b, unsigned int palette[], unsigned int n);

/**
 * #### `void bm_reduce_palette_OD8(Bitmap *b, unsigned int palette[], size_t n)`
 *
 * Reduces the colors in the bitmap `b` to the colors in `palette`
 * by applying [ordered dithering](https://en.wikipedia.org/wiki/Ordered_dithering)
 * and a 8x8 Bayer matrix.
 *
 * `palette` is an array of integers containing the new palette and
 * `n` is the number of entries in the palette.
 */
void bm_reduce_palette_OD8(Bitmap *b, unsigned int palette[], unsigned int n);

/**
 * ### Drawing Primitives
 * `bmp.h` provides these methods for drawing graphics primitives.
 */

/**
 * #### `void bm_clear(Bitmap *b)`
 *
 * Clears the bitmap to the pen color.
 */
void bm_clear(Bitmap *b);

/**
 * #### `void bm_putpixel(Bitmap *b, int x, int y)`
 *
 * Draws a single pixel at <x,y> using the pen color.
 */
void bm_putpixel(Bitmap *b, int x, int y);

/**
 * #### `void bm_line(Bitmap *b, int x0, int y0, int x1, int y1)`
 *
 * Draws a line from <x0,y0> to <x1,y1> using the pen color.
 */
void bm_line(Bitmap *b, int x0, int y0, int x1, int y1);

/**
 * #### `void bm_rect(Bitmap *b, int x0, int y0, int x1, int y1)`
 *
 * Draws a rectangle from <x0,y0> to <x1,y1> using the pen color.
 */
void bm_rect(Bitmap *b, int x0, int y0, int x1, int y1);

/**
 * #### `void bm_fillrect(Bitmap *b, int x0, int y0, int x1, int y1)`
 *
 * Fills a rectangle from <x0,y0> to <x1,y1> using the pen color.
 */
void bm_fillrect(Bitmap *b, int x0, int y0, int x1, int y1);

/**
 * #### `void bm_dithrect(Bitmap *b, int x0, int y0, int x1, int y1)`
 *
 * Draws a rectangle dithered in a checkerboard pattern
 * from `<x0,y0>` to `<x1,y1>`, using the pen color.
 */
void bm_dithrect(Bitmap *b, int x0, int y0, int x1, int y1);

/**
 * #### `void bm_circle(Bitmap *b, int x0, int y0, int r)`
 *
 * Draws a circle of radius `r` centered at <x,y> using the pen color.
 */
void bm_circle(Bitmap *b, int x0, int y0, int r);

/**
 * #### `void bm_fillcircle(Bitmap *b, int x0, int y0, int r)`
 *
 * Fills a circle of radius `r` centered at <x,y> using the pen color.
 */
void bm_fillcircle(Bitmap *b, int x0, int y0, int r);

/**
 * #### `void bm_ellipse(Bitmap *b, int x0, int y0, int x1, int y1)`
 *
 * Draws an ellipse that occupies the rectangle from <x0,y0> to
 * <x1,y1>, using the pen color
 */
void bm_ellipse(Bitmap *b, int x0, int y0, int x1, int y1);

/**
 * #### `void bm_round_rect(Bitmap *b, int x0, int y0, int x1, int y1, int r)`
 *
 * Draws a rect from <x0,y0> to <x1,y1> using the pen color with rounded corners
 * of radius `r`
 */
void bm_roundrect(Bitmap *b, int x0, int y0, int x1, int y1, int r);

/**
 * #### `void bm_fill_round_rect(Bitmap *b, int x0, int y0, int x1, int y1, int r)`
 *
 * Fills a rectangle from <x0,y0> to <x1,y1> using the pen color with rounded corners
 * of radius `r`
 */
void bm_fillroundrect(Bitmap *b, int x0, int y0, int x1, int y1, int r);

/**
 * #### `void bm_bezier3(Bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2)`
 *
 * Draws a Bezier curve with 3 control points `<x0,y0>`, `<x1,y1>` and `<x2,y2>`.
 */
void bm_bezier3(Bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2);

/**
 * #### `void bm_poly(Bitmap *b, BmPoint points[], unsigned int n)`
 *
 * Draws a polygon using the pen color.
 *
 * `points` is an array of `n` `BmPoints` of the polygon's vertices.
 */
void bm_poly(Bitmap *b, BmPoint points[], unsigned int n);

/**
 * #### `void bm_fillpoly(Bitmap *b, BmPoint points[], unsigned int n)`
 *
 * Draws a filled polygon using the pen color.
 *
 * `points` is an array of `n` `BmPoints` of the polygon's vertices.
 */
void bm_fillpoly(Bitmap *b, BmPoint points[], unsigned int n);

/**
 * #### `void bm_fill(Bitmap *b, int x, int y)`
 *
 * Floodfills from `<x,y>` using the pen color.
 *
 * The color of the pixel at `<x,y>` is used as the source color.
 * The color of the pen is used as the target color.
 */
void bm_fill(Bitmap *b, int x, int y);

/**
 * ### Font Routines
 */

/**
 * #### `void bm_set_font(Bitmap *b, BmFont *font)`
 *
 * Changes the font used to render text on the bitmap.
 */
void bm_set_font(Bitmap *b, BmFont *font);

/**
 * #### `BmFont *bm_get_font(Bitmap *b)`
 *
 * Retrieves the current font used to render text on the bitmap.
 */
BmFont *bm_get_font(Bitmap *b);

/**
 * #### `void bm_reset_font(BmFont *b)`
 *
 * Resets the font used to draw on the `Bitmap` to the
 * default *Apple II*-inspired font
 */
void bm_reset_font(Bitmap *b);

/**
 * #### `int bm_text_width(Bitmap *b, const char *s)`
 *
 * Returns the width in pixels of a string of text.
 */
int bm_text_width(Bitmap *b, const char *s);

/**
 * #### `int bm_text_height(Bitmap *b, const char *s)`
 *
 * returns the height in pixels of a string of text.
 */
int bm_text_height(Bitmap *b, const char *s);

/**
 * #### `void bm_putc(Bitmap *b, int x, int y, char c)`
 *
 * Prints the character `c` at position `x,y` on the bitmap `b`.
 */
int bm_putc(Bitmap *b, int x, int y, char c);

/**
 * #### `void bm_puts(Bitmap *b, int x, int y, const char *s)`
 *
 * Prints the string `s` at position `x,y` on the bitmap `b`.
 */
int bm_puts(Bitmap *b, int x, int y, const char *text);

/**
 * #### `void bm_printf(Bitmap *b, int x, int y, const char *fmt, ...)`
 *
 * Prints a `printf()`-formatted style string to the bitmap `b`,
 * `fmt` is a `printf()`-style format string (It uses `vsnprintf()` internally).
 */
int bm_printf(Bitmap *b, int x, int y, const char *fmt, ...);

/**
 * #### `void bm_free_font(BmFont *font)`
 *
 * Deallocates a font. It basically just calls the `dtor` member of the
 * `font` structure.
 */
void bm_free_font(BmFont *font);

/**
 * #### `BmFont *bm_make_ras_font(const char *file, int spacing)`
 *
 * Creates a raster font from a bitmap file named `file` in any of the
 * supported file types.
 *
 * The characters in the bitmap must be arranged like this:
 * ```
 *  !"#$%&'()*+,-./
 * 0123456789:;<=>?
 * @ABCDEFGHIJKLMNO
 * PQRSTUVWXYZ[\]^_
 * `abcdefghijklmno
 * pqrstuvwxyz{|}~
 * ```
 * The characters are in ASCII sequence, without the first 32 control characters.
 * The pixel width and hight of the individual characters is calculated by dividing
 * the width and height of the bitmap by 16 and 6 respectively.
 *
 * `fonts/font.gif` is an example of an font that can be used in this way:  \
 * ![sample font image](fonts/font.gif)
 *
 * The image is 128x48 pixels, so the individual characters are 8x8 pixels.
 * (128/16=8 and 48/6=8)
 *
 * Black (`#000000`) is used to indicate transparent pixels.
 *
 * The `spacing` parameter determines. If it is zero, the width of the
 * characters is used.
 *
 * The returned font's `type` will be set to `"RASTER_FONT"`
 */
BmFont *bm_make_ras_font(const char *file, int spacing);

/** #### `BmFont *bm_make_sfont(const char *file)`
 *
 * Creates a raster font from a SFont or a GrafX2 font.
 *
 * A [SFont][sfont] is a bitmap (in any supported file format) that contains all
 * ASCII characters from 33 (`'!'`) to 127 in a single row. There is an additional
 * row of pixels at the top that describes the width of each character using magenta
 * (`#FF00FF`) pixels for the spacing. The height of the font is the height of the
 * rest of the bitmap.
 *
 * [GrafX2][grafx2] is a pixel art paint program that uses a similar format, except
 * the pixels in the first row don't have to use magenta.
 *
 * [sfont]: http://www.linux-games.com/sfont/
 * [grafx2]: https://en.wikipedia.org/wiki/GrafX2
 */
BmFont *bm_make_sfont(const char *file);

/** #### `BmFont *bm_make_xbm_font(const unsigned char *bits, int spacing)`
 *
 * Creates a font from a XBM bitmap. The XBM bitmaps can be compiled directly into
 * a program's executable rather than being loaded at runtime.
 *
 * The XBM bitmap should have the same layout as `bm_make_ras_font()` above.
 * The `fonts/` directory has several examples available.
 */
BmFont *bm_make_xbm_font(const unsigned char *bits, int spacing);

/**
 * TODO
 * ----
 * - [x] I should also go through the API and make the naming a bit more consistent.
 *     - ~~Functions like `bm_rect()` should use `w,h` instead of `x1,y1` as parameters.~~
 * - [ ] How about replacing functions like `bm_brightness()` with a `bm_foreach()`
 *       function that takes a callback which iterates over all the pixels to simplify
 *       the API.  \
 *       The callback can look like `int (*)(Bitmap *b, int oldcolor, int x, int y)`
 * - [ ] I've added a precompiler definition `IGNORE_ALPHA` which causes all color
 *       operations to apply a `& 0x00FFFFFF` so that alpha values are ignored.  \
 *       It is not properly tested because I don't have any serious projects that
 *       depends on the alpha values at the moment.
 * - [x] `bm_fill()` should _perhaps_ stop using `bm_picker()`
 * - [ ] To consider: In `bm_rotate_blit()` perhaps check `u,v` against the `src`
 *       clipping rect instead.  \
 *       If I do this, I might have to do it for all blitting functions.
 * - [x] Maybe `bm_fill()` _should_ take the clipping rectangle into account.
 * - [ ] `bm_fillellipse()`, like `bm_ellipse()` but filled.
 * - [x] In `bm_make_ras_font()`, because the top left corner is the space character, we
 *       can assume that the color of that pixel is the transparent color, rather than
 *       hardcoding it as black (0).
 * - [ ] `bm_atoi()` does not parse `chucknorris` correctly.  \
 *       See <https://stackoverflow.com/a/8333464/115589>
 * - [ ] It only supports RGB and RGBA PNGs, but I encountered some other types of
 *       PNGs in the wild, so support ought to be added.  \
 *       The unsupported PNGs were in some of the SFont/GrafX2 fonts I cite at the
 *       end of the README.
 * - [ ] I'm regretting my decision to have the BmFont.width function not look at the
 *       actual character you want to draw, so `bm_text_width()` is broken if you
 *       aren't using a fixed width font.
 *
 * References
 * ----------
 *
 * * [BMP file format](http://en.wikipedia.org/wiki/BMP_file_format) on Wikipedia
 * * [Bresenham's line algorithm](http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm) on Wikipedia
 * * <http://members.chello.at/~easyfilter/bresenham.html>
 * * [Flood fill](http://en.wikipedia.org/wiki/Flood_fill) on Wikipedia
 * * [Midpoint circle algorithm](http://en.wikipedia.org/wiki/Midpoint_circle_algorithm) on Wikipedia
 * * <http://web.archive.org/web/20110706093850/http://free.pages.at/easyfilter/bresenham.html>
 * * [Typography in 8 bits: System fonts](http://damieng.com/blog/2011/02/20/typography-in-8-bits-system-fonts)
 * * [GIF89a specification](http://www.w3.org/Graphics/GIF/spec-gif89a.txt)
 * * Nelson, M.R. : "LZW Data Compression", Dr. Dobb's Journal, October 1989.
 * * <http://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art011>
 * * [What's In A GIF](http://www.matthewflickinger.com/lab/whatsinagif/index.html) by Matthew Flickinger
 * * <http://web.archive.org/web/20100206055706/http://www.qzx.com/pc-gpe/pcx.txt>
 * * <http://www.shikadi.net/moddingwiki/PCX_Format>
 * * [Truevision TGA](https://en.wikipedia.org/wiki/Truevision_TGA) on Wikipedia
 * * <http://paulbourke.net/dataformats/tga/>
 * * <http://www.ludorg.net/amnesia/TGA_File_Format_Spec.html>
 * * [X PixMap](https://en.wikipedia.org/wiki/X_PixMap) on Wikipedia
 * * <http://www.fileformat.info/format/xpm/egff.htm>
 * * "Fast Bitmap Rotation and Scaling" By Steven Mortimer, Dr Dobbs' Journal, July 01, 2001  \
 *   <http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337>
 * * <http://www.efg2.com/Lab/ImageProcessing/RotateScanline.htm>
 * * [Image Filtering](http://lodev.org/cgtutor/filtering.html) in _Lode's Computer Graphics Tutorial_
 * * [Efficient Polygon Fill Algorithm With C Code Sample](http://alienryderflex.com/polygon_fill/)
 *   by Darel Rex Finley.
 * * [Computer Graphics: Scan Line Polygon Fill Algorithm](https://hackernoon.com/computer-graphics-scan-line-polygon-fill-algorithm-3cb47283df6)
 *   by Alberto Scicali
 */

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif

#endif /* BMP_H */
