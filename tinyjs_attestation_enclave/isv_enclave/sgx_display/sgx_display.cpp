#include <stdlib.h>
#include "string.h"
#include "bmp.h"
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "isv_enclave_t.h"

#define bytes_per_pixel 4

#define OP_ADD_OVERLAY 1
#define OP_REMOVE_OVERLY 2
#define OP_CLEAR_OVERLAYS 3

#define ADD_OVERLAY_HDR_LEN 16
#define REMOVE_OVERLAY_HDR_LEN 4
#define BMP_TAG_LEN 16
#define BMP_IV_LEN 12

bool isLittleEndian();
uint16_t htons(uint16_t in);
uint32_t htonl(uint32_t in);

const sgx_aes_gcm_128bit_key_t p_key = {
  0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
  0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
};
static uint16_t seq_no = 0;

/* Creates an encrypted message to add an overlay on the screen. The header
 * information is authenticated and bitmap rgba data is encrypted. Each overlay
 * is stored on the raspberry with an ID. Writing to the same overlay ID will
 * replace the previously stored overlay with a new one.
 *
 * output: pointers to the output buffer to store the message
 * out_len: total length of the message
 * input: input string to convert into a bitmap
 * id: id of the overlay to add
 * x: x-coordinate of where the overlay will be placed (from top left)
 * y: y-coordinate of where the overlay will be placed (from top left)
 * width: width of the bitmap image in pixels
 * height: height of the bitmap image in pixels
 */
void create_add_overlay_msg(uint8_t *output, uint32_t *out_len, char *input,
                            uint8_t id, uint16_t x, uint16_t y, uint16_t width,
                            uint16_t height)
{
  uint8_t *p_aad = output;
  uint16_t seq = htons(seq_no);
  *out_len = (uint32_t) width * height * bytes_per_pixel;

  /* Draw character string */
  Bitmap *b = bm_create(width, height);
  bm_set_color(b, bm_atoi("red"));
  bm_set_alpha(b, 255);
  bm_puts(b, 0, 0, (char*)input);

  /* Copy overlay metadata */
  memcpy(output, &seq, sizeof(uint16_t));
  seq_no++;
  output += sizeof(uint16_t);
  *output = OP_ADD_OVERLAY;
  output++;
  *output = id;
  output++;
  x = htons(x);
  memcpy(output, &x, sizeof(uint16_t));
  output += sizeof(uint16_t);
  y = htons(y);
  memcpy(output, &y, sizeof(uint16_t));
  output += sizeof(uint16_t);
  width = htons(width);
  memcpy(output, &width, sizeof(uint16_t));
  output += sizeof(uint16_t);
  height = htons(height);
  memcpy(output, &height, sizeof(uint16_t));
  output += sizeof(uint16_t);
  uint32_t img_len = htonl(*out_len);
  memcpy(output, &img_len, sizeof(uint32_t));
  output += sizeof(uint32_t);

  /* Generate random iv */
  uint8_t p_iv[BMP_IV_LEN];
  sgx_read_rand(p_iv, BMP_IV_LEN);

  /* Copy the encrypted data to the output buffer */
  sgx_aes_gcm_128bit_tag_t tag;
  /* TODO: What if encryption fails? */
  sgx_rijndael128GCM_encrypt(&p_key, b->data, *out_len, output, p_iv,
                             BMP_IV_LEN, p_aad, ADD_OVERLAY_HDR_LEN, &tag);
  output += *out_len;

  /* Copy the tag and nonce to the output buffer */
  memcpy(output,(uint8_t *) &tag, BMP_TAG_LEN);
  output += BMP_TAG_LEN;
  memcpy(output, p_iv, BMP_IV_LEN);

  /* Return the total length of the package including headers (img_len already
   * included) */
  *out_len += ADD_OVERLAY_HDR_LEN + BMP_TAG_LEN + BMP_IV_LEN;

  /* Null terminate rgba data */
  bm_free(b);
}

/* Creates an authenticated message to remove an overlay on the screen. The
 * header information is authenticated but no data is encrypted.
 *
 * output: pointers to the output buffer to store the message
 * out_len: total length of the message
 * clear_all: set to 0 or 1. If 1 all overlays will be removed
 * id: id of the overlay to add
 */
void create_remove_overlay_msg(uint8_t *output, uint32_t *out_len,
                               uint8_t clear_all, uint8_t id)
{
  uint8_t *p_aad = output;
  uint8_t op = OP_ADD_OVERLAY;
  uint16_t seq = htons(seq_no);

  memcpy(output, &seq, sizeof(uint16_t));
  seq_no++;
  output += sizeof(uint16_t);
  if (clear_all)
    *output = OP_CLEAR_OVERLAYS;
  else
    *output = OP_REMOVE_OVERLY;
  output++;
  *output = id;
  output++;

  /* Generate random iv */
  uint8_t p_iv[BMP_IV_LEN];
  sgx_read_rand(p_iv, BMP_IV_LEN);

  sgx_aes_gcm_128bit_tag_t tag;
  /* TODO: What if encryption fails? */
  sgx_rijndael128GCM_encrypt(&p_key, NULL, 0, output, p_iv, BMP_IV_LEN, p_aad,
                            REMOVE_OVERLAY_HDR_LEN, &tag);

  /* Copy the tag and nonce to the output buffer */
  memcpy(output,(uint8_t *) &tag, BMP_TAG_LEN);
  output += BMP_TAG_LEN;
  memcpy(output, p_iv, BMP_IV_LEN);

  /* Return the total length of the message */
  *out_len = REMOVE_OVERLAY_HDR_LEN + BMP_TAG_LEN + BMP_IV_LEN;
}

bool isLittleEndian()
{
  volatile uint8_t swaptest[2] = {1,0};
  return ( *(uint16_t *)swaptest == 1);
}

uint16_t htons(uint16_t in)
{
  uint8_t *b, swp;
  if (isLittleEndian())
  {
    b = (uint8_t *) &in;
    swp = *b;
    *b = *(b+1);
    *(b+1) = swp;
  }
  return in;
}

uint32_t htonl(uint32_t in)
{
  uint8_t *b, swp;
  if (isLittleEndian())
  {
    b = (uint8_t *) &in;
    swp = *b;
    *b = *(b+3);
    *(b+3) = swp;
    swp = *(b+1);
    *(b+1) = *(b+2);
    *(b+2) = swp;
  }
  return in;
}

