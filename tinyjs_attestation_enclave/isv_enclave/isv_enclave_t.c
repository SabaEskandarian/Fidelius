#include "isv_enclave_t.h"

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


typedef struct ms_enclave_init_ra_t {
	sgx_status_t ms_retval;
	int ms_b_pse;
	sgx_ra_context_t* ms_p_context;
} ms_enclave_init_ra_t;

typedef struct ms_enclave_ra_close_t {
	sgx_status_t ms_retval;
	sgx_ra_context_t ms_context;
} ms_enclave_ra_close_t;

typedef struct ms_verify_att_result_mac_t {
	sgx_status_t ms_retval;
	sgx_ra_context_t ms_context;
	uint8_t* ms_message;
	size_t ms_message_size;
	uint8_t* ms_mac;
	size_t ms_mac_size;
} ms_verify_att_result_mac_t;

typedef struct ms_put_secret_data_t {
	sgx_status_t ms_retval;
	sgx_ra_context_t ms_context;
	uint8_t* ms_p_secret;
	uint32_t ms_secret_size;
	uint8_t* ms_gcm_mac;
} ms_put_secret_data_t;

typedef struct ms_get_mac_key_t {
	sgx_status_t ms_retval;
	uint8_t* ms_p_mac;
	uint32_t ms_mac_size;
	uint8_t* ms_gcm_mac;
} ms_get_mac_key_t;

typedef struct ms_run_js_t {
	sgx_status_t ms_retval;
	char* ms_code;
	size_t ms_len;
} ms_run_js_t;

typedef struct ms_add_form_t {
	sgx_status_t ms_retval;
	char* ms_name;
	size_t ms_len;
	char* ms_origin;
	size_t ms_origin_len;
	uint16_t ms_x;
	uint16_t ms_y;
} ms_add_form_t;

typedef struct ms_add_input_t {
	sgx_status_t ms_retval;
	char* ms_name;
	size_t ms_len1;
	char* ms_input_i;
	size_t ms_len2;
	uint8_t* ms_sig_form;
	size_t ms_sig_form_size;
	int ms_validate;
	uint16_t ms_x;
	uint16_t ms_y;
	uint16_t ms_height;
	uint16_t ms_width;
} ms_add_input_t;

typedef struct ms_onFocus_t {
	sgx_status_t ms_retval;
	char* ms_formName;
	size_t ms_formName_len;
	char* ms_inputName;
	size_t ms_inputName_len;
	uint16_t ms_x;
	uint16_t ms_y;
	uint16_t ms_width;
	uint16_t ms_height;
} ms_onFocus_t;

typedef struct ms_onBlur_t {
	sgx_status_t ms_retval;
} ms_onBlur_t;

typedef struct ms_form_len_t {
	uint32_t ms_retval;
	char* ms_formName;
	size_t ms_formName_len;
} ms_form_len_t;

typedef struct ms_submit_form_t {
	sgx_status_t ms_retval;
	char* ms_formName;
	size_t ms_formName_len;
	uint8_t* ms_dest;
	uint32_t ms_encr_size;
	uint8_t* ms_gcm_mac;
} ms_submit_form_t;

typedef struct ms_gcm_decrypt_t {
	sgx_status_t ms_retval;
	uint8_t* ms_p_src;
	uint32_t ms_src_len;
	uint8_t* ms_p_dst;
	uint8_t* ms_p_iv;
	sgx_aes_gcm_128bit_tag_t* ms_p_in_mac;
} ms_gcm_decrypt_t;

typedef struct ms_get_keyboard_chars_t {
	sgx_status_t ms_retval;
	uint8_t* ms_p_src;
} ms_get_keyboard_chars_t;

typedef struct ms_test_decryption_t {
	sgx_status_t ms_retval;
	uint8_t* ms_form;
	uint32_t ms_form_size;
	uint8_t* ms_gcm_mac;
} ms_test_decryption_t;

typedef struct ms_get_http_response_t {
	char* ms_response;
	size_t ms_len;
} ms_get_http_response_t;

typedef struct ms_print_debug_t {
	int ms_retval;
} ms_print_debug_t;

typedef struct ms_sgx_ra_get_ga_t {
	sgx_status_t ms_retval;
	sgx_ra_context_t ms_context;
	sgx_ec256_public_t* ms_g_a;
} ms_sgx_ra_get_ga_t;

typedef struct ms_sgx_ra_proc_msg2_trusted_t {
	sgx_status_t ms_retval;
	sgx_ra_context_t ms_context;
	sgx_ra_msg2_t* ms_p_msg2;
	sgx_target_info_t* ms_p_qe_target;
	sgx_report_t* ms_p_report;
	sgx_quote_nonce_t* ms_p_nonce;
} ms_sgx_ra_proc_msg2_trusted_t;

typedef struct ms_sgx_ra_get_msg3_trusted_t {
	sgx_status_t ms_retval;
	sgx_ra_context_t ms_context;
	uint32_t ms_quote_size;
	sgx_report_t* ms_qe_report;
	sgx_ra_msg3_t* ms_p_msg3;
	uint32_t ms_msg3_size;
} ms_sgx_ra_get_msg3_trusted_t;

typedef struct ms_create_add_overlay_msg_t {
	uint8_t* ms_output;
	uint32_t* ms_out_len;
	char* ms_form_id;
	size_t ms_form_id_len;
} ms_create_add_overlay_msg_t;

typedef struct ms_create_remove_overlay_msg_t {
	uint8_t* ms_output;
	uint32_t* ms_out_len;
	char* ms_form_id;
	size_t ms_form_id_len;
} ms_create_remove_overlay_msg_t;

typedef struct ms_ocall_print_string_t {
	char* ms_str;
} ms_ocall_print_string_t;

typedef struct ms_enc_make_http_request_t {
	sgx_status_t ms_retval;
	char* ms_method;
	char* ms_url;
	char* ms_headers;
	char* ms_request_data;
	uint8_t* ms_p_mac;
	int* ms_ret_code;
} ms_enc_make_http_request_t;

typedef struct ms_create_session_ocall_t {
	sgx_status_t ms_retval;
	uint32_t* ms_sid;
	uint8_t* ms_dh_msg1;
	uint32_t ms_dh_msg1_size;
	uint32_t ms_timeout;
} ms_create_session_ocall_t;

