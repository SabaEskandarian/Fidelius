#include "rgbencoder.h"
#include <stdio.h>
#include <stdint.h>

static void pack_pixels(char *dst, char *src);
static void unpack_pixels(char *dst, char *src);
static inline void convert_to_rgb(char *dst, char val);
static inline char convert_from_rgb(char *dst);
static inline int get_bitmask(char start, char len);

/* Converts an RGB array to RGBA */
void rgb_to_rgba(char *dst, char *src, int size)
{
  for (int i = 0; i < size * 4; i++){
    if (i % 4 == 3)
      *dst = 0x00;
    else
      *dst = *src++;
    dst++;
  }
}

/* Encodes the input data into RGB data, each pixel color is divided into 4
 * buckets to store a 4^3 = 64 distinct pixel values = 6 bits.
 * 
 * dst : Destination for RGB data, must be 4 times larger than the src buffer
 * src : Data to be encoded, data must be a multiple of 3 bytes
 * len : Length of the data to be encoded in bytes 
 */
void encode_rgb(char *dst, char *src, int len)
{
  for (int i = 0; i < len/3; i++)
  {
    pack_pixels(dst, src);
    dst += 12;
    src += 3;
  }
}

/* Decodes the given RGB data
 * 
 * dst : Destination for the decoded data, the provided buffer needs to be at
 * least src/4 bytes longs
 * src : Data to be encoded, data must be a multiple of 3 bytes
 * len : Length of the data to be encoded in bytes 
 */
void decode_rgb(char *dst, char *src, int len)
{
  for (int i = 0; i < len/3; i++)
  {
    unpack_pixels(dst, src);
    dst += 3;
    src += 12;
  }
}

/* Packs 3 bytes of source information into 4 pixels */
static void pack_pixels(char *dst, char *src)
{
  /* Bits 0 - 6 */
  char val = *src & get_bitmask(0, 6);
  convert_to_rgb(dst, val);
  dst += 3;

  /* Bits 6 - 12 */
  val = (*src & get_bitmask(6, 2)) >> 6;
  src++;
  val |= (*src & get_bitmask(0, 4)) << 2;
  convert_to_rgb(dst, val);
  dst += 3;

  /* Bits 12 - 18 */
  val = (*src & get_bitmask(4, 4)) >> 4;
  src++;
  val |= (*src & get_bitmask(0, 2)) << 4;
  convert_to_rgb(dst, val);
  dst += 3;

  /* Bits 18-24 */
  val = (*src & get_bitmask(2, 6)) >> 2;
  convert_to_rgb(dst, val);
}

/* Unpacks 4 pixels into decoded data */
static void unpack_pixels(char *dst, char *src)
{
  /* Bits 0 - 6 */
  char val = convert_from_rgb(src);
  *dst = val;
  src += 3;

  /* Bits 6 - 12 */
  val = convert_from_rgb(src);
  *dst |= (val & get_bitmask(0, 2)) << 6;
  dst++;
  *dst = (val & get_bitmask(2, 4)) >> 2;
  src += 3;

  /* Bits 12 - 18 */
  val = convert_from_rgb(src);
  *dst |= (val & get_bitmask(0, 4)) << 4;
  dst++;
  *dst = (val & get_bitmask(4, 2)) >> 4;
  src += 3;

  /* Bits 18-24 */
  val = convert_from_rgb(src);
  *dst |= (val & get_bitmask(0, 6)) << 2;
}

static inline void convert_to_rgb(char *dst, char val)
{
  char mask = get_bitmask(0, 2);

  for (int i = 0; i < 3; i++)
  {
    char color = val & mask;
    *dst = (color << 6) + 32;
    val >>= 2;
    dst++;
  }
}

static inline char convert_from_rgb(char *dst)
{
  char val = 0;

  for (int i = 0; i < 3; i++)
  {
    char color = (*dst >> 6) << i * 2;
    val |= (*dst >> 6) << i * 2;
    dst++;
  }
  return val;
}

static inline int get_bitmask(char start, char len)
{
  return ~(~0 << len) << start;
}

