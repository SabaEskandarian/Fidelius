/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TOsub, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include <assert.h>
#include "isv_enclave.h"
#include "isv_enclave_t.h"
#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"
#include "string.h"
#include "TinyJS.h"
#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"

#include <map>
#include <stdlib.h>

// This is the public EC key of the SP. The corresponding private EC key is
// used by the SP to sign data used in the remote attestation SIGMA protocol
// to sign channel binding data in MSG2. A successful verification of the
// signature confirms the identity of the SP to the ISV app in remote
// attestation secure channel binding. The public EC key should be hardcoded in
// the enclave or delivered in a trustworthy manner. The use of a spoofed public
// EC key in the remote attestation with secure channel binding session may lead
// to a security compromise. Every different SP the enlcave communicates to
// must have a unique SP public key. Delivery of the SP public key is
// determined by the ISV. The TKE SIGMA protocl expects an Elliptical Curve key
// based on NIST P-256
static const sgx_ec256_public_t g_sp_pub_key = {
    {
        0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
    },
    {
        0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
    }

};

// Used to store the secret passed by the SP in the sample code. The
// size is forced to be 8 bytes. Expected value is
// 0x01,0x02,0x03,0x04,0x0x5,0x0x6,0x0x7
uint8_t g_secret[8] = {0};

uint8_t mac_secret[8] = {0};


#ifdef SUPPLIED_KEY_DERIVATION

#pragma message ("Supplied key derivation function is used.")

typedef struct _hash_buffer_t
{
    uint8_t counter[4];
    sgx_ec256_dh_shared_t shared_secret;
    uint8_t algorithm_id[4];
} hash_buffer_t;

const char ID_U[] = "SGXRAENCLAVE";
const char ID_V[] = "SGXRASERVER";


// Derive two keys from shared key and key id.
bool derive_key(
    const sgx_ec256_dh_shared_t *p_shared_key,
    uint8_t key_id,
    sgx_ec_key_128bit_t *first_derived_key,
    sgx_ec_key_128bit_t *second_derived_key)
{
    sgx_status_t sgx_ret = SGX_SUCCESS;
    hash_buffer_t hash_buffer;
    sgx_sha_state_handle_t sha_context;
    sgx_sha256_hash_t key_material;

    memset(&hash_buffer, 0, sizeof(hash_buffer_t));
    /* counter in big endian  */
    hash_buffer.counter[3] = key_id;

    /*convert from little endian to big endian */
    for (size_t i = 0; i < sizeof(sgx_ec256_dh_shared_t); i++)
    {
        hash_buffer.shared_secret.s[i] = p_shared_key->s[sizeof(p_shared_key->s)-1 - i];
    }

    sgx_ret = sgx_sha256_init(&sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t*)&hash_buffer, sizeof(hash_buffer_t), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t*)&ID_U, sizeof(ID_U), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t*)&ID_V, sizeof(ID_V), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_get_hash(sha_context, &key_material);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_close(sha_context);

    assert(sizeof(sgx_ec_key_128bit_t)* 2 == sizeof(sgx_sha256_hash_t));
    memcpy(first_derived_key, &key_material, sizeof(sgx_ec_key_128bit_t));
    memcpy(second_derived_key, (uint8_t*)&key_material + sizeof(sgx_ec_key_128bit_t), sizeof(sgx_ec_key_128bit_t));

    // memset here can be optimized away by compiler, so please use memset_s on
    // windows for production code and similar functions on other OSes.
    memset(&key_material, 0, sizeof(sgx_sha256_hash_t));

    return true;
}

//isv defined key derivation function id
#define ISV_KDF_ID 2

typedef enum _derive_key_type_t
{
    DERIVE_KEY_SMK_SK = 0,
    DERIVE_KEY_MK_VK,
} derive_key_type_t;

