#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */

#include "sgx_tseal.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

int generate_random_number(int one);
void hardcoded();
void gcm_decrypt(uint8_t* p_src, uint32_t src_len, uint8_t* p_dst, uint8_t* p_iv, sgx_aes_gcm_128bit_tag_t* p_in_mac);
void change_mode();
void nextchar_decrypted();
void nextchar_encrypted();
sgx_status_t seal(uint8_t* plaintext, size_t plaintext_len, sgx_sealed_data_t* sealed_data, size_t sealed_size);
sgx_status_t unseal(sgx_sealed_data_t* sealed_data, size_t sealed_size, uint8_t* plaintext, uint32_t plaintext_len);

sgx_status_t SGX_CDECL ocall_print(const char* str);
sgx_status_t SGX_CDECL ocall_getKeyBoardInput(uint8_t* kb, uint8_t* p_src, uint8_t* tag, uint8_t* p_iv);
sgx_status_t SGX_CDECL ocall_getPointerToKb(uint8_t** kb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
