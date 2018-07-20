#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_satus_t etc. */

#include "sgx_tseal.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print, (const char* str));
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_getKeyBoardInput, (uint8_t* kb, uint8_t* p_src, uint8_t* tag, uint8_t* p_iv));
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_getPointerToKb, (uint8_t** kb));

sgx_status_t generate_random_number(sgx_enclave_id_t eid, int* retval, int one);
sgx_status_t hardcoded(sgx_enclave_id_t eid);
sgx_status_t gcm_decrypt(sgx_enclave_id_t eid, uint8_t* p_src, uint32_t src_len, uint8_t* p_dst, uint8_t* p_iv, sgx_aes_gcm_128bit_tag_t* p_in_mac);
sgx_status_t change_mode(sgx_enclave_id_t eid);
sgx_status_t nextchar_decrypted(sgx_enclave_id_t eid);
sgx_status_t nextchar_encrypted(sgx_enclave_id_t eid);
sgx_status_t seal(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* plaintext, size_t plaintext_len, sgx_sealed_data_t* sealed_data, size_t sealed_size);
sgx_status_t unseal(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_sealed_data_t* sealed_data, size_t sealed_size, uint8_t* plaintext, uint32_t plaintext_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