sgx_status_t key_derivation(const sgx_ec256_dh_shared_t* shared_key,
    uint16_t kdf_id,
    sgx_ec_key_128bit_t* smk_key,
    sgx_ec_key_128bit_t* sk_key,
    sgx_ec_key_128bit_t* mk_key,
    sgx_ec_key_128bit_t* vk_key)
{
    bool derive_ret = false;

    if (NULL == shared_key)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    if (ISV_KDF_ID != kdf_id)
    {
        //fprintf(stderr, "\nError, key derivation id mismatch in [%s].", __FUNCTION__);
        return SGX_ERROR_KDF_MISMATCH;
    }

    derive_ret = derive_key(shared_key, DERIVE_KEY_SMK_SK,
        smk_key, sk_key);
    if (derive_ret != true)
    {
        //fprintf(stderr, "\nError, derive key fail in [%s].", __FUNCTION__);
        return SGX_ERROR_UNEXPECTED;
    }

    derive_ret = derive_key(shared_key, DERIVE_KEY_MK_VK,
        mk_key, vk_key);
    if (derive_ret != true)
    {
        //fprintf(stderr, "\nError, derive key fail in [%s].", __FUNCTION__);
        return SGX_ERROR_UNEXPECTED;
    }
    return SGX_SUCCESS;
}
#else
#pragma message ("Default key derivation function is used.")
#endif

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf_enc(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

std::string intToString(int i){
    char buf[20];
    snprintf(buf, 20, "%d", i);
    std::string ret = buf;
    return ret;
}

// This ecall is a wrapper of sgx_ra_init to create the trusted
// KE exchange key context needed for the remote attestation
// SIGMA API's. Input pointers aren't checked since the trusted stubs
// copy them into EPC memory.
//
// @param b_pse Indicates whether the ISV app is using the
//              platform services.
// @param p_context Pointer to the location where the returned
//                  key context is to be copied.
//
// @return Any error return from the create PSE session if b_pse
//         is true.
// @return Any error returned from the trusted key exchange API
//         for creating a key context.

sgx_status_t enclave_init_ra(
    int b_pse,
    sgx_ra_context_t *p_context)
{
    // isv enclave call to trusted key exchange library.
    sgx_status_t ret;
    if(b_pse)
    {
        int busy_retry_times = 2;
        do{
            ret = sgx_create_pse_session();
        }while (ret == SGX_ERROR_BUSY && busy_retry_times--);
        if (ret != SGX_SUCCESS)
            return ret;
    }
#ifdef SUPPLIED_KEY_DERIVATION
    ret = sgx_ra_init_ex(&g_sp_pub_key, b_pse, key_derivation, p_context);
#else
    ret = sgx_ra_init(&g_sp_pub_key, b_pse, p_context);
#endif
    if(b_pse)
    {
        sgx_close_pse_session();
        return ret;
    }
    return ret;
}


// Closes the tKE key context used during the SIGMA key
// exchange.
//
// @param context The trusted KE library key context.
//
// @return Return value from the key context close API

sgx_status_t SGXAPI enclave_ra_close(
    sgx_ra_context_t context)
{
    sgx_status_t ret;
    ret = sgx_ra_close(context);
    return ret;
}


// Verify the mac sent in att_result_msg from the SP using the
// MK key. Input pointers aren't checked since the trusted stubs
// copy them into EPC memory.
//
//
// @param context The trusted KE library key context.
// @param p_message Pointer to the message used to produce MAC
// @param message_size Size in bytes of the message.
// @param p_mac Pointer to the MAC to compare to.
// @param mac_size Size in bytes of the MAC
//
// @return SGX_ERROR_INVALID_PARAMETER - MAC size is incorrect.
// @return Any error produced by tKE  API to get SK key.
// @return Any error produced by the AESCMAC function.
// @return SGX_ERROR_MAC_MISMATCH - MAC compare fails.

sgx_status_t verify_att_result_mac(sgx_ra_context_t context,
                                   uint8_t* p_message,
                                   size_t message_size,
                                   uint8_t* p_mac,
                                   size_t mac_size)
{
    sgx_status_t ret;
    sgx_ec_key_128bit_t mk_key;

    if(mac_size != sizeof(sgx_mac_t))
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }
    if(message_size > UINT32_MAX)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }

    do {
        uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};

        ret = sgx_ra_get_keys(context, SGX_RA_KEY_MK, &mk_key);
        if(SGX_SUCCESS != ret)
        {
            break;
        }
        ret = sgx_rijndael128_cmac_msg(&mk_key,
                                       p_message,
                                       (uint32_t)message_size,
                                       &mac);
        if(SGX_SUCCESS != ret)
        {
            break;
        }
        if(0 == consttime_memequal(p_mac, mac, sizeof(mac)))
        {
            ret = SGX_ERROR_MAC_MISMATCH;
            break;
        }

    }
    while(0);

    return ret;
}


