#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <string.h> /* for memcpy etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)


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

static sgx_status_t SGX_CDECL sgx_generate_random_number(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_generate_random_number_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_generate_random_number_t* ms = SGX_CAST(ms_generate_random_number_t*, pms);
	sgx_status_t status = SGX_SUCCESS;



	ms->ms_retval = generate_random_number(ms->ms_one);


	return status;
}

static sgx_status_t SGX_CDECL sgx_hardcoded(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	hardcoded();
	return status;
}

static sgx_status_t SGX_CDECL sgx_gcm_decrypt(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_gcm_decrypt_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_gcm_decrypt_t* ms = SGX_CAST(ms_gcm_decrypt_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_p_src = ms->ms_p_src;
	uint8_t* _tmp_p_dst = ms->ms_p_dst;
	size_t _len_p_dst = 1000;
	uint8_t* _in_p_dst = NULL;
	uint8_t* _tmp_p_iv = ms->ms_p_iv;
	size_t _len_p_iv = sizeof(*_tmp_p_iv);
	uint8_t* _in_p_iv = NULL;
	sgx_aes_gcm_128bit_tag_t* _tmp_p_in_mac = ms->ms_p_in_mac;
	size_t _len_p_in_mac = sizeof(*_tmp_p_in_mac);
	sgx_aes_gcm_128bit_tag_t* _in_p_in_mac = NULL;

	CHECK_UNIQUE_POINTER(_tmp_p_dst, _len_p_dst);
	CHECK_UNIQUE_POINTER(_tmp_p_iv, _len_p_iv);
	CHECK_UNIQUE_POINTER(_tmp_p_in_mac, _len_p_in_mac);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_dst != NULL && _len_p_dst != 0) {
		_in_p_dst = (uint8_t*)malloc(_len_p_dst);
		if (_in_p_dst == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_p_dst, _tmp_p_dst, _len_p_dst);
	}
	if (_tmp_p_iv != NULL && _len_p_iv != 0) {
		_in_p_iv = (uint8_t*)malloc(_len_p_iv);
		if (_in_p_iv == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_p_iv, _tmp_p_iv, _len_p_iv);
	}
	if (_tmp_p_in_mac != NULL && _len_p_in_mac != 0) {
		_in_p_in_mac = (sgx_aes_gcm_128bit_tag_t*)malloc(_len_p_in_mac);
		if (_in_p_in_mac == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_p_in_mac, _tmp_p_in_mac, _len_p_in_mac);
	}

	gcm_decrypt(_tmp_p_src, ms->ms_src_len, _in_p_dst, _in_p_iv, _in_p_in_mac);
err:
	if (_in_p_dst) free(_in_p_dst);
	if (_in_p_iv) free(_in_p_iv);
	if (_in_p_in_mac) free(_in_p_in_mac);

	return status;
}

static sgx_status_t SGX_CDECL sgx_change_mode(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	change_mode();
	return status;
}

static sgx_status_t SGX_CDECL sgx_nextchar_decrypted(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	nextchar_decrypted();
	return status;
}

static sgx_status_t SGX_CDECL sgx_nextchar_encrypted(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	nextchar_encrypted();
	return status;
}

