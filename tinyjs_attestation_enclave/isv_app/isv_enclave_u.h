#ifndef ISV_ENCLAVE_U_H__
#define ISV_ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_satus_t etc. */

#include "sgx_key_exchange.h"
#include "sgx_trts.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_string, (const char* str));
sgx_status_t SGX_UBRIDGE(SGX_NOCONVENTION, create_session_ocall, (uint32_t* sid, uint8_t* dh_msg1, uint32_t dh_msg1_size, uint32_t timeout));
sgx_status_t SGX_UBRIDGE(SGX_NOCONVENTION, exchange_report_ocall, (uint32_t sid, uint8_t* dh_msg2, uint32_t dh_msg2_size, uint8_t* dh_msg3, uint32_t dh_msg3_size, uint32_t timeout));
sgx_status_t SGX_UBRIDGE(SGX_NOCONVENTION, close_session_ocall, (uint32_t sid, uint32_t timeout));
sgx_status_t SGX_UBRIDGE(SGX_NOCONVENTION, invoke_service_ocall, (uint8_t* pse_message_req, uint32_t pse_message_req_size, uint8_t* pse_message_resp, uint32_t pse_message_resp_size, uint32_t timeout));
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));

sgx_status_t enclave_init_ra(sgx_enclave_id_t eid, sgx_status_t* retval, int b_pse, sgx_ra_context_t* p_context);
sgx_status_t enclave_ra_close(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context);
sgx_status_t verify_att_result_mac(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, uint8_t* message, size_t message_size, uint8_t* mac, size_t mac_size);
sgx_status_t put_secret_data(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, uint8_t* p_secret, uint32_t secret_size, uint8_t* gcm_mac);
sgx_status_t get_mac_key(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* p_mac, uint32_t mac_size, uint8_t* gcm_mac);
sgx_status_t run_js(sgx_enclave_id_t eid, sgx_status_t* retval, char* code, size_t len);
sgx_status_t add_form(sgx_enclave_id_t eid, sgx_status_t* retval, const char* name, size_t len, const char* origin, size_t origin_len, uint16_t x, uint16_t y);
sgx_status_t add_input(sgx_enclave_id_t eid, sgx_status_t* retval, const char* name, size_t len1, const char* input_i, size_t len2, const uint8_t* sig_form, size_t sig_form_size, int validate, uint16_t x, uint16_t y, uint16_t height, uint16_t width);
sgx_status_t onFocus(sgx_enclave_id_t eid, sgx_status_t* retval, const char* formName, const char* inputName, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
sgx_status_t onBlur(sgx_enclave_id_t eid, sgx_status_t* retval);
sgx_status_t form_len(sgx_enclave_id_t eid, uint32_t* retval, const char* formName);
sgx_status_t submit_form(sgx_enclave_id_t eid, sgx_status_t* retval, const char* formName, uint8_t* dest, uint32_t encr_size, uint8_t* gcm_mac);
sgx_status_t gcm_decrypt(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* p_src, uint32_t src_len, uint8_t* p_dst, uint8_t* p_iv, sgx_aes_gcm_128bit_tag_t* p_in_mac);
sgx_status_t get_keyboard_chars(sgx_enclave_id_t eid, sgx_status_t* retval, uint8_t* p_src);
sgx_status_t sgx_ra_get_ga(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, sgx_ec256_public_t* g_a);
sgx_status_t sgx_ra_proc_msg2_trusted(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, const sgx_ra_msg2_t* p_msg2, const sgx_target_info_t* p_qe_target, sgx_report_t* p_report, sgx_quote_nonce_t* p_nonce);
sgx_status_t sgx_ra_get_msg3_trusted(sgx_enclave_id_t eid, sgx_status_t* retval, sgx_ra_context_t context, uint32_t quote_size, sgx_report_t* qe_report, sgx_ra_msg3_t* p_msg3, uint32_t msg3_size);
sgx_status_t create_add_overlay_msg(sgx_enclave_id_t eid, uint8_t* output, uint32_t* out_len, const char* form_id);
sgx_status_t create_remove_overlay_msg(sgx_enclave_id_t eid, uint8_t* output, uint32_t* out_len, const char* form_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