// Generate a secret information for the SP encrypted with SK.
// Input pointers aren't checked since the trusted stubs copy
// them into EPC memory.
//
// @param context The trusted KE library key context.
// @param p_secret Message containing the secret.
// @param secret_size Size in bytes of the secret message.
// @param p_gcm_mac The pointer the the AESGCM MAC for the
//                 message.
//
// @return SGX_ERROR_INVALID_PARAMETER - secret size if
//         incorrect.
// @return Any error produced by tKE  API to get SK key.
// @return Any error produced by the AESGCM function.
// @return SGX_ERROR_UNEXPECTED - the secret doesn't match the
//         expected value.

sgx_status_t put_secret_data(
    sgx_ra_context_t context,
    uint8_t *p_secret,
    uint32_t secret_size,
    uint8_t *p_gcm_mac)
{
    sgx_status_t ret = SGX_SUCCESS;
    sgx_ec_key_128bit_t sk_key;

    do {
        if(secret_size != 8)
        {
            ret = SGX_ERROR_INVALID_PARAMETER;
            break;
        }

        ret = sgx_ra_get_keys(context, SGX_RA_KEY_SK, &sk_key);
        if(SGX_SUCCESS != ret)
        {
            break;
        }

        uint8_t aes_gcm_iv[12] = {0};
        ret = sgx_rijndael128GCM_decrypt(&sk_key,
                                         p_secret,
                                         secret_size,
                                         &g_secret[0],
                                         &aes_gcm_iv[0],
                                         12,
                                         NULL,
                                         0,
                                         (const sgx_aes_gcm_128bit_tag_t *)
                                            (p_gcm_mac));

        uint32_t i;
        bool secret_match = true;
        for(i=0;i<secret_size;i++)
        {
            if(g_secret[i] != i)
            {
                secret_match = false;
            }
        }

        if(!secret_match)
        {
            ret = SGX_ERROR_UNEXPECTED;
        }
        // Once the server has the shared secret, it should be sealed to
        // persistent storage for future use. This will prevents having to
        // perform remote attestation until the secret goes stale. Once the
        // enclave is created again, the secret can be unsealed.
    } while(0);
    return ret;
}

/////////////our stuff//////////////////////

static const form nullForm = {};
static const input nullInput = {};

std::map<std::string, form> forms;
form curForm = nullForm;
input curInput = nullInput;
std::string origin = "";

void printForm(form f){
    printf_enc("# elements in form: %d\n", f.inputs.size());
    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
    it != f.inputs.end(); ++it)
    {
        printf_enc("input name: %s\n", it->first);
    }
}

std::string parse_form(form f, bool include_vals) {
    std::string start = "{";
    std::string end = "}";

    std::string parsed = "" + start;

    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
    it != f.inputs.end(); ++it)
    {
        std::string key = it->first;
        std::string value = "";
        if(include_vals) {
            value = it->second.value;
        } 
        std::string pair = "\"" + key + "\":\"" + value + "\",";
        parsed = parsed + pair;
    }

    parsed = parsed + end;
    // printf_enc("prased form: %s\n", parsed);
    // printf_enc("len: %d\n", parsed.length());
    return parsed; 
}

std::string parse_form_secure(form f, uint8_t* p_gcm_mac) {
    std::string start = "{";
    std::string end = "}";

    std::string parsed = "" + start;

    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
    it != f.inputs.end(); ++it)
    {
        std::string key = it->first;
        std::string value = it->second.value;

        uint32_t len_val = (uint32_t) value.length();
        uint8_t encr_val[len_val] = {0};
        uint8_t aes_gcm_iv[12] = {0};

        sgx_status_t ret = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_tag_t *) (&g_secret),
                (const uint8_t*) &value[0],
                len_val,  
                &encr_val[0],
                &aes_gcm_iv[0],
                12,
                NULL,
                0,
                (sgx_aes_gcm_128bit_tag_t *) (p_gcm_mac));
        if(ret != SGX_SUCCESS){
            return "-1";
        }

        value = std::string((char *) &encr_val[0]);

        std::string pair = "\"" + key + "\":\"" + value + "\",";
        parsed = parsed + pair;
    }

    parsed = parsed + end;
    return parsed; 
}