static sgx_status_t SGX_CDECL sgx_seal(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_seal_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_seal_t* ms = SGX_CAST(ms_seal_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_plaintext = ms->ms_plaintext;
	size_t _tmp_plaintext_len = ms->ms_plaintext_len;
	size_t _len_plaintext = _tmp_plaintext_len;
	uint8_t* _in_plaintext = NULL;
	sgx_sealed_data_t* _tmp_sealed_data = ms->ms_sealed_data;
	size_t _tmp_sealed_size = ms->ms_sealed_size;
	size_t _len_sealed_data = _tmp_sealed_size;
	sgx_sealed_data_t* _in_sealed_data = NULL;

	CHECK_UNIQUE_POINTER(_tmp_plaintext, _len_plaintext);
	CHECK_UNIQUE_POINTER(_tmp_sealed_data, _len_sealed_data);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_plaintext != NULL && _len_plaintext != 0) {
		_in_plaintext = (uint8_t*)malloc(_len_plaintext);
		if (_in_plaintext == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_plaintext, _tmp_plaintext, _len_plaintext);
	}
	if (_tmp_sealed_data != NULL && _len_sealed_data != 0) {
		if ((_in_sealed_data = (sgx_sealed_data_t*)malloc(_len_sealed_data)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_sealed_data, 0, _len_sealed_data);
	}

	ms->ms_retval = seal(_in_plaintext, _tmp_plaintext_len, _in_sealed_data, _tmp_sealed_size);
err:
	if (_in_plaintext) free(_in_plaintext);
	if (_in_sealed_data) {
		memcpy(_tmp_sealed_data, _in_sealed_data, _len_sealed_data);
		free(_in_sealed_data);
	}

	return status;
}

static sgx_status_t SGX_CDECL sgx_unseal(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_unseal_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_unseal_t* ms = SGX_CAST(ms_unseal_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	sgx_sealed_data_t* _tmp_sealed_data = ms->ms_sealed_data;
	size_t _tmp_sealed_size = ms->ms_sealed_size;
	size_t _len_sealed_data = _tmp_sealed_size;
	sgx_sealed_data_t* _in_sealed_data = NULL;
	uint8_t* _tmp_plaintext = ms->ms_plaintext;
	uint32_t _tmp_plaintext_len = ms->ms_plaintext_len;
	size_t _len_plaintext = _tmp_plaintext_len;
	uint8_t* _in_plaintext = NULL;

	CHECK_UNIQUE_POINTER(_tmp_sealed_data, _len_sealed_data);
	CHECK_UNIQUE_POINTER(_tmp_plaintext, _len_plaintext);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_sealed_data != NULL && _len_sealed_data != 0) {
		_in_sealed_data = (sgx_sealed_data_t*)malloc(_len_sealed_data);
		if (_in_sealed_data == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_sealed_data, _tmp_sealed_data, _len_sealed_data);
	}
	if (_tmp_plaintext != NULL && _len_plaintext != 0) {
		if ((_in_plaintext = (uint8_t*)malloc(_len_plaintext)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_plaintext, 0, _len_plaintext);
	}

	ms->ms_retval = unseal(_in_sealed_data, _tmp_sealed_size, _in_plaintext, _tmp_plaintext_len);
err:
	if (_in_sealed_data) free(_in_sealed_data);
	if (_in_plaintext) {
		memcpy(_tmp_plaintext, _in_plaintext, _len_plaintext);
		free(_in_plaintext);
	}

	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv;} ecall_table[8];
} g_ecall_table = {
	8,
	{
		{(void*)(uintptr_t)sgx_generate_random_number, 0},
		{(void*)(uintptr_t)sgx_hardcoded, 0},
		{(void*)(uintptr_t)sgx_gcm_decrypt, 0},
		{(void*)(uintptr_t)sgx_change_mode, 0},
		{(void*)(uintptr_t)sgx_nextchar_decrypted, 0},
		{(void*)(uintptr_t)sgx_nextchar_encrypted, 0},
		{(void*)(uintptr_t)sgx_seal, 0},
		{(void*)(uintptr_t)sgx_unseal, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[3][8];
} g_dyn_entry_table = {
	3,
	{
		{0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_print(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_print_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_t);
	void *__tmp = NULL;

	ocalloc_size += (str != NULL && sgx_is_within_enclave(str, _len_str)) ? _len_str : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_t));

	if (str != NULL && sgx_is_within_enclave(str, _len_str)) {
		ms->ms_str = (char*)__tmp;
		memcpy(__tmp, str, _len_str);
		__tmp = (void *)((size_t)__tmp + _len_str);
	} else if (str == NULL) {
		ms->ms_str = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_getKeyBoardInput(uint8_t* kb, uint8_t* p_src, uint8_t* tag, uint8_t* p_iv)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_p_src = 1;
	size_t _len_tag = 16;
	size_t _len_p_iv = 12;

	ms_ocall_getKeyBoardInput_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_getKeyBoardInput_t);
	void *__tmp = NULL;

	void *__tmp_p_src = NULL;
	void *__tmp_tag = NULL;
	void *__tmp_p_iv = NULL;
	ocalloc_size += (p_src != NULL && sgx_is_within_enclave(p_src, _len_p_src)) ? _len_p_src : 0;
	ocalloc_size += (tag != NULL && sgx_is_within_enclave(tag, _len_tag)) ? _len_tag : 0;
	ocalloc_size += (p_iv != NULL && sgx_is_within_enclave(p_iv, _len_p_iv)) ? _len_p_iv : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_getKeyBoardInput_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_getKeyBoardInput_t));

	ms->ms_kb = SGX_CAST(uint8_t*, kb);
	if (p_src != NULL && sgx_is_within_enclave(p_src, _len_p_src)) {
		ms->ms_p_src = (uint8_t*)__tmp;
		__tmp_p_src = __tmp;
		memset(__tmp_p_src, 0, _len_p_src);
		__tmp = (void *)((size_t)__tmp + _len_p_src);
	} else if (p_src == NULL) {
		ms->ms_p_src = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (tag != NULL && sgx_is_within_enclave(tag, _len_tag)) {
		ms->ms_tag = (uint8_t*)__tmp;
		__tmp_tag = __tmp;
		memset(__tmp_tag, 0, _len_tag);
		__tmp = (void *)((size_t)__tmp + _len_tag);
	} else if (tag == NULL) {
		ms->ms_tag = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (p_iv != NULL && sgx_is_within_enclave(p_iv, _len_p_iv)) {
		ms->ms_p_iv = (uint8_t*)__tmp;
		__tmp_p_iv = __tmp;
		memset(__tmp_p_iv, 0, _len_p_iv);
		__tmp = (void *)((size_t)__tmp + _len_p_iv);
	} else if (p_iv == NULL) {
		ms->ms_p_iv = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(1, ms);

	if (status == SGX_SUCCESS) {
		if (p_src) memcpy((void*)p_src, __tmp_p_src, _len_p_src);
		if (tag) memcpy((void*)tag, __tmp_tag, _len_tag);
		if (p_iv) memcpy((void*)p_iv, __tmp_p_iv, _len_p_iv);
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_getPointerToKb(uint8_t** kb)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_kb = sizeof(*kb);

	ms_ocall_getPointerToKb_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_getPointerToKb_t);
	void *__tmp = NULL;

	void *__tmp_kb = NULL;
	ocalloc_size += (kb != NULL && sgx_is_within_enclave(kb, _len_kb)) ? _len_kb : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_getPointerToKb_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_getPointerToKb_t));

	if (kb != NULL && sgx_is_within_enclave(kb, _len_kb)) {
		ms->ms_kb = (uint8_t**)__tmp;
		__tmp_kb = __tmp;
		memset(__tmp_kb, 0, _len_kb);
		__tmp = (void *)((size_t)__tmp + _len_kb);
	} else if (kb == NULL) {
		ms->ms_kb = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(2, ms);

	if (status == SGX_SUCCESS) {
		if (kb) memcpy((void*)kb, __tmp_kb, _len_kb);
	}
	sgx_ocfree();
	return status;
}

