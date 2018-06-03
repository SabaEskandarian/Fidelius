#include "isv_enclave_u.h"
#include <errno.h>

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

static sgx_status_t SGX_CDECL isv_enclave_ocall_print_string(void* pms)
{
	ms_ocall_print_string_t* ms = SGX_CAST(ms_ocall_print_string_t*, pms);
	ocall_print_string((const char*)ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_create_session_ocall(void* pms)
{
	ms_create_session_ocall_t* ms = SGX_CAST(ms_create_session_ocall_t*, pms);
	ms->ms_retval = create_session_ocall(ms->ms_sid, ms->ms_dh_msg1, ms->ms_dh_msg1_size, ms->ms_timeout);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_exchange_report_ocall(void* pms)
{
	ms_exchange_report_ocall_t* ms = SGX_CAST(ms_exchange_report_ocall_t*, pms);
	ms->ms_retval = exchange_report_ocall(ms->ms_sid, ms->ms_dh_msg2, ms->ms_dh_msg2_size, ms->ms_dh_msg3, ms->ms_dh_msg3_size, ms->ms_timeout);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_close_session_ocall(void* pms)
{
	ms_close_session_ocall_t* ms = SGX_CAST(ms_close_session_ocall_t*, pms);
	ms->ms_retval = close_session_ocall(ms->ms_sid, ms->ms_timeout);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_invoke_service_ocall(void* pms)
{
	ms_invoke_service_ocall_t* ms = SGX_CAST(ms_invoke_service_ocall_t*, pms);
	ms->ms_retval = invoke_service_ocall(ms->ms_pse_message_req, ms->ms_pse_message_req_size, ms->ms_pse_message_resp, ms->ms_pse_message_resp_size, ms->ms_timeout);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall((const void*)ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall((const void*)ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall((const void*)ms->ms_waiter, (const void*)ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL isv_enclave_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall((const void**)ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[10];
} ocall_table_isv_enclave = {
	10,
	{
		(void*)isv_enclave_ocall_print_string,
		(void*)isv_enclave_create_session_ocall,
		(void*)isv_enclave_exchange_report_ocall,
		(void*)isv_enclave_close_session_ocall,
		(void*)isv_enclave_invoke_service_ocall,
		(void*)isv_enclave_sgx_oc_cpuidex,
		(void*)isv_enclave_sgx_thread_wait_untrusted_event_ocall,
		(void*)isv_enclave_sgx_thread_set_untrusted_event_ocall,
		(void*)isv_enclave_sgx_thread_setwait_untrusted_events_ocall,
		(void*)isv_enclave_sgx_thread_set_multiple_untrusted_events_ocall,
	}
};
sgx_status_t enclave_init_ra(sgx_enclave_id_t eid, sgx_status_t* retval, int b_pse, sgx_ra_context_t* p_context)
{
	sgx_status_t status;
	ms_enclave_init_ra_t ms;
	ms.ms_b_pse = b_pse;
	ms.ms_p_context = p_context;
	status = sgx_ecall(eid, 0, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t enclave_ra_close(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context)
{
	sgx_status_t status;
	ms_enclave_ra_close_t ms;
	ms.ms_context = context;
	status = sgx_ecall(eid, 1, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t verify_att_result_mac(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, uint8_t* message, size_t message_size, uint8_t* mac, size_t mac_size)
{
	sgx_status_t status;
	ms_verify_att_result_mac_t ms;
	ms.ms_context = context;
	ms.ms_message = message;
	ms.ms_message_size = message_size;
	ms.ms_mac = mac;
	ms.ms_mac_size = mac_size;
	status = sgx_ecall(eid, 2, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t put_secret_data(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, uint8_t* p_secret, uint32_t secret_size, uint8_t* gcm_mac)
{
	sgx_status_t status;
	ms_put_secret_data_t ms;
	ms.ms_context = context;
	ms.ms_p_secret = p_secret;
	ms.ms_secret_size = secret_size;
	ms.ms_gcm_mac = gcm_mac;
	status = sgx_ecall(eid, 3, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t get_mac_key(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* p_mac, uint32_t mac_size, uint8_t* gcm_mac)
{
	sgx_status_t status;
	ms_get_mac_key_t ms;
	ms.ms_p_mac = p_mac;
	ms.ms_mac_size = mac_size;
	ms.ms_gcm_mac = gcm_mac;
	status = sgx_ecall(eid, 4, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t run_js(sgx_enclave_id_t eid, sgx_status_t* retval, char* code, size_t len)
{
	sgx_status_t status;
	ms_run_js_t ms;
	ms.ms_code = code;
	ms.ms_len = len;
	status = sgx_ecall(eid, 5, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t add_form(sgx_enclave_id_t eid, sgx_status_t* retval, const char* name, size_t len, const char* origin, size_t origin_len, uint16_t x, uint16_t y)
{
	sgx_status_t status;
	ms_add_form_t ms;
	ms.ms_name = (char*)name;
	ms.ms_len = len;
	ms.ms_origin = (char*)origin;
	ms.ms_origin_len = origin_len;
	ms.ms_x = x;
	ms.ms_y = y;
	status = sgx_ecall(eid, 6, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t add_input(sgx_enclave_id_t eid, sgx_status_t* retval, const char* name, size_t len1, const char* input_i, size_t len2, const uint8_t* sig_form, size_t sig_form_size, int validate, uint16_t x, uint16_t y, uint16_t height, uint16_t width)
{
	sgx_status_t status;
	ms_add_input_t ms;
	ms.ms_name = (char*)name;
	ms.ms_len1 = len1;
	ms.ms_input_i = (char*)input_i;
	ms.ms_len2 = len2;
	ms.ms_sig_form = (uint8_t*)sig_form;
	ms.ms_sig_form_size = sig_form_size;
	ms.ms_validate = validate;
	ms.ms_x = x;
	ms.ms_y = y;
	ms.ms_height = height;
	ms.ms_width = width;
	status = sgx_ecall(eid, 7, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t onFocus(sgx_enclave_id_t eid, sgx_status_t* retval, const char* formName, const char* inputName, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	sgx_status_t status;
	ms_onFocus_t ms;
	ms.ms_formName = (char*)formName;
	ms.ms_formName_len = formName ? strlen(formName) + 1 : 0;
	ms.ms_inputName = (char*)inputName;
	ms.ms_inputName_len = inputName ? strlen(inputName) + 1 : 0;
	ms.ms_x = x;
	ms.ms_y = y;
	ms.ms_width = width;
	ms.ms_height = height;
	status = sgx_ecall(eid, 8, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t onBlur(sgx_enclave_id_t eid, sgx_status_t* retval)
{
	sgx_status_t status;
	ms_onBlur_t ms;
	status = sgx_ecall(eid, 9, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t form_len(sgx_enclave_id_t eid, uint32_t* retval, const char* formName)
{
	sgx_status_t status;
	ms_form_len_t ms;
	ms.ms_formName = (char*)formName;
	ms.ms_formName_len = formName ? strlen(formName) + 1 : 0;
	status = sgx_ecall(eid, 10, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t submit_form(sgx_enclave_id_t eid, sgx_status_t* retval, const char* formName, uint8_t* dest, uint32_t encr_size, uint8_t* gcm_mac)
{
	sgx_status_t status;
	ms_submit_form_t ms;
	ms.ms_formName = (char*)formName;
	ms.ms_formName_len = formName ? strlen(formName) + 1 : 0;
	ms.ms_dest = dest;
	ms.ms_encr_size = encr_size;
	ms.ms_gcm_mac = gcm_mac;
	status = sgx_ecall(eid, 11, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t gcm_decrypt(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* p_src, uint32_t src_len, uint8_t* p_dst, uint8_t* p_iv, sgx_aes_gcm_128bit_tag_t* p_in_mac)
{
	sgx_status_t status;
	ms_gcm_decrypt_t ms;
	ms.ms_p_src = p_src;
	ms.ms_src_len = src_len;
	ms.ms_p_dst = p_dst;
	ms.ms_p_iv = p_iv;
	ms.ms_p_in_mac = p_in_mac;
	status = sgx_ecall(eid, 12, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t get_keyboard_chars(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* p_src)
{
	sgx_status_t status;
	ms_get_keyboard_chars_t ms;
	ms.ms_p_src = p_src;
	status = sgx_ecall(eid, 13, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t sgx_ra_get_ga(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, sgx_ec256_public_t* g_a)
{
	sgx_status_t status;
	ms_sgx_ra_get_ga_t ms;
	ms.ms_context = context;
	ms.ms_g_a = g_a;
	status = sgx_ecall(eid, 14, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t sgx_ra_proc_msg2_trusted(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, const sgx_ra_msg2_t* p_msg2, const sgx_target_info_t* p_qe_target, sgx_report_t* p_report, sgx_quote_nonce_t* p_nonce)
{
	sgx_status_t status;
	ms_sgx_ra_proc_msg2_trusted_t ms;
	ms.ms_context = context;
	ms.ms_p_msg2 = (sgx_ra_msg2_t*)p_msg2;
	ms.ms_p_qe_target = (sgx_target_info_t*)p_qe_target;
	ms.ms_p_report = p_report;
	ms.ms_p_nonce = p_nonce;
	status = sgx_ecall(eid, 15, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t sgx_ra_get_msg3_trusted(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, uint32_t quote_size, sgx_report_t* qe_report, sgx_ra_msg3_t* p_msg3, uint32_t msg3_size)
{
	sgx_status_t status;
	ms_sgx_ra_get_msg3_trusted_t ms;
	ms.ms_context = context;
	ms.ms_quote_size = quote_size;
	ms.ms_qe_report = qe_report;
	ms.ms_p_msg3 = p_msg3;
	ms.ms_msg3_size = msg3_size;
	status = sgx_ecall(eid, 16, &ocall_table_isv_enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t create_add_overlay_msg(sgx_enclave_id_t eid, uint8_t* output, uint32_t* out_len, const char* form_id)
{
	sgx_status_t status;
	ms_create_add_overlay_msg_t ms;
	ms.ms_output = output;
	ms.ms_out_len = out_len;
	ms.ms_form_id = (char*)form_id;
	ms.ms_form_id_len = form_id ? strlen(form_id) + 1 : 0;
	status = sgx_ecall(eid, 17, &ocall_table_isv_enclave, &ms);
	return status;
}

sgx_status_t create_remove_overlay_msg(sgx_enclave_id_t eid, uint8_t* output, uint32_t* out_len, const char* form_id)
{
	sgx_status_t status;
	ms_create_remove_overlay_msg_t ms;
	ms.ms_output = output;
	ms.ms_out_len = out_len;
	ms.ms_form_id = (char*)form_id;
	ms.ms_form_id_len = form_id ? strlen(form_id) + 1 : 0;
	status = sgx_ecall(eid, 18, &ocall_table_isv_enclave, &ms);
	return status;
}