//copys a string into the enclave
std::string copyString(char* s, size_t len) {
    char es_temp[len];
    memcpy(es_temp, s, len);
    std::string es = std::string(es_temp);
    return es;
}

sgx_status_t get_mac_key(uint8_t *p_mac, uint32_t mac_size,
        uint8_t *p_gcm_mac) {
    sgx_status_t ret = SGX_SUCCESS;

    if(mac_size != SGX_CMAC_MAC_SIZE)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }


    uint8_t aes_gcm_iv[12] = {0};
    ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_tag_t *) (&g_secret),
                                     p_mac,
                                     mac_size,
                                     &mac_secret[0],
                                     &aes_gcm_iv[0],
                                     12,
                                     NULL,
                                     0,
                                     (const sgx_aes_gcm_128bit_tag_t *)
                                        (p_gcm_mac));

    return ret;
}


sgx_status_t validate(uint8_t *p_message, uint32_t message_size,
                      sgx_ec256_signature_t* p_signature) {
    
    sgx_ecc_state_handle_t ecc_handle;
    uint8_t result;

    sgx_status_t ret = sgx_ecdsa_verify(
            p_message,
            message_size,
            &g_sp_pub_key,
            p_signature,
            &result,
            ecc_handle
        );
    if(ret != SGX_SUCCESS) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    if((sgx_status_t) result != SGX_EC_VALID) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    return ret;
}

//adds a new form to the internal list of forms
sgx_status_t add_form(char* name, size_t len, 
                        char* this_origin, size_t origin_len, uint16_t x, uint16_t y) {

   std::string oName = copyString(this_origin, origin_len);
    if (origin != "" && origin != oName) {
        return SGX_ERROR_INVALID_PARAMETER;
    } else {
        origin = oName;
    }
    std::string eName = copyString(name, len);
    if(forms.count(eName) > 0) {
      return SGX_ERROR_INVALID_PARAMETER; //error if form already exists
    } else {
        form new_form;
        new_form.x = x;
        new_form.y = y;
        forms.insert(std::pair<std::string, form>(eName, new_form));
        return SGX_SUCCESS;
    }
}

//adds a new input field to a form
sgx_status_t add_input(char * name, size_t len1, char* input_i, size_t len2,
                    uint8_t *p_sig_form, size_t sig_form_size, int val) {
    
    std::string eName = copyString(name, len1);
    std::string eInput = copyString(input_i, len2);
    if(forms.count(eName) == 0) {
      return SGX_ERROR_INVALID_PARAMETER; //error if form does not exist
    } else {  
        std::map<std::string, form>::iterator it;
        it = forms.find((std::string) eName);
        form f = it->second;
        if(f.validated) {
            return SGX_ERROR_INVALID_PARAMETER;
        }


        std::map<std::string, input> inputs = f.inputs;
        if(inputs.count(eInput) > 0) {
            return SGX_ERROR_INVALID_PARAMETER; //error if input already exists
        } else {
            input new_input;
            inputs.insert(std::pair<std::string, input>(eInput, new_input));
            f.inputs = inputs;
            
            // all inputs added, then only parse form and validate form
            if(val == 1) {
                std::string form = parse_form(f, false);
                size_t len_form = form.length();
                if(SGX_SUCCESS == validate((uint8_t*) form.c_str(), (uint32_t) len_form, (sgx_ec256_signature_t*) p_sig_form)) {        
                    f.validated = true;
                }
                else {
                    // delete form, return failure
                    forms.erase((std::string) eName);
                    return SGX_ERROR_MAC_MISMATCH;
                }
            }             
            // what's the point of this line again Sawyer?
            it->second = f;
            return SGX_SUCCESS;
        }
    }
}

