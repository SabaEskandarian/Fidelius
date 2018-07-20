#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_generate_random_number_t {
	int ms_retval;
	int ms_one;
} ms_generate_random_number_t;

typedef struct ms_gcm_decrypt_t {
	uint8_t* ms_p_src;
	uint32_t ms_src_len;
	uint8_t* ms_p_dst;
	uint8_t* ms_p_iv;
	sgx_aes_gcm_128bit_tag_t* ms_p_in_mac;
} ms_gcm_decrypt_t;

typedef struct ms_seal_t {
	sgx_status_t ms_retval;
	uint8_t* ms_plaintext;
	size_t ms_plaintext_len;
	sgx_sealed_data_t* ms_sealed_data;
	size_t ms_sealed_size;
} ms_seal_t;

typedef struct ms_unseal_t {
	sgx_status_t ms_retval;
	sgx_sealed_data_t* ms_sealed_data;
	size_t ms_sealed_size;
	uint8_t* ms_plaintext;
	uint32_t ms_plaintext_len;
} ms_unseal_t;

typedef struct ms_ocall_print_t {
	char* ms_str;
} ms_ocall_print_t;

typedef struct ms_ocall_getKeyBoardInput_t {
	uint8_t* ms_kb;
	uint8_t* ms_p_src;
	uint8_t* ms_tag;
	uint8_t* ms_p_iv;
} ms_ocall_getKeyBoardInput_t;

typedef struct ms_ocall_getPointerToKb_t {
	uint8_t** ms_kb;
} ms_ocall_getPointerToKb_t;

static sgx_status_t SGX_CDECL Enclave_ocall_print(void* pms)
{
	ms_ocall_print_t* ms = SGX_CAST(ms_ocall_print_t*, pms);
	ocall_print((const char*)ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_getKeyBoardInput(void* pms)
{
	ms_ocall_getKeyBoardInput_t* ms = SGX_CAST(ms_ocall_getKeyBoardInput_t*, pms);
	ocall_getKeyBoardInput(ms->ms_kb, ms->ms_p_src, ms->ms_tag, ms->ms_p_iv);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_getPointerToKb(void* pms)
{
	ms_ocall_getPointerToKb_t* ms = SGX_CAST(ms_ocall_getPointerToKb_t*, pms);
	ocall_getPointerToKb(ms->ms_kb);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[3];
} ocall_table_Enclave = {
	3,
	{
		(void*)Enclave_ocall_print,
		(void*)Enclave_ocall_getKeyBoardInput,
		(void*)Enclave_ocall_getPointerToKb,
	}
};
sgx_status_t generate_random_number(sgx_enclave_id_t eid, int* retval, int one)
{
	sgx_status_t status;
	ms_generate_random_number_t ms;
	ms.ms_one = one;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t hardcoded(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t gcm_decrypt(sgx_enclave_id_t eid, uint8_t* p_src, uint32_t src_len, uint8_t* p_dst, uint8_t* p_iv, sgx_aes_gcm_128bit_tag_t* p_in_mac)
{
	sgx_status_t status;
	ms_gcm_decrypt_t ms;
	ms.ms_p_src = p_src;
	ms.ms_src_len = src_len;
	ms.ms_p_dst = p_dst;
	ms.ms_p_iv = p_iv;
	ms.ms_p_in_mac = p_in_mac;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t change_mode(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t nextchar_decrypted(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 4, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t nextchar_encrypted(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 5, &ocall_table_Enclave, NULL);
	return status;
}

sgx_status_t seal(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* plaintext, size_t plaintext_len, sgx_sealed_data_t* sealed_data, size_t sealed_size)
{
	sgx_status_t status;
	ms_seal_t ms;
	ms.ms_plaintext = plaintext;
	ms.ms_plaintext_len = plaintext_len;
	ms.ms_sealed_data = sealed_data;
	ms.ms_sealed_size = sealed_size;
	status = sgx_ecall(eid, 6, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t unseal(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_sealed_data_t* sealed_data, size_t sealed_size, uint8_t* plaintext, uint32_t plaintext_len)
{
	sgx_status_t status;
	ms_unseal_t ms;
	ms.ms_sealed_data = sealed_data;
	ms.ms_sealed_size = sealed_size;
	ms.ms_plaintext = plaintext;
	ms.ms_plaintext_len = plaintext_len;
	status = sgx_ecall(eid, 7, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