typedef struct ms_exchange_report_ocall_t {
	sgx_status_t ms_retval;
	uint32_t ms_sid;
	uint8_t* ms_dh_msg2;
	uint32_t ms_dh_msg2_size;
	uint8_t* ms_dh_msg3;
	uint32_t ms_dh_msg3_size;
	uint32_t ms_timeout;
} ms_exchange_report_ocall_t;

typedef struct ms_close_session_ocall_t {
	sgx_status_t ms_retval;
	uint32_t ms_sid;
	uint32_t ms_timeout;
} ms_close_session_ocall_t;

typedef struct ms_invoke_service_ocall_t {
	sgx_status_t ms_retval;
	uint8_t* ms_pse_message_req;
	uint32_t ms_pse_message_req_size;
	uint8_t* ms_pse_message_resp;
	uint32_t ms_pse_message_resp_size;
	uint32_t ms_timeout;
} ms_invoke_service_ocall_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	void* ms_waiter;
	void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

static sgx_status_t SGX_CDECL sgx_enclave_init_ra(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_enclave_init_ra_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_enclave_init_ra_t* ms = SGX_CAST(ms_enclave_init_ra_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	sgx_ra_context_t* _tmp_p_context = ms->ms_p_context;
	size_t _len_p_context = sizeof(*_tmp_p_context);
	sgx_ra_context_t* _in_p_context = NULL;

	CHECK_UNIQUE_POINTER(_tmp_p_context, _len_p_context);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_context != NULL && _len_p_context != 0) {
		if ((_in_p_context = (sgx_ra_context_t*)malloc(_len_p_context)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_p_context, 0, _len_p_context);
	}

	ms->ms_retval = enclave_init_ra(ms->ms_b_pse, _in_p_context);
err:
	if (_in_p_context) {
		memcpy(_tmp_p_context, _in_p_context, _len_p_context);
		free(_in_p_context);
	}

	return status;
}

static sgx_status_t SGX_CDECL sgx_enclave_ra_close(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_enclave_ra_close_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_enclave_ra_close_t* ms = SGX_CAST(ms_enclave_ra_close_t*, pms);
	sgx_status_t status = SGX_SUCCESS;



	ms->ms_retval = enclave_ra_close(ms->ms_context);


	return status;
}

static sgx_status_t SGX_CDECL sgx_verify_att_result_mac(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_verify_att_result_mac_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_verify_att_result_mac_t* ms = SGX_CAST(ms_verify_att_result_mac_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_message = ms->ms_message;
	size_t _tmp_message_size = ms->ms_message_size;
	size_t _len_message = _tmp_message_size;
	uint8_t* _in_message = NULL;
	uint8_t* _tmp_mac = ms->ms_mac;
	size_t _tmp_mac_size = ms->ms_mac_size;
	size_t _len_mac = _tmp_mac_size;
	uint8_t* _in_mac = NULL;

	CHECK_UNIQUE_POINTER(_tmp_message, _len_message);
	CHECK_UNIQUE_POINTER(_tmp_mac, _len_mac);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_message != NULL && _len_message != 0) {
		_in_message = (uint8_t*)malloc(_len_message);
		if (_in_message == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_message, _tmp_message, _len_message);
	}
	if (_tmp_mac != NULL && _len_mac != 0) {
		_in_mac = (uint8_t*)malloc(_len_mac);
		if (_in_mac == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_mac, _tmp_mac, _len_mac);
	}

	ms->ms_retval = verify_att_result_mac(ms->ms_context, _in_message, _tmp_message_size, _in_mac, _tmp_mac_size);
err:
	if (_in_message) free(_in_message);
	if (_in_mac) free(_in_mac);

	return status;
}

static sgx_status_t SGX_CDECL sgx_put_secret_data(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_put_secret_data_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_put_secret_data_t* ms = SGX_CAST(ms_put_secret_data_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_p_secret = ms->ms_p_secret;
	uint32_t _tmp_secret_size = ms->ms_secret_size;
	size_t _len_p_secret = _tmp_secret_size;
	uint8_t* _in_p_secret = NULL;
	uint8_t* _tmp_gcm_mac = ms->ms_gcm_mac;
	size_t _len_gcm_mac = 16 * sizeof(*_tmp_gcm_mac);
	uint8_t* _in_gcm_mac = NULL;

	if (sizeof(*_tmp_gcm_mac) != 0 &&
		16 > (SIZE_MAX / sizeof(*_tmp_gcm_mac))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_p_secret, _len_p_secret);
	CHECK_UNIQUE_POINTER(_tmp_gcm_mac, _len_gcm_mac);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_secret != NULL && _len_p_secret != 0) {
		_in_p_secret = (uint8_t*)malloc(_len_p_secret);
		if (_in_p_secret == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_p_secret, _tmp_p_secret, _len_p_secret);
	}
	if (_tmp_gcm_mac != NULL && _len_gcm_mac != 0) {
		_in_gcm_mac = (uint8_t*)malloc(_len_gcm_mac);
		if (_in_gcm_mac == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_gcm_mac, _tmp_gcm_mac, _len_gcm_mac);
	}

	ms->ms_retval = put_secret_data(ms->ms_context, _in_p_secret, _tmp_secret_size, _in_gcm_mac);
err:
	if (_in_p_secret) free(_in_p_secret);
	if (_in_gcm_mac) free(_in_gcm_mac);

	return status;
}

static sgx_status_t SGX_CDECL sgx_get_mac_key(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_get_mac_key_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_get_mac_key_t* ms = SGX_CAST(ms_get_mac_key_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_p_mac = ms->ms_p_mac;
	uint32_t _tmp_mac_size = ms->ms_mac_size;
	size_t _len_p_mac = _tmp_mac_size;
	uint8_t* _in_p_mac = NULL;
	uint8_t* _tmp_gcm_mac = ms->ms_gcm_mac;
	size_t _len_gcm_mac = 16 * sizeof(*_tmp_gcm_mac);
	uint8_t* _in_gcm_mac = NULL;

	if (sizeof(*_tmp_gcm_mac) != 0 &&
		16 > (SIZE_MAX / sizeof(*_tmp_gcm_mac))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_p_mac, _len_p_mac);
	CHECK_UNIQUE_POINTER(_tmp_gcm_mac, _len_gcm_mac);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_mac != NULL && _len_p_mac != 0) {
		_in_p_mac = (uint8_t*)malloc(_len_p_mac);
		if (_in_p_mac == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_p_mac, _tmp_p_mac, _len_p_mac);
	}
	if (_tmp_gcm_mac != NULL && _len_gcm_mac != 0) {
		_in_gcm_mac = (uint8_t*)malloc(_len_gcm_mac);
		if (_in_gcm_mac == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_gcm_mac, _tmp_gcm_mac, _len_gcm_mac);
	}

	ms->ms_retval = get_mac_key(_in_p_mac, _tmp_mac_size, _in_gcm_mac);
err:
	if (_in_p_mac) free(_in_p_mac);
	if (_in_gcm_mac) free(_in_gcm_mac);

	return status;
}

static sgx_status_t SGX_CDECL sgx_run_js(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_run_js_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_run_js_t* ms = SGX_CAST(ms_run_js_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_code = ms->ms_code;
	size_t _tmp_len = ms->ms_len;
	size_t _len_code = _tmp_len;
	char* _in_code = NULL;

	CHECK_UNIQUE_POINTER(_tmp_code, _len_code);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_code != NULL && _len_code != 0) {
		_in_code = (char*)malloc(_len_code);
		if (_in_code == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_code, _tmp_code, _len_code);
	}

	ms->ms_retval = run_js(_in_code, _tmp_len);
err:
	if (_in_code) {
		memcpy(_tmp_code, _in_code, _len_code);
		free(_in_code);
	}

	return status;
}

static sgx_status_t SGX_CDECL sgx_add_form(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_add_form_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_add_form_t* ms = SGX_CAST(ms_add_form_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_name = ms->ms_name;
	size_t _tmp_len = ms->ms_len;
	size_t _len_name = _tmp_len;
	char* _in_name = NULL;
	char* _tmp_origin = ms->ms_origin;
	size_t _tmp_origin_len = ms->ms_origin_len;
	size_t _len_origin = _tmp_origin_len;
	char* _in_origin = NULL;

	CHECK_UNIQUE_POINTER(_tmp_name, _len_name);
	CHECK_UNIQUE_POINTER(_tmp_origin, _len_origin);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_name != NULL && _len_name != 0) {
		_in_name = (char*)malloc(_len_name);
		if (_in_name == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_name, _tmp_name, _len_name);
	}
	if (_tmp_origin != NULL && _len_origin != 0) {
		_in_origin = (char*)malloc(_len_origin);
		if (_in_origin == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_origin, _tmp_origin, _len_origin);
	}

	ms->ms_retval = add_form((const char*)_in_name, _tmp_len, (const char*)_in_origin, _tmp_origin_len, ms->ms_x, ms->ms_y);
err:
	if (_in_name) free((void*)_in_name);
	if (_in_origin) free((void*)_in_origin);

	return status;
}

static sgx_status_t SGX_CDECL sgx_add_input(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_add_input_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_add_input_t* ms = SGX_CAST(ms_add_input_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_name = ms->ms_name;
	size_t _tmp_len1 = ms->ms_len1;
	size_t _len_name = _tmp_len1;
	char* _in_name = NULL;
	char* _tmp_input_i = ms->ms_input_i;
	size_t _tmp_len2 = ms->ms_len2;
	size_t _len_input_i = _tmp_len2;
	char* _in_input_i = NULL;
	uint8_t* _tmp_sig_form = ms->ms_sig_form;
	size_t _tmp_sig_form_size = ms->ms_sig_form_size;
	size_t _len_sig_form = _tmp_sig_form_size;
	uint8_t* _in_sig_form = NULL;

	CHECK_UNIQUE_POINTER(_tmp_name, _len_name);
	CHECK_UNIQUE_POINTER(_tmp_input_i, _len_input_i);
	CHECK_UNIQUE_POINTER(_tmp_sig_form, _len_sig_form);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_name != NULL && _len_name != 0) {
		_in_name = (char*)malloc(_len_name);
		if (_in_name == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_name, _tmp_name, _len_name);
	}
	if (_tmp_input_i != NULL && _len_input_i != 0) {
		_in_input_i = (char*)malloc(_len_input_i);
		if (_in_input_i == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_input_i, _tmp_input_i, _len_input_i);
	}
	if (_tmp_sig_form != NULL && _len_sig_form != 0) {
		_in_sig_form = (uint8_t*)malloc(_len_sig_form);
		if (_in_sig_form == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_sig_form, _tmp_sig_form, _len_sig_form);
	}

	ms->ms_retval = add_input((const char*)_in_name, _tmp_len1, (const char*)_in_input_i, _tmp_len2, (const uint8_t*)_in_sig_form, _tmp_sig_form_size, ms->ms_validate, ms->ms_x, ms->ms_y, ms->ms_height, ms->ms_width);
err:
	if (_in_name) free((void*)_in_name);
	if (_in_input_i) free((void*)_in_input_i);
	if (_in_sig_form) free((void*)_in_sig_form);

	return status;
}

static sgx_status_t SGX_CDECL sgx_onFocus(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_onFocus_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_onFocus_t* ms = SGX_CAST(ms_onFocus_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_formName = ms->ms_formName;
	size_t _len_formName = ms->ms_formName_len ;
	char* _in_formName = NULL;
	char* _tmp_inputName = ms->ms_inputName;
	size_t _len_inputName = ms->ms_inputName_len ;
	char* _in_inputName = NULL;

	CHECK_UNIQUE_POINTER(_tmp_formName, _len_formName);
	CHECK_UNIQUE_POINTER(_tmp_inputName, _len_inputName);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_formName != NULL && _len_formName != 0) {
		_in_formName = (char*)malloc(_len_formName);
		if (_in_formName == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_formName, _tmp_formName, _len_formName);
		_in_formName[_len_formName - 1] = '\0';
		if (_len_formName != strlen(_in_formName) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_inputName != NULL && _len_inputName != 0) {
		_in_inputName = (char*)malloc(_len_inputName);
		if (_in_inputName == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_inputName, _tmp_inputName, _len_inputName);
		_in_inputName[_len_inputName - 1] = '\0';
		if (_len_inputName != strlen(_in_inputName) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

	ms->ms_retval = onFocus((const char*)_in_formName, (const char*)_in_inputName, ms->ms_x, ms->ms_y, ms->ms_width, ms->ms_height);
err:
	if (_in_formName) free((void*)_in_formName);
	if (_in_inputName) free((void*)_in_inputName);

	return status;
}

static sgx_status_t SGX_CDECL sgx_onBlur(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_onBlur_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_onBlur_t* ms = SGX_CAST(ms_onBlur_t*, pms);
	sgx_status_t status = SGX_SUCCESS;



	ms->ms_retval = onBlur();


	return status;
}

static sgx_status_t SGX_CDECL sgx_form_len(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_form_len_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_form_len_t* ms = SGX_CAST(ms_form_len_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_formName = ms->ms_formName;
	size_t _len_formName = ms->ms_formName_len ;
	char* _in_formName = NULL;

	CHECK_UNIQUE_POINTER(_tmp_formName, _len_formName);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_formName != NULL && _len_formName != 0) {
		_in_formName = (char*)malloc(_len_formName);
		if (_in_formName == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_formName, _tmp_formName, _len_formName);
		_in_formName[_len_formName - 1] = '\0';
		if (_len_formName != strlen(_in_formName) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

	ms->ms_retval = form_len((const char*)_in_formName);
err:
	if (_in_formName) free((void*)_in_formName);

	return status;
}

static sgx_status_t SGX_CDECL sgx_submit_form(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_submit_form_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_submit_form_t* ms = SGX_CAST(ms_submit_form_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_formName = ms->ms_formName;
	size_t _len_formName = ms->ms_formName_len ;
	char* _in_formName = NULL;
	uint8_t* _tmp_dest = ms->ms_dest;
	uint32_t _tmp_encr_size = ms->ms_encr_size;
	size_t _len_dest = _tmp_encr_size;
	uint8_t* _in_dest = NULL;
	uint8_t* _tmp_gcm_mac = ms->ms_gcm_mac;
	size_t _len_gcm_mac = 16 * sizeof(*_tmp_gcm_mac);
	uint8_t* _in_gcm_mac = NULL;

	if (sizeof(*_tmp_gcm_mac) != 0 &&
		16 > (SIZE_MAX / sizeof(*_tmp_gcm_mac))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_formName, _len_formName);
	CHECK_UNIQUE_POINTER(_tmp_dest, _len_dest);
	CHECK_UNIQUE_POINTER(_tmp_gcm_mac, _len_gcm_mac);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_formName != NULL && _len_formName != 0) {
		_in_formName = (char*)malloc(_len_formName);
		if (_in_formName == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_formName, _tmp_formName, _len_formName);
		_in_formName[_len_formName - 1] = '\0';
		if (_len_formName != strlen(_in_formName) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_dest != NULL && _len_dest != 0) {
		if ((_in_dest = (uint8_t*)malloc(_len_dest)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_dest, 0, _len_dest);
	}
	if (_tmp_gcm_mac != NULL && _len_gcm_mac != 0) {
		if ((_in_gcm_mac = (uint8_t*)malloc(_len_gcm_mac)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_gcm_mac, 0, _len_gcm_mac);
	}

	ms->ms_retval = submit_form((const char*)_in_formName, _in_dest, _tmp_encr_size, _in_gcm_mac);
err:
	if (_in_formName) free((void*)_in_formName);
	if (_in_dest) {
		memcpy(_tmp_dest, _in_dest, _len_dest);
		free(_in_dest);
	}
	if (_in_gcm_mac) {
		memcpy(_tmp_gcm_mac, _in_gcm_mac, _len_gcm_mac);
		free(_in_gcm_mac);
	}

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

	ms->ms_retval = gcm_decrypt(_tmp_p_src, ms->ms_src_len, _in_p_dst, _in_p_iv, _in_p_in_mac);
err:
	if (_in_p_dst) free(_in_p_dst);
	if (_in_p_iv) free(_in_p_iv);
	if (_in_p_in_mac) free(_in_p_in_mac);

	return status;
}

static sgx_status_t SGX_CDECL sgx_get_keyboard_chars(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_get_keyboard_chars_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_get_keyboard_chars_t* ms = SGX_CAST(ms_get_keyboard_chars_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_p_src = ms->ms_p_src;
	size_t _len_p_src = 29;
	uint8_t* _in_p_src = NULL;

	CHECK_UNIQUE_POINTER(_tmp_p_src, _len_p_src);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_src != NULL && _len_p_src != 0) {
		_in_p_src = (uint8_t*)malloc(_len_p_src);
		if (_in_p_src == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_p_src, _tmp_p_src, _len_p_src);
	}

	ms->ms_retval = get_keyboard_chars(_in_p_src);
err:
	if (_in_p_src) free(_in_p_src);

	return status;
}

static sgx_status_t SGX_CDECL sgx_test_decryption(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_test_decryption_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_test_decryption_t* ms = SGX_CAST(ms_test_decryption_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_form = ms->ms_form;
	uint32_t _tmp_form_size = ms->ms_form_size;
	size_t _len_form = _tmp_form_size;
	uint8_t* _in_form = NULL;
	uint8_t* _tmp_gcm_mac = ms->ms_gcm_mac;
	size_t _len_gcm_mac = 16 * sizeof(*_tmp_gcm_mac);
	uint8_t* _in_gcm_mac = NULL;

	if (sizeof(*_tmp_gcm_mac) != 0 &&
		16 > (SIZE_MAX / sizeof(*_tmp_gcm_mac))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_form, _len_form);
	CHECK_UNIQUE_POINTER(_tmp_gcm_mac, _len_gcm_mac);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_form != NULL && _len_form != 0) {
		_in_form = (uint8_t*)malloc(_len_form);
		if (_in_form == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_form, _tmp_form, _len_form);
	}
	if (_tmp_gcm_mac != NULL && _len_gcm_mac != 0) {
		_in_gcm_mac = (uint8_t*)malloc(_len_gcm_mac);
		if (_in_gcm_mac == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_gcm_mac, _tmp_gcm_mac, _len_gcm_mac);
	}

	ms->ms_retval = test_decryption(_in_form, _tmp_form_size, _in_gcm_mac);
err:
	if (_in_form) free(_in_form);
	if (_in_gcm_mac) free(_in_gcm_mac);

	return status;
}

static sgx_status_t SGX_CDECL sgx_get_http_response(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_get_http_response_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_get_http_response_t* ms = SGX_CAST(ms_get_http_response_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_response = ms->ms_response;
	size_t _tmp_len = ms->ms_len;
	size_t _len_response = _tmp_len;
	char* _in_response = NULL;

	CHECK_UNIQUE_POINTER(_tmp_response, _len_response);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_response != NULL && _len_response != 0) {
		_in_response = (char*)malloc(_len_response);
		if (_in_response == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_response, _tmp_response, _len_response);
	}

	get_http_response(_in_response, _tmp_len);
err:
	if (_in_response) free(_in_response);

	return status;
}

static sgx_status_t SGX_CDECL sgx_print_debug(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_print_debug_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_print_debug_t* ms = SGX_CAST(ms_print_debug_t*, pms);
	sgx_status_t status = SGX_SUCCESS;



	ms->ms_retval = print_debug();


	return status;
}

static sgx_status_t SGX_CDECL sgx_sgx_ra_get_ga(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_sgx_ra_get_ga_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_sgx_ra_get_ga_t* ms = SGX_CAST(ms_sgx_ra_get_ga_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	sgx_ec256_public_t* _tmp_g_a = ms->ms_g_a;
	size_t _len_g_a = sizeof(*_tmp_g_a);
	sgx_ec256_public_t* _in_g_a = NULL;

	CHECK_UNIQUE_POINTER(_tmp_g_a, _len_g_a);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_g_a != NULL && _len_g_a != 0) {
		if ((_in_g_a = (sgx_ec256_public_t*)malloc(_len_g_a)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_g_a, 0, _len_g_a);
	}

	ms->ms_retval = sgx_ra_get_ga(ms->ms_context, _in_g_a);
err:
	if (_in_g_a) {
		memcpy(_tmp_g_a, _in_g_a, _len_g_a);
		free(_in_g_a);
	}

	return status;
}

static sgx_status_t SGX_CDECL sgx_sgx_ra_proc_msg2_trusted(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_sgx_ra_proc_msg2_trusted_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_sgx_ra_proc_msg2_trusted_t* ms = SGX_CAST(ms_sgx_ra_proc_msg2_trusted_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	sgx_ra_msg2_t* _tmp_p_msg2 = ms->ms_p_msg2;
	size_t _len_p_msg2 = sizeof(*_tmp_p_msg2);
	sgx_ra_msg2_t* _in_p_msg2 = NULL;
	sgx_target_info_t* _tmp_p_qe_target = ms->ms_p_qe_target;
	size_t _len_p_qe_target = sizeof(*_tmp_p_qe_target);
	sgx_target_info_t* _in_p_qe_target = NULL;
	sgx_report_t* _tmp_p_report = ms->ms_p_report;
	size_t _len_p_report = sizeof(*_tmp_p_report);
	sgx_report_t* _in_p_report = NULL;
	sgx_quote_nonce_t* _tmp_p_nonce = ms->ms_p_nonce;
	size_t _len_p_nonce = sizeof(*_tmp_p_nonce);
	sgx_quote_nonce_t* _in_p_nonce = NULL;

	CHECK_UNIQUE_POINTER(_tmp_p_msg2, _len_p_msg2);
	CHECK_UNIQUE_POINTER(_tmp_p_qe_target, _len_p_qe_target);
	CHECK_UNIQUE_POINTER(_tmp_p_report, _len_p_report);
	CHECK_UNIQUE_POINTER(_tmp_p_nonce, _len_p_nonce);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_msg2 != NULL && _len_p_msg2 != 0) {
		_in_p_msg2 = (sgx_ra_msg2_t*)malloc(_len_p_msg2);
		if (_in_p_msg2 == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_p_msg2, _tmp_p_msg2, _len_p_msg2);
	}
	if (_tmp_p_qe_target != NULL && _len_p_qe_target != 0) {
		_in_p_qe_target = (sgx_target_info_t*)malloc(_len_p_qe_target);
		if (_in_p_qe_target == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_p_qe_target, _tmp_p_qe_target, _len_p_qe_target);
	}
	if (_tmp_p_report != NULL && _len_p_report != 0) {
		if ((_in_p_report = (sgx_report_t*)malloc(_len_p_report)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_p_report, 0, _len_p_report);
	}
	if (_tmp_p_nonce != NULL && _len_p_nonce != 0) {
		if ((_in_p_nonce = (sgx_quote_nonce_t*)malloc(_len_p_nonce)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_p_nonce, 0, _len_p_nonce);
	}

	ms->ms_retval = sgx_ra_proc_msg2_trusted(ms->ms_context, (const sgx_ra_msg2_t*)_in_p_msg2, (const sgx_target_info_t*)_in_p_qe_target, _in_p_report, _in_p_nonce);
err:
	if (_in_p_msg2) free((void*)_in_p_msg2);
	if (_in_p_qe_target) free((void*)_in_p_qe_target);
	if (_in_p_report) {
		memcpy(_tmp_p_report, _in_p_report, _len_p_report);
		free(_in_p_report);
	}
	if (_in_p_nonce) {
		memcpy(_tmp_p_nonce, _in_p_nonce, _len_p_nonce);
		free(_in_p_nonce);
	}

	return status;
}

static sgx_status_t SGX_CDECL sgx_sgx_ra_get_msg3_trusted(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_sgx_ra_get_msg3_trusted_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_sgx_ra_get_msg3_trusted_t* ms = SGX_CAST(ms_sgx_ra_get_msg3_trusted_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	sgx_report_t* _tmp_qe_report = ms->ms_qe_report;
	size_t _len_qe_report = sizeof(*_tmp_qe_report);
	sgx_report_t* _in_qe_report = NULL;
	sgx_ra_msg3_t* _tmp_p_msg3 = ms->ms_p_msg3;

	CHECK_UNIQUE_POINTER(_tmp_qe_report, _len_qe_report);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_qe_report != NULL && _len_qe_report != 0) {
		_in_qe_report = (sgx_report_t*)malloc(_len_qe_report);
		if (_in_qe_report == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_qe_report, _tmp_qe_report, _len_qe_report);
	}

	ms->ms_retval = sgx_ra_get_msg3_trusted(ms->ms_context, ms->ms_quote_size, _in_qe_report, _tmp_p_msg3, ms->ms_msg3_size);
err:
	if (_in_qe_report) free(_in_qe_report);

	return status;
}

static sgx_status_t SGX_CDECL sgx_create_add_overlay_msg(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_create_add_overlay_msg_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_create_add_overlay_msg_t* ms = SGX_CAST(ms_create_add_overlay_msg_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_output = ms->ms_output;
	size_t _len_output = 524288;
	uint8_t* _in_output = NULL;
	uint32_t* _tmp_out_len = ms->ms_out_len;
	size_t _len_out_len = sizeof(*_tmp_out_len);
	uint32_t* _in_out_len = NULL;
	char* _tmp_form_id = ms->ms_form_id;
	size_t _len_form_id = ms->ms_form_id_len ;
	char* _in_form_id = NULL;

	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_out_len, _len_out_len);
	CHECK_UNIQUE_POINTER(_tmp_form_id, _len_form_id);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_output != NULL && _len_output != 0) {
		if ((_in_output = (uint8_t*)malloc(_len_output)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_output, 0, _len_output);
	}
	if (_tmp_out_len != NULL && _len_out_len != 0) {
		if ((_in_out_len = (uint32_t*)malloc(_len_out_len)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_out_len, 0, _len_out_len);
	}
	if (_tmp_form_id != NULL && _len_form_id != 0) {
		_in_form_id = (char*)malloc(_len_form_id);
		if (_in_form_id == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_form_id, _tmp_form_id, _len_form_id);
		_in_form_id[_len_form_id - 1] = '\0';
		if (_len_form_id != strlen(_in_form_id) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

	create_add_overlay_msg(_in_output, _in_out_len, (const char*)_in_form_id);
err:
	if (_in_output) {
		memcpy(_tmp_output, _in_output, _len_output);
		free(_in_output);
	}
	if (_in_out_len) {
		memcpy(_tmp_out_len, _in_out_len, _len_out_len);
		free(_in_out_len);
	}
	if (_in_form_id) free((void*)_in_form_id);

	return status;
}

static sgx_status_t SGX_CDECL sgx_create_remove_overlay_msg(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_create_remove_overlay_msg_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_create_remove_overlay_msg_t* ms = SGX_CAST(ms_create_remove_overlay_msg_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	uint8_t* _tmp_output = ms->ms_output;
	size_t _len_output = 524288;
	uint8_t* _in_output = NULL;
	uint32_t* _tmp_out_len = ms->ms_out_len;
	size_t _len_out_len = sizeof(*_tmp_out_len);
	uint32_t* _in_out_len = NULL;
	char* _tmp_form_id = ms->ms_form_id;
	size_t _len_form_id = ms->ms_form_id_len ;
	char* _in_form_id = NULL;

	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_out_len, _len_out_len);
	CHECK_UNIQUE_POINTER(_tmp_form_id, _len_form_id);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_output != NULL && _len_output != 0) {
		if ((_in_output = (uint8_t*)malloc(_len_output)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_output, 0, _len_output);
	}
	if (_tmp_out_len != NULL && _len_out_len != 0) {
		if ((_in_out_len = (uint32_t*)malloc(_len_out_len)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_out_len, 0, _len_out_len);
	}
	if (_tmp_form_id != NULL && _len_form_id != 0) {
		_in_form_id = (char*)malloc(_len_form_id);
		if (_in_form_id == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy((void*)_in_form_id, _tmp_form_id, _len_form_id);
		_in_form_id[_len_form_id - 1] = '\0';
		if (_len_form_id != strlen(_in_form_id) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

	create_remove_overlay_msg(_in_output, _in_out_len, (const char*)_in_form_id);
err:
	if (_in_output) {
		memcpy(_tmp_output, _in_output, _len_output);
		free(_in_output);
	}
	if (_in_out_len) {
		memcpy(_tmp_out_len, _in_out_len, _len_out_len);
		free(_in_out_len);
	}
	if (_in_form_id) free((void*)_in_form_id);

	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv;} ecall_table[22];
} g_ecall_table = {
	22,
	{
		{(void*)(uintptr_t)sgx_enclave_init_ra, 0},
		{(void*)(uintptr_t)sgx_enclave_ra_close, 0},
		{(void*)(uintptr_t)sgx_verify_att_result_mac, 0},
		{(void*)(uintptr_t)sgx_put_secret_data, 0},
		{(void*)(uintptr_t)sgx_get_mac_key, 0},
		{(void*)(uintptr_t)sgx_run_js, 0},
		{(void*)(uintptr_t)sgx_add_form, 0},
		{(void*)(uintptr_t)sgx_add_input, 0},
		{(void*)(uintptr_t)sgx_onFocus, 0},
		{(void*)(uintptr_t)sgx_onBlur, 0},
		{(void*)(uintptr_t)sgx_form_len, 0},
		{(void*)(uintptr_t)sgx_submit_form, 0},
		{(void*)(uintptr_t)sgx_gcm_decrypt, 0},
		{(void*)(uintptr_t)sgx_get_keyboard_chars, 0},
		{(void*)(uintptr_t)sgx_test_decryption, 0},
		{(void*)(uintptr_t)sgx_get_http_response, 0},
		{(void*)(uintptr_t)sgx_print_debug, 0},
		{(void*)(uintptr_t)sgx_sgx_ra_get_ga, 0},
		{(void*)(uintptr_t)sgx_sgx_ra_proc_msg2_trusted, 0},
		{(void*)(uintptr_t)sgx_sgx_ra_get_msg3_trusted, 0},
		{(void*)(uintptr_t)sgx_create_add_overlay_msg, 0},
		{(void*)(uintptr_t)sgx_create_remove_overlay_msg, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[11][22];
} g_dyn_entry_table = {
	11,
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_print_string(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_print_string_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_string_t);
	void *__tmp = NULL;

	ocalloc_size += (str != NULL && sgx_is_within_enclave(str, _len_str)) ? _len_str : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_string_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_string_t));

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


	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL enc_make_http_request(sgx_status_t* retval, const char* method, const char* url, const char* headers, const char* request_data, uint8_t* p_mac, int* ret_code)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_method = method ? strlen(method) + 1 : 0;
	size_t _len_url = url ? strlen(url) + 1 : 0;
	size_t _len_headers = headers ? strlen(headers) + 1 : 0;
	size_t _len_request_data = request_data ? strlen(request_data) + 1 : 0;
	size_t _len_p_mac = 16 * sizeof(*p_mac);
	size_t _len_ret_code = sizeof(*ret_code);

	ms_enc_make_http_request_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_enc_make_http_request_t);
	void *__tmp = NULL;

	void *__tmp_p_mac = NULL;
	void *__tmp_ret_code = NULL;
	ocalloc_size += (method != NULL && sgx_is_within_enclave(method, _len_method)) ? _len_method : 0;
	ocalloc_size += (url != NULL && sgx_is_within_enclave(url, _len_url)) ? _len_url : 0;
	ocalloc_size += (headers != NULL && sgx_is_within_enclave(headers, _len_headers)) ? _len_headers : 0;
	ocalloc_size += (request_data != NULL && sgx_is_within_enclave(request_data, _len_request_data)) ? _len_request_data : 0;
	ocalloc_size += (p_mac != NULL && sgx_is_within_enclave(p_mac, _len_p_mac)) ? _len_p_mac : 0;
	ocalloc_size += (ret_code != NULL && sgx_is_within_enclave(ret_code, _len_ret_code)) ? _len_ret_code : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_enc_make_http_request_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_enc_make_http_request_t));

	if (method != NULL && sgx_is_within_enclave(method, _len_method)) {
		ms->ms_method = (char*)__tmp;
		memcpy(__tmp, method, _len_method);
		__tmp = (void *)((size_t)__tmp + _len_method);
	} else if (method == NULL) {
		ms->ms_method = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (url != NULL && sgx_is_within_enclave(url, _len_url)) {
		ms->ms_url = (char*)__tmp;
		memcpy(__tmp, url, _len_url);
		__tmp = (void *)((size_t)__tmp + _len_url);
	} else if (url == NULL) {
		ms->ms_url = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (headers != NULL && sgx_is_within_enclave(headers, _len_headers)) {
		ms->ms_headers = (char*)__tmp;
		memcpy(__tmp, headers, _len_headers);
		__tmp = (void *)((size_t)__tmp + _len_headers);
	} else if (headers == NULL) {
		ms->ms_headers = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (request_data != NULL && sgx_is_within_enclave(request_data, _len_request_data)) {
		ms->ms_request_data = (char*)__tmp;
		memcpy(__tmp, request_data, _len_request_data);
		__tmp = (void *)((size_t)__tmp + _len_request_data);
	} else if (request_data == NULL) {
		ms->ms_request_data = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (p_mac != NULL && sgx_is_within_enclave(p_mac, _len_p_mac)) {
		ms->ms_p_mac = (uint8_t*)__tmp;
		__tmp_p_mac = __tmp;
		memset(__tmp_p_mac, 0, _len_p_mac);
		__tmp = (void *)((size_t)__tmp + _len_p_mac);
	} else if (p_mac == NULL) {
		ms->ms_p_mac = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (ret_code != NULL && sgx_is_within_enclave(ret_code, _len_ret_code)) {
		ms->ms_ret_code = (int*)__tmp;
		__tmp_ret_code = __tmp;
		memset(__tmp_ret_code, 0, _len_ret_code);
		__tmp = (void *)((size_t)__tmp + _len_ret_code);
	} else if (ret_code == NULL) {
		ms->ms_ret_code = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(1, ms);

	if (retval) *retval = ms->ms_retval;
	if (p_mac) memcpy((void*)p_mac, __tmp_p_mac, _len_p_mac);
	if (ret_code) memcpy((void*)ret_code, __tmp_ret_code, _len_ret_code);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL create_session_ocall(sgx_status_t* retval, uint32_t* sid, uint8_t* dh_msg1, uint32_t dh_msg1_size, uint32_t timeout)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_sid = sizeof(*sid);
	size_t _len_dh_msg1 = dh_msg1_size;

	ms_create_session_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_create_session_ocall_t);
	void *__tmp = NULL;

	void *__tmp_sid = NULL;
	void *__tmp_dh_msg1 = NULL;
	ocalloc_size += (sid != NULL && sgx_is_within_enclave(sid, _len_sid)) ? _len_sid : 0;
	ocalloc_size += (dh_msg1 != NULL && sgx_is_within_enclave(dh_msg1, _len_dh_msg1)) ? _len_dh_msg1 : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_create_session_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_create_session_ocall_t));

	if (sid != NULL && sgx_is_within_enclave(sid, _len_sid)) {
		ms->ms_sid = (uint32_t*)__tmp;
		__tmp_sid = __tmp;
		memset(__tmp_sid, 0, _len_sid);
		__tmp = (void *)((size_t)__tmp + _len_sid);
	} else if (sid == NULL) {
		ms->ms_sid = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	if (dh_msg1 != NULL && sgx_is_within_enclave(dh_msg1, _len_dh_msg1)) {
		ms->ms_dh_msg1 = (uint8_t*)__tmp;
		__tmp_dh_msg1 = __tmp;
		memset(__tmp_dh_msg1, 0, _len_dh_msg1);
		__tmp = (void *)((size_t)__tmp + _len_dh_msg1);
	} else if (dh_msg1 == NULL) {
		ms->ms_dh_msg1 = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_dh_msg1_size = dh_msg1_size;
	ms->ms_timeout = timeout;
	status = sgx_ocall(2, ms);

	if (retval) *retval = ms->ms_retval;
	if (sid) memcpy((void*)sid, __tmp_sid, _len_sid);
	if (dh_msg1) memcpy((void*)dh_msg1, __tmp_dh_msg1, _len_dh_msg1);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL exchange_report_ocall(sgx_status_t* retval, uint32_t sid, uint8_t* dh_msg2, uint32_t dh_msg2_size, uint8_t* dh_msg3, uint32_t dh_msg3_size, uint32_t timeout)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_dh_msg2 = dh_msg2_size;
	size_t _len_dh_msg3 = dh_msg3_size;

	ms_exchange_report_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_exchange_report_ocall_t);
	void *__tmp = NULL;

	void *__tmp_dh_msg3 = NULL;
	ocalloc_size += (dh_msg2 != NULL && sgx_is_within_enclave(dh_msg2, _len_dh_msg2)) ? _len_dh_msg2 : 0;
	ocalloc_size += (dh_msg3 != NULL && sgx_is_within_enclave(dh_msg3, _len_dh_msg3)) ? _len_dh_msg3 : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_exchange_report_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_exchange_report_ocall_t));

	ms->ms_sid = sid;
	if (dh_msg2 != NULL && sgx_is_within_enclave(dh_msg2, _len_dh_msg2)) {
		ms->ms_dh_msg2 = (uint8_t*)__tmp;
		memcpy(__tmp, dh_msg2, _len_dh_msg2);
		__tmp = (void *)((size_t)__tmp + _len_dh_msg2);
	} else if (dh_msg2 == NULL) {
		ms->ms_dh_msg2 = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_dh_msg2_size = dh_msg2_size;
	if (dh_msg3 != NULL && sgx_is_within_enclave(dh_msg3, _len_dh_msg3)) {
		ms->ms_dh_msg3 = (uint8_t*)__tmp;
		__tmp_dh_msg3 = __tmp;
		memset(__tmp_dh_msg3, 0, _len_dh_msg3);
		__tmp = (void *)((size_t)__tmp + _len_dh_msg3);
	} else if (dh_msg3 == NULL) {
		ms->ms_dh_msg3 = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_dh_msg3_size = dh_msg3_size;
	ms->ms_timeout = timeout;
	status = sgx_ocall(3, ms);

	if (retval) *retval = ms->ms_retval;
	if (dh_msg3) memcpy((void*)dh_msg3, __tmp_dh_msg3, _len_dh_msg3);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL close_session_ocall(sgx_status_t* retval, uint32_t sid, uint32_t timeout)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_close_session_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_close_session_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_close_session_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_close_session_ocall_t));

	ms->ms_sid = sid;
	ms->ms_timeout = timeout;
	status = sgx_ocall(4, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL invoke_service_ocall(sgx_status_t* retval, uint8_t* pse_message_req, uint32_t pse_message_req_size, uint8_t* pse_message_resp, uint32_t pse_message_resp_size, uint32_t timeout)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_pse_message_req = pse_message_req_size;
	size_t _len_pse_message_resp = pse_message_resp_size;

	ms_invoke_service_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_invoke_service_ocall_t);
	void *__tmp = NULL;

	void *__tmp_pse_message_resp = NULL;
	ocalloc_size += (pse_message_req != NULL && sgx_is_within_enclave(pse_message_req, _len_pse_message_req)) ? _len_pse_message_req : 0;
	ocalloc_size += (pse_message_resp != NULL && sgx_is_within_enclave(pse_message_resp, _len_pse_message_resp)) ? _len_pse_message_resp : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_invoke_service_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_invoke_service_ocall_t));

	if (pse_message_req != NULL && sgx_is_within_enclave(pse_message_req, _len_pse_message_req)) {
		ms->ms_pse_message_req = (uint8_t*)__tmp;
		memcpy(__tmp, pse_message_req, _len_pse_message_req);
		__tmp = (void *)((size_t)__tmp + _len_pse_message_req);
	} else if (pse_message_req == NULL) {
		ms->ms_pse_message_req = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_pse_message_req_size = pse_message_req_size;
	if (pse_message_resp != NULL && sgx_is_within_enclave(pse_message_resp, _len_pse_message_resp)) {
		ms->ms_pse_message_resp = (uint8_t*)__tmp;
		__tmp_pse_message_resp = __tmp;
		memset(__tmp_pse_message_resp, 0, _len_pse_message_resp);
		__tmp = (void *)((size_t)__tmp + _len_pse_message_resp);
	} else if (pse_message_resp == NULL) {
		ms->ms_pse_message_resp = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_pse_message_resp_size = pse_message_resp_size;
	ms->ms_timeout = timeout;
	status = sgx_ocall(5, ms);

	if (retval) *retval = ms->ms_retval;
	if (pse_message_resp) memcpy((void*)pse_message_resp, __tmp_pse_message_resp, _len_pse_message_resp);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(*cpuinfo);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	void *__tmp_cpuinfo = NULL;
	ocalloc_size += (cpuinfo != NULL && sgx_is_within_enclave(cpuinfo, _len_cpuinfo)) ? _len_cpuinfo : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));

	if (cpuinfo != NULL && sgx_is_within_enclave(cpuinfo, _len_cpuinfo)) {
		ms->ms_cpuinfo = (int*)__tmp;
		__tmp_cpuinfo = __tmp;
		memset(__tmp_cpuinfo, 0, _len_cpuinfo);
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
	} else if (cpuinfo == NULL) {
		ms->ms_cpuinfo = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_leaf = leaf;
	ms->ms_subleaf = subleaf;
	status = sgx_ocall(6, ms);

	if (cpuinfo) memcpy((void*)cpuinfo, __tmp_cpuinfo, _len_cpuinfo);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));

	ms->ms_self = SGX_CAST(void*, self);
	status = sgx_ocall(7, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));

	ms->ms_waiter = SGX_CAST(void*, waiter);
	status = sgx_ocall(8, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));

	ms->ms_waiter = SGX_CAST(void*, waiter);
	ms->ms_self = SGX_CAST(void*, self);
	status = sgx_ocall(9, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(*waiters);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;

	ocalloc_size += (waiters != NULL && sgx_is_within_enclave(waiters, _len_waiters)) ? _len_waiters : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));

	if (waiters != NULL && sgx_is_within_enclave(waiters, _len_waiters)) {
		ms->ms_waiters = (void**)__tmp;
		memcpy(__tmp, waiters, _len_waiters);
		__tmp = (void *)((size_t)__tmp + _len_waiters);
	} else if (waiters == NULL) {
		ms->ms_waiters = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_total = total;
	status = sgx_ocall(10, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