//sets flag indicating which form/input field should stand by to recive user input
sgx_status_t onFocus(const char* formName, const char* inputName, 
                    uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  std::map<std::string, form>::iterator it;
  it = forms.find((std::string) formName);
  if(it == forms.end()) {
    return SGX_ERROR_INVALID_PARAMETER;
  }
  form f = it->second;
  std::map<std::string, input>::iterator it2;
  it2 = f.inputs.find((std::string) inputName);
  if(it2 == f.inputs.end()) {
    return SGX_ERROR_INVALID_PARAMETER;
  }

  curForm = f;
  curInput = it2->second;
  curInput.x = x;
  curInput.y = y;
  return SGX_SUCCESS;
}

//sets flag indicating no form/input field is ready to accept user input
sgx_status_t onBlur() {
  curForm = nullForm;
  curInput = nullInput;
  return SGX_SUCCESS;
}

uint32_t form_len(const char* formName) {
  std::map<std::string, form>::iterator it;
  it = forms.find((std::string) formName);
  if(it == forms.end()) {
    return SGX_ERROR_INVALID_PARAMETER;
  }
  form f = it->second;
  uint32_t len = (uint32_t) parse_form(f, true).length();
  return len;
}

//see put_secret_data for explanation of types
sgx_status_t submit_form(const char* formName, 
    uint8_t* dest, uint32_t encr_size, uint8_t* p_gcm_mac) {
  std::map<std::string, form>::iterator it;
  it = forms.find((std::string) formName);
  if(it == forms.end()) {
    return SGX_ERROR_INVALID_PARAMETER;
  }
  form f = it->second;
  if(!f.validated) {
    return SGX_ERROR_INVALID_PARAMETER;
  }

  std::string str_form = parse_form_secure(f, p_gcm_mac); 
  if(str_form == "-1") {
    return SGX_ERROR_INVALID_PARAMETER;
  }
  memcpy(dest, str_form.c_str(), str_form.length()+1);
  return SGX_SUCCESS;
}

int t = 5;
int s = 6;
void js_print(CScriptVar *v, void *userdata) {
    printf_enc("> %s\n", v->getParameter("text")->getString().c_str());
}

// void js_print_t(CScriptVar *v, void *userdata) {
//     printf_enc("test\n");
//     printf_enc("> %s\n", *(int*)userdata);
// }


void js_update_form(CScriptVar *v, void *userdata) {
    std::string formName = v->getParameter("formName")->getString();
    std::string inputName = v->getParameter("inputName")->getString();
    std::string val = v->getParameter("val")->getString();
    std::map<std::string, form>::iterator it;
    it = forms.find((std::string) formName);
    if(it == forms.end()) {
        printf_enc("Invalid form name: %d\n", formName);
        return;
    }
    form f = it->second;
    std::map<std::string, input>::iterator it2;
    it2 = f.inputs.find((std::string) inputName);
    if(it2 == f.inputs.end()) {
        printf_enc("Invalid input name: %d\n", inputName);
        return;
    }

    it2->second.value =val;
    printf_enc("set %s to %s\n", it2->first, val);
}

void js_dump(CScriptVar *v, void *userdata) {
    CTinyJS *js = (CTinyJS*)userdata;
    js->root->trace(">  ");
}

sgx_status_t run_js(char* code, size_t len){
    std::string str_forms = "";
    for(std::map<std::string, form>::iterator it = forms.begin();
    it != forms.end(); ++it)
    {
        std::string name = it->first;
        form f = it->second;
        if(!f.validated) {
            continue;
        }
        std::string form = parse_form(it->second, true);
        str_forms += name + " = " + form + ";";
    }
    str_forms = "print('working');update_form('tmp', 'tst', 'working');"; //remove long-term

    char tmp[len];
    memcpy(tmp, code, len);
    std::string enc_code = std::string(tmp);
    enc_code = str_forms + enc_code;
    std::string res;
    CTinyJS *js = new CTinyJS();
    registerFunctions(js);
    js->addNative("function print(text)", &js_print, 0);
    js->addNative("function dump()", &js_dump, js);
    js->addNative("function update_form(formName, inputName, val)", &js_update_form, js);

    //js->addNative("function print_t()", &js_print_t, &t);
    try {
        //js->execute("print_t()");
        //t = 6;
        //js->execute("print_t()");
        //js->execute("var lets_quit = 0; function quit() { lets_quit = 1; }");
        // js->execute("print(\n\"Interactive mode... Type quit(); to exit, or print(...); to print something, or dump() to dump the symbol table!\");");
        res = js->evaluate(enc_code);
    } catch (CScriptException *e) {
        printf_enc("ERROR: %s\n", e->text.c_str());
        return SGX_ERROR_UNEXPECTED;
    }
    printf_enc("testing inside: %s\n", res);
    memcpy(code, res.c_str(), res.length()+1);
    // printf_enc("testing: %s", res);
  delete js;
#ifdef _WIN32
#ifdef _DEBUG
  _CrtDumpMemoryLeaks();
#endif
#endif
  return SGX_SUCCESS;
}

