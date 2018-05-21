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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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

typedef struct input
{
    std::string value;
    int x;
    int y;
} input;

typedef struct form
{
    std::map<std::string, input> inputs;
} form;

static const form nullForm = {};
static const input nullInput = {};

std::map<std::string, form> forms;
form curForm = nullForm;
input curInput = nullInput;

void printForm(form f){
    printf_enc("# elements in form: %d\n", f.inputs.size());
    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
    it != f.inputs.end(); ++it)
    {
        printf_enc("input name: %s\n", it->first);
    }
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
                      uint8_t* p_mac, size_t mac_size) {
    
    if(mac_size != sizeof(sgx_mac_t))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};
    sgx_status_t ret  = sgx_rijndael128_cmac_msg(
            ((const sgx_cmac_128bit_key_t*) (&mac_secret)),
            p_message,
            message_size,
            &mac
        );

    if(SGX_SUCCESS != ret)
        {
            return SGX_ERROR_INVALID_PARAMETER;
        }
    if(0 == consttime_memequal(p_mac, mac, sizeof(mac)))
        {
            ret = SGX_ERROR_MAC_MISMATCH;
            return ret;
        }

    return ret;
}


//adds a new form to the internal list of forms
sgx_status_t add_form(char* name, size_t len, uint8_t *p_mac, size_t mac_size) {
    if(SGX_SUCCESS != validate((uint8_t*) name, (uint32_t) len, p_mac, mac_size)){
        return SGX_ERROR_INVALID_PARAMETER;
    }

    std::string eName = copyString(name, len);
    if(forms.count(eName) > 0) {
      return SGX_ERROR_INVALID_PARAMETER; //error if form already exists
    } else {
        form new_form;
        forms.insert(std::pair<std::string, form>(eName, new_form));
        return SGX_SUCCESS;
    }
}

//adds a new input field to a form
sgx_status_t add_input(char * name, size_t len1, char* input_i, size_t len2,
                    uint8_t *p_mac_form, size_t mac_form_size, 
                    uint8_t *p_mac_input, size_t mac_input_size) {
    if(SGX_SUCCESS != validate((uint8_t*) name, (uint32_t) len1, p_mac_form, mac_form_size)
            || SGX_SUCCESS != validate((uint8_t*) input_i, (uint32_t) len2, p_mac_input, mac_input_size)){
        return SGX_ERROR_INVALID_PARAMETER;
    }

    std::string eName = copyString(name, len1);
    std::string eInput = copyString(input_i, len2);
    if(forms.count(eName) == 0) {
      return SGX_ERROR_INVALID_PARAMETER; //error if form does not exist
    } else {
        std::map<std::string, form>::iterator it;
        it = forms.find((std::string) eName);
        form f = it->second;
        std::map<std::string, input> inputs = f.inputs;
        if(inputs.count(eInput) > 0) {
            return SGX_ERROR_INVALID_PARAMETER; //error if input already exists
        } else {
            input new_input;
            inputs.insert(std::pair<std::string, input>(eInput, new_input));
            f.inputs = inputs;
            it->second = f;
            return SGX_SUCCESS;
        }
    }
}

//sets flag indicating which form/input field should stand by to recive user input
sgx_status_t onFocus(const char* formName, const char* inputName) {
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
  return SGX_SUCCESS;
}

//sets flag indicating no form/input field is ready to accept user input
void onBlur() {
  curForm = nullForm;
  curInput = nullInput;
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

  uint8_t aes_gcm_iv[12] = {0};
  uint8_t str_form[1] = {0}; //TODO: change this
  uint32_t len_str_form = 1;
  sgx_status_t ret = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_tag_t *) (&g_secret),
                (const uint8_t*) &str_form,
                len_str_form,  
                dest,
                &aes_gcm_iv[0],
                12,
                NULL,
                0,
                (sgx_aes_gcm_128bit_tag_t *) (p_gcm_mac));

  return ret;
}

int t = 5;
void js_print(CScriptVar *v, void *userdata) {
    printf_enc("> %s\n", v->getParameter("text")->getString().c_str());
}

// void js_print_t(CScriptVar *v, void *userdata) {
//     printf_enc("test\n");
//     printf_enc("> %s\n", *(int*)userdata);
// }


void js_dump(CScriptVar *v, void *userdata) {
    CTinyJS *js = (CTinyJS*)userdata;
    js->root->trace(">  ");
}

sgx_status_t run_js(char* code, size_t len){

    std::string enc_code = copyString(code, len);
    CTinyJS *js = new CTinyJS();
    registerFunctions(js);
    std::string res;
    js->addNative("function print(text)", &js_print, 0);
    js->addNative("function dump()", &js_dump, js);
    //js->addNative("function print_t()", &js_print_t, &t);
    try {
        //js->execute("print_t()");
        //t = 6;
        //js->execute("print_t()");
        // js->execute("var lets_quit = 0; function quit() { lets_quit = 1; }");
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




