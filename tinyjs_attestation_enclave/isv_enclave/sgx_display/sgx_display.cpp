#include <stdlib.h>
#include "bmp.h"
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "isv_enclave_t.h"
#include "isv_enclave.h"
#include <stdarg.h>
#include <string>
#include <map>
#include <vector>
#include <bitset>
//#include <chrono>
//#include <fstream>

#define bytes_per_pixel 4

#define OP_ADD_OVERLAY 1
#define OP_REMOVE_OVERLAY 2
#define OP_CLEAR_OVERLAYS 3

#define ADD_OVERLAY_HDR_LEN 16
#define REMOVE_OVERLAY_HDR_LEN 12
#define BMP_TAG_LEN 16
#define BMP_IV_LEN 12

static bool isLittleEndian();
static uint16_t htons(uint16_t in);
static uint32_t htonl(uint32_t in);
static uint32_t add_bitmap_data(uint8_t *output, uint16_t x, uint16_t y,
                                uint16_t width, uint16_t height,
                                std::string value);

const sgx_aes_gcm_128bit_key_t p_key = {
  0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
  0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
};
static uint16_t seq_no = 0;

extern std::map<std::string, form> forms;
extern std::string origin;

//#define timeNow() std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count()
//ofstream denc_times("dis_enc_times.csv");

void printFormDisplay(form f){
    printf_enc("# elements in form: %d\n", f.inputs.size());
    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
    it != f.inputs.end(); ++it)
    {
        printf_enc("input name: %s\n", it->first.c_str());
    }
}



/* Creates an encrypted message to add an overlay on the screen. The header
 * information is authenticated and the input field data is encrypted. All the
 * overlays for a form are stored on the raspberry pi with an ID. Writing to
 * the same overlay ID will replace the previously stored overlay with a new one.
 *
 * output: pointers to the output buffer to store the message
 * out_len: total length of the message
 * form_id: form_id string
 */
void create_add_overlay_msg(uint8_t *output, uint32_t *out_len, const char *form_id)
{
  ocall_print_string("DISPLAY: BEGIN");
  uint8_t *p_aad = output;
  uint16_t seq = htons(seq_no);

  *out_len = (uint32_t) ADD_OVERLAY_HDR_LEN + BMP_TAG_LEN + BMP_IV_LEN;

  //ocall_print_string("DISPLAY: SETTING HEADER");
  /* Copy header data */
  memcpy(output, &seq, sizeof(uint16_t));
  seq_no++;
  output += sizeof(uint16_t);
  *output = OP_ADD_OVERLAY;
  output++;
  /* TODO: Replace ID with ID of the form */
  *output = 0;
  output++;
  /* Token */
  output += 8;
  uint32_t *msg_len = (uint32_t *) output;
  output += sizeof(uint32_t);

  /* Start of encrypted data */
  uint8_t *p_enc = output;
  //ocall_print_string("DISPLAY: READING FORMS");
  /* Add bitmap data */
  uint32_t bitmap_len = 0;

  std::map<std::string,form>::iterator form_it;
  form_it = forms.find((std::string)(form_id));
  if (form_it == forms.end())
  {
    //ocall_print_string("DISPLAY: FORM NOT FOUND");
  }
  form form = form_it->second;
  //printFormDisplay(form);
  //printf_enc("NUMBER OF INPUTS: %d", form.inputs.size());

  printf_enc("DISPLAY: FORM ORGIN = %s", origin.c_str());
  /* Add the origin bitmap */
  bitmap_len = add_bitmap_data(output, form.x, form.y, 200,
                            30, origin);
  *out_len += bitmap_len;
  output += bitmap_len;

  ocall_print_string("DISPLAY: ADDING FIELD BITMAPS");
  /* Add the bitmaps of the input fields */
  //printf_enc("DISPLAY: Form X = %d", form.x);
  //printf_enc("DISPLAY: Form Y = %d", form.y);
  for (std::map<std::string, input>::iterator it = form.inputs.begin();
       it != form.inputs.end(); it++)
  {
    input field = it->second;
    printf_enc("DISPLAY: Input Field name = %s value = %s", it->first.c_str(), field.value.c_str());
    //printf_enc("DISPLAY: Input Field Value = %s", );
    //printf_enc("DISPLAY: Input Field X = %d", field.x);
    //printf_enc("DISPLAY: Input Field Y = %d", field.y);
    //printf_enc("DISPLAY: Input Field Width = %d", field.width);
   //printf_enc("DISPLAY: Input Field Height= %d", field.height);

    //ocall_print_string(field.value.c_str());
    bitmap_len = add_bitmap_data(output, field.x, field.y, field.width,
                                 field.height, field.value);
    output += bitmap_len;
    *out_len += bitmap_len;
  }

  /* Set the correct message length after adding fields */
  *msg_len = htonl(*out_len);

  /* Generate random iv */
  uint8_t p_iv[BMP_IV_LEN];
  sgx_read_rand(p_iv, BMP_IV_LEN);

  /* Copy the encrypted data to the output buffer */
  sgx_aes_gcm_128bit_tag_t tag;
  /* TODO: What if encryption fails? */
  //denc_times << "bitmap_encryption," << timeNow() << "," << std::endl;
  if (sgx_rijndael128GCM_encrypt(&p_key, p_enc, (uint32_t) (output - p_enc), p_enc, p_iv,
                             BMP_IV_LEN, NULL, 0, &tag) != SGX_SUCCESS) printf_enc("DISPLAY: ENCRYPT FAILED");

  /* Copy the tag and nonce to the output buffer */
  memcpy(output,(uint8_t *) &tag, BMP_TAG_LEN);
  output += BMP_TAG_LEN;
  memcpy(output, p_iv, BMP_IV_LEN);
  
  printf_enc("DISPLAY: OVERLAY PACKET CREATED, RETURNING NORMALLY");
}