sgx_status_t get_keyboard_chars(uint8_t *p_src, uint32_t src_len, uint8_t *p_iv,  sgx_aes_gcm_128bit_tag_t *p_in_mac){
    if(&curForm == &nullForm || &curInput == &nullInput) {
        printf_enc("No input in focus.");
        return SGX_ERROR_INVALID_PARAMETER;
    }

    sgx_status_t status;
    printf_enc("Executing gcm_decrypt function from enclave...");
    const sgx_aes_gcm_128bit_key_t p_key = {
        0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
        0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
        };
    
    /*
    for (int i=0; i<4; i++){
        printf("Cipher:%x", p_src[i]);
    }
    
    for (int i=0; i<sizeof(p_in_mac)/sizeof(p_in_mac[0]); i++){
        printf("Tag:%x", p_in_mac[i]);
    }
    */
    uint8_t new_char[src_len]; 
    status = sgx_rijndael128GCM_decrypt(&p_key,p_src, src_len, &new_char[0], p_iv, 12, NULL, 0, p_in_mac);
    for (int i=0; i<src_len; i++){
        printf_enc("Decrypted Characters(in Enclave):%x", new_char[i]);
    }
    
    printf_enc("Status_decrypt: %x", status);
    if(status != SGX_SUCCESS) {
        return status;
    }
    curInput.value += (std::string) (char*) &new_char;

    return status;
}

//START OF KEYBOARD STUFF
sgx_status_t get_keyboard_chars(uint8_t *p_src){
    
    if(&curForm == &nullForm || &curInput == &nullInput) {
        printf_enc("No input in focus.");
        return SGX_ERROR_INVALID_PARAMETER;
    }


 //call function to get nextchar (decrypted)
    uint8_t ciphertext[1];
    uint8_t iv[12];
    uint8_t tag[16];

  ciphertext[0] = p_src[0]; 
  
  for (int i = 1; i<17; i++){
    tag[i-1] = p_src[i];
  }
  for (int i = 17; i<29; i++){
    iv[i-17] = p_src[i];  
  }

    uint8_t p_char[1]; //initialize a buffer
    
    sgx_status_t status;
    
    status = gcm_decrypt(&ciphertext[0],1,&p_char[0], &iv[0], &tag);
    if (p_char[0] == 0x7A){//letter z
            p_char[0] = -1; //basically doing nothing
    }
    else{
        curInput.value += (std::string) (char*) &p_char;
    }
    printf_enc("Char obtained: %x", p_char[0] );    
    printf_enc("new value: %s", curInput.value);
    return status;
}

sgx_status_t gcm_decrypt(uint8_t *p_src, uint32_t src_len, uint8_t *p_dst, uint8_t *p_iv,  sgx_aes_gcm_128bit_tag_t *p_in_mac){
    sgx_status_t status;
    printf_enc("Executing gcm_decrypt function from enclave...");
    const sgx_aes_gcm_128bit_key_t p_key = {
         0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
         0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
        };
    
    status = sgx_rijndael128GCM_decrypt(&p_key,p_src, src_len, p_dst, p_iv, 12, NULL, 0, p_in_mac);
    for (int i=0; i<src_len; i++){
        printf_enc("Decrypted Characters(in Enclave):%x", p_dst[i]);
    }
    
    printf_enc("Status_decrypt: %x\n", status);
    return status;
}