/* Adds the bitmap metadata and rgba data for an input field into the encrypted buffer
 * sent to the rpi
 *
 * x: x-coordinate of where the bitmap will be placed (from top left)
 * y: y-coordinate of where the bitmap will be placed (from top left)
 * width: width of the bitmap image in pixels
 * height: height of the bitmap image in pixels
 */
static uint32_t add_bitmap_data(uint8_t *output, uint16_t x, uint16_t y,
                                uint16_t width, uint16_t height,
                                std::string value)
{
  uint32_t img_len = width * height * bytes_per_pixel;
  char text[value.length()+1];
  std::strncpy(text, value.c_str(), value.length()+1);

  /* Draw character string */
  Bitmap *b = bm_make_text(width, height, bm_atoi("black"), bm_atoi("white"),
                           255, text);

  int len = width*height*4;
	stlpmtx_std::vector<bool> bits;
	for (int i = 0; i < len; i+=4) {
		if ((b->data[i]) == 0) {
			bits.push_back(false);
		} else {
			bits.push_back(true);
		}
	}
	while (bits.size()%8 != 0) {bits.push_back(false);}

	int bufferLen = bits.size()/8;
	char buffer[bufferLen];
	for (int i = 0; i < bufferLen; i++) {
		std::bitset<8> bits8;
		for (int j = 0; j < 8; j++) {
			bits8[j] = bits[i*8 + j];
		}
		char oneByte = (char)bits8.to_ulong();
		buffer[i] = oneByte;
	}


  /* Copy meta data and rgba data to buffer */
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
  memcpy(output, buffer, bufferLen);
  output += bufferLen;

  /* Free bitmap data */
  bm_free(b);

  return img_len + 8;
}

/* Creates an authenticated message to remove an overlay on the screen. The
 * header information is authenticated but no data is encrypted.
 *
 * output: pointers to the output buffer to store the message
 * out_len: total length of the message
 * form_id: form_id string
 */
void create_remove_overlay_msg(uint8_t *output, uint32_t *out_len, const char *form_id)
{
  uint8_t *p_aad = output;
  uint8_t op = OP_ADD_OVERLAY;
  uint16_t seq = htons(seq_no);

  memcpy(output, &seq, sizeof(uint16_t));
  seq_no++;
  output += sizeof(uint16_t);
  *output = OP_REMOVE_OVERLAY;
  output++;
  /* TODO: Replace ID with ID of the form */
  *output = 0;
  output++;

  /* Token */
  output += 8;

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

static bool isLittleEndian()
{
  volatile uint8_t swaptest[2] = {1,0};
  return ( *(uint16_t *)swaptest == 1);
}

static uint16_t htons(uint16_t in)
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

static uint32_t htonl(uint32_t in)
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
