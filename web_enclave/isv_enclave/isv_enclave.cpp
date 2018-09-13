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
#include <stdio.h> /* vsnprintf */

#include <assert.h>
#include "isv_enclave.h"
#include "isv_enclave_t.h"
#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"
#include "string.h"
#include "TinyJS.h"
#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"
#include "sgx_tseal.h"
#include <bitset>
#include "sgx_thread.h"


#include <map>
#include <stdlib.h>

#define FOCUS_ON 1
#define FOCUS_OFF 0


static sgx_ec256_private_t g_sp_priv_key = {
    {
        0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
        0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
        0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
        0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
    }
};
    
    // This is the public EC key of SP, this key is hard coded in isv_enclave.
    // It is based on NIST P-256 curve. Not used in the SP code.
static sgx_ec256_public_t g_sp_pub_key = {
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

static const sgx_ec256_public_t test_p_key = {
    {0xb9, 0x06, 0xb2, 0xe1, 0x03, 0xcc, 0x7b, 0x5d,
    0x00, 0xa4, 0xa9, 0x3b, 0xb7, 0x73, 0xf7, 0x24,
    0x67, 0x33, 0x51, 0x49, 0x09, 0xcd, 0x5c, 0xe0,
    0xf2, 0xce, 0x31, 0x76, 0x1a, 0xea, 0xcf, 0x44},
    {0xc2, 0x8b, 0x3a, 0xf0, 0x3f, 0x61, 0xd9, 0xad,
    0xb8, 0x60, 0x96, 0x3c, 0xdc, 0x94, 0x5d, 0x82,
    0x82, 0x3d, 0x0a, 0x01, 0x2f, 0xda, 0x9e, 0xba,
    0x11, 0xf4, 0xc4, 0x41, 0xd3, 0xe0, 0x2a, 0xf8}
};
//New--this should be used to sign foms/js for validation
static sgx_ec256_private_t test_priv = {
    {0x97, 0x56, 0xc6, 0x16, 0xd7, 0xd5, 0xf2, 0xf6, 
    0xb9, 0x8c, 0xc0, 0x32, 0x3f, 0x17, 0xab, 0x95, 
    0x53, 0x12, 0x95, 0x13, 0xf4, 0x67, 0x75, 0xa1, 
    0x43, 0x5a, 0x00, 0x21, 0xf5, 0x52, 0x24, 0xcc}
};

/* =========================
 * Symmetric Key for NET I/O
 * =========================
 *
 * Used to store the secret passed by the SP in the sample code. The
 * size is forced to be 8 bytes. Expected value is 
 * 0x01,0x02,0x03,0x04,0x0x5,0x0x6,0x0x7
 */
uint8_t g_secret[8] = {0};
short ENCLAVE_STATE;
sgx_thread_mutex_t * curInputMutex;



//------------------remote attestation stuff---------------------

#ifdef SUPPLIED_KEY_DERIVATION

#pragma message("Supplied key derivation function is used.")

typedef struct _hash_buffer_t
{
    uint8_t counter[4];
    sgx_ec256_dh_shared_t shared_secret;
    uint8_t algorithm_id[4];
} hash_buffer_t;

const char ID_U[] = "SGXRAENCLAVE";
const char ID_V[] = "SGXRASERVER";

FILE *enc_times = fopen("enc_times.csv", "w+");

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
        hash_buffer.shared_secret.s[i] = p_shared_key->s[sizeof(p_shared_key->s) - 1 - i];
    }

    sgx_ret = sgx_sha256_init(&sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t *)&hash_buffer, sizeof(hash_buffer_t), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t *)&ID_U, sizeof(ID_U), sha_context);
    if (sgx_ret != SGX_SUCCESS)
    {
        sgx_sha256_close(sha_context);
        return false;
    }
    sgx_ret = sgx_sha256_update((uint8_t *)&ID_V, sizeof(ID_V), sha_context);
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

    assert(sizeof(sgx_ec_key_128bit_t) * 2 == sizeof(sgx_sha256_hash_t));
    memcpy(first_derived_key, &key_material, sizeof(sgx_ec_key_128bit_t));
    memcpy(second_derived_key, (uint8_t *)&key_material + sizeof(sgx_ec_key_128bit_t), sizeof(sgx_ec_key_128bit_t));

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

sgx_status_t key_derivation(const sgx_ec256_dh_shared_t *shared_key,
                            uint16_t kdf_id,
                            sgx_ec_key_128bit_t *smk_key,
                            sgx_ec_key_128bit_t *sk_key,
                            sgx_ec_key_128bit_t *mk_key,
                            sgx_ec_key_128bit_t *vk_key)
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
#pragma message("Default key derivation function is used.")
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

void printf_time(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_time(buf);
}

void printf_hex(const char *fmt, size_t length)
{
    ocall_print_hex(fmt, length);
}

std::string intToString(int i)
{
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
    sgx_thread_mutex_init(curInputMutex, NULL); 
    // isv enclave call to trusted key exchange library.
    sgx_status_t ret;
    if (b_pse)
    {
        int busy_retry_times = 2;
        do
        {
            ret = sgx_create_pse_session();
        } while (ret == SGX_ERROR_BUSY && busy_retry_times--);
        if (ret != SGX_SUCCESS)
            return ret;
    }
#ifdef SUPPLIED_KEY_DERIVATION
    ret = sgx_ra_init_ex(&g_sp_pub_key, b_pse, key_derivation, p_context);
#else
    ret = sgx_ra_init(&g_sp_pub_key, b_pse, p_context);
#endif
    if (b_pse)
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
                                   uint8_t *p_message,
                                   size_t message_size,
                                   uint8_t *p_mac,
                                   size_t mac_size)
{
    sgx_status_t ret;
    sgx_ec_key_128bit_t mk_key;

    if (mac_size != sizeof(sgx_mac_t))
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }
    if (message_size > UINT32_MAX)
    {
        ret = SGX_ERROR_INVALID_PARAMETER;
        return ret;
    }

    do
    {
        uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};

        ret = sgx_ra_get_keys(context, SGX_RA_KEY_MK, &mk_key);
        if (SGX_SUCCESS != ret)
        {
            break;
        }
        ret = sgx_rijndael128_cmac_msg(&mk_key,
                                       p_message,
                                       (uint32_t)message_size,
                                       &mac);
        if (SGX_SUCCESS != ret)
        {
            break;
        }
        if (0 == consttime_memequal(p_mac, mac, sizeof(mac)))
        {
            ret = SGX_ERROR_MAC_MISMATCH;
            break;
        }

    } while (0);

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

    do
    {
        if (secret_size != 8)
        {
            ret = SGX_ERROR_INVALID_PARAMETER;
            break;
        }

        ret = sgx_ra_get_keys(context, SGX_RA_KEY_SK, &sk_key);
        if (SGX_SUCCESS != ret)
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
                                         (const sgx_aes_gcm_128bit_tag_t *)(p_gcm_mac));

        uint32_t i;
        bool secret_match = true;
        for (i = 0; i < secret_size; i++)
        {
            if (g_secret[i] != i)
            {
                secret_match = false;
            }
        }

        if (!secret_match)
        {
            ret = SGX_ERROR_UNEXPECTED;
        }
        // Once the server has the shared secret, it should be sealed to
        // persistent storage for future use. This will prevents having to
        // perform remote attestation until the secret goes stale. Once the
        // enclave is created again, the secret can be unsealed.
    } while (0);
    return ret;
}

//----------------------our stuff------------------------//
static const form nullForm = {};
static const input nullInput = {};

//entry point to form datastructure
std::map<std::string, form> forms;
std::string scripts = "";

//these track the form/input currently in focus
form curForm = nullForm;
input curInput = nullInput;

std::string origin = "";
int revNum = 0;

/*
    Given a form, prints the number of elements in it
    and each input field's name
*/
void printForm(form f)
{
    printf_enc("# elements in form: %d", f.inputs.size());
    for (std::map<std::string, input>::const_iterator it = f.inputs.begin();
         it != f.inputs.end(); ++it)
    {
        printf_enc("input name: %s", it->first.c_str());
    }
}

/*
    Given a form, parses the form into a JSON-interpretable string. Note that for
    validation to succeed the JS object signed must exactly match the parsed form. 
    
    Params:
        --include vals: if true, include the field values in the 
                        parsed form (should be false for validation) 

*/
std::string parse_form(form f, bool include_vals)
{
    printForm(f);
    std::string start = "{\"formname\": \"" + f.name + "\", "; //"formname" is a reserved input name
    std::string end = "}";
    std::string parsed = "" + start;

    for (std::map<std::string, input>::const_iterator it = f.inputs.begin();
         it != f.inputs.end(); ++it)
    {
        std::string key = it->first;
        std::string value = "";
        if (include_vals || key == "formName")
        {
            value = it->second.value;
        }
        std::string pair = std::string("\"") + key + std::string("\": \"") + value + std::string("\", ");
        parsed = parsed + pair;
        if (key == "password")
        {
            parsed.pop_back();
        }
    }
    parsed.pop_back();
    parsed = parsed + end;
    return parsed;
}

//copies a string into the enclave
std::string copyString(const char *s, size_t len)
{
    //char es_temp[len];
    //memcpy(es_temp, s, len);
    //std::string es = std::string(es_temp);
    std::string es = s;
    return es;
}



/*
    Validates a form or JS program (or any arbitrary data, really). Validation
    involves hashing the message and then verifiying that the signiture is valid.

    Params:
        --p_message: a pointer to the data being validated
        --message_size: the length of the data
        --p_signature: a pointer to the signature
*/

sgx_status_t validate(uint8_t *p_message, uint32_t message_size,
                      sgx_ec256_signature_t *p_signature)
{
    char *sig = (char *)p_signature;
    for (int i = 0; i < sizeof(sgx_ec256_signature_t); i++) {
        unsigned char x = sig[i];
        printf_enc("b: %x", x);
    }
    sgx_ecc_state_handle_t ecc_handle;
    sgx_status_t sample_ret = sgx_ecc256_open_context(&ecc_handle);
    if (SGX_SUCCESS != sample_ret)
    {
        printf_enc("\nError, cannot get ECC context");
    }
    uint8_t result;
    //sgx_sha256_hash_t hash;
    sgx_status_t ret;// = sgx_sha256_msg(p_message, message_size, &hash);
    /*
    if (ret != SGX_SUCCESS)
    {
        return ret;
    }*/

    //printf_enc("hash: %s signature: %s", hash, p_signature);

    //ret = sgx_ecdsa_sign(p_message, strlen((const char*)p_message), &g_sp_priv_key, p_signature, ecc_handle);

    printf_hex((char *)p_signature, (size_t)sizeof(sgx_ec256_signature_t));

    ret = sgx_ecdsa_verify(p_message, strlen((const char*)p_message), &g_sp_pub_key, p_signature,
                           &result, ecc_handle);
    if (ecc_handle)
    {
        sgx_ecc256_close_context(ecc_handle);
    }
    if (ret != SGX_SUCCESS)
    {
        printf_enc("err, verify failed: %d", ret);
        return ret;
    }
    if ((sgx_generic_ecresult_t)result != SGX_EC_VALID)
    {
        printf_enc("err, bad sig: %d", result);
        return SGX_ERROR_INVALID_PARAMETER;
    }
    return SGX_SUCCESS;
}

//add a new script
sgx_status_t add_script(const char* sign, int lenSign, const char* script, int lenScript){
    //verify signature
    printf_enc("VALIDATING SCRIPT");
    //copies how we do for forms. Might both be broken? ~saba
    if (SGX_SUCCESS != validate((uint8_t *)script, lenScript, (sgx_ec256_signature_t *)sign))
    {
        //return failure
        printf_enc("SCRIPT SIGNATURE DOES NOT MATCH");
        //return SGX_ERROR_INVALID_PARAMETER;
    }
    
    //add js to the big list of JS scripts.
    //for now they are all functions, so this is ok. need a different way in general ~saba
    std::string newScript(script);
    //printf_enc("adding script: %s\n", newScript.c_str());
    scripts += newScript + "\n";
    
    return SGX_SUCCESS;
}

//adds a new form to the map of forms
sgx_status_t add_form(const char *name, size_t len,
                      const char *this_origin, size_t origin_len, uint16_t x, uint16_t y, 
                      const char* onsub, size_t onsubLen)
{


    std::string oName = copyString(this_origin, origin_len);

    //if the manager is attempting to change the origin, something has gone wrong
    if (origin != "" && origin != oName)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    else
    {
        origin = oName;
        revNum = 0;//We don't have a server atm so this starts at 0
    }

    std::string eName = copyString(name, len);
    if (forms.count(eName) > 0)
    {
        return SGX_ERROR_INVALID_PARAMETER; //error if form already exists
    }
    else
    {
        std::string onsubAction = copyString(onsub, onsubLen);
        
        form new_form;
        new_form.name = eName;
        new_form.x = x;
        new_form.y = y;
        new_form.validated = false;
        new_form.onsubmit = onsubAction;
        printf_enc("added new form: %s (%d,%d)", eName.c_str(), x, y);
        input new_input;
        new_input.value = eName;
        new_input.x = 10;
        new_input.y = 10;
        new_input.width = 10;
        new_input.height = 10;
        new_input.name = "dummy form";
        new_form.validated = false; //this should be false but the feature is buggy
        forms.insert(std::pair<std::string, form>(eName, new_form));
        return SGX_SUCCESS;
    }
}

/*
    Creates a new input field in a form. All the paramaters are self-explanatory except
    val. This int signals whether the form is ready to validate. Set it to 1 when adding
    the last input field to trigger validation. Note that the p_sig_form variable is only
    used when validation occurs. Also note that after a form is validated it can no longer 
    be modified. 

*/
sgx_status_t add_input(const char *form_name, size_t len_form, const char *input_name, size_t len_input,
                       const uint8_t *p_sig_form, size_t sig_form_size, int val, uint16_t x, uint16_t y, uint16_t height, uint16_t width)
{
    std::string formName = copyString(form_name, len_form);
    std::string inputName = copyString(input_name, len_input);
    if (forms.count(formName) == 0)
    {
        printf_enc("ENCLAVE: form named: %s not found", formName.c_str());
        return SGX_ERROR_INVALID_PARAMETER; //error if form does not exist
    }
    else
    {
        std::map<std::string, form>::iterator it;
        it = forms.find((std::string)formName);
        form f = it->second;
        if (f.validated)
        { //you can't add inputs to an already validated form
            printf_enc("TRIED TO ADD TO AN ALREADY VALIDATED FORM");
            return SGX_ERROR_INVALID_PARAMETER;
        }

        if (f.inputs.count(inputName) > 0)
        {
            printf_enc("ENCLAVE: tried to add already existing input named %s to form %s", inputName.c_str(), formName.c_str());
            return SGX_ERROR_INVALID_PARAMETER; //error if input already exists
        }
        else
        {
            input new_input;
            new_input.name = inputName;
            new_input.x = x;
            new_input.y = y;
            new_input.width = width;
            new_input.height = height;
            new_input.value = "";
            f.inputs.insert(std::pair<std::string, input>(inputName, new_input));

            // after all inputs added, parse form and validate form
            if (val == 1)
            {
                std::string form = parse_form(f, false);
                printf_enc("VALIDATING: %s", form.c_str());
                if (SGX_SUCCESS != validate((uint8_t *)form.c_str(), (uint32_t)form.length(), (sgx_ec256_signature_t *)p_sig_form))
                {
                    // delete form, return failure
                    printf_enc("FORM SIGNATURE DOES NOT MATCH, %d", sizeof(sgx_ec256_signature_t));
                    f.validated = false;
                    //forms.erase((std::string) formName);
                    //return SGX_ERROR_INVALID_PARAMETER;
                }
                //this next line won't be called if the preceding return is uncommented
                //f.validated = true;
            }
            it->second = f;
            return SGX_SUCCESS;
        }
    }
}

//sets flag indicating which form/input field should stand by to recive user input
sgx_status_t onFocus(const char* formName, const char* inputName, 
                    uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  sgx_thread_mutex_lock(curInputMutex);
  std::map<std::string, form>::iterator it;
  it = forms.find((std::string) formName);
  
  if(it == forms.end()) {
    printf_enc("INPUT: invalid formname: %s", formName);
    sgx_thread_mutex_unlock(curInputMutex);
    return SGX_ERROR_INVALID_PARAMETER;
  }
  form f = it->second;
  std::map<std::string, input>::iterator it2;
  it2 = f.inputs.find((std::string) inputName);

  if(it2 == f.inputs.end()) {
    printf_enc("INPUT: invalid inputname: %s", inputName);
    sgx_thread_mutex_unlock(curInputMutex);
    return SGX_ERROR_INVALID_PARAMETER;
  }
  //printf_enc("INPUT: FORM NAME = %s", it->first.c_str());


  curForm = f;
  curInput = it2->second;
  //printf_enc("INPUT: Input name = %s which should be the same as: %s", inputName, curInput.name.c_str());
  //printf_enc("INPUT: Input Field Value = %s", curInput.value.c_str());
  //printf_enc("INPUT: Input Field X = %d", curInput.x);
  //printf_enc("INPUT: Input Field Y = %d", curInput.y);
  //printf_enc("INPUT: Input Field Width = %d", curInput.width);
  //printf_enc("INPUT: Input Field Height= %d", curInput.height);
  ENCLAVE_STATE = FOCUS_ON;
  sgx_thread_mutex_unlock(curInputMutex);
  return SGX_SUCCESS;
}

//sets flag indicating no form/input field is ready to accept user input
sgx_status_t onBlur() {
  printf_enc("BEFORE BLUR data for %s: %s: %s",curForm.name.c_str(), curInput.name.c_str(), curForm.inputs[curInput.name].value.c_str());
  sgx_thread_mutex_lock(curInputMutex);
  curForm = nullForm;
  curInput = nullInput;
  ENCLAVE_STATE = FOCUS_OFF;
  sgx_thread_mutex_unlock(curInputMutex);
  printf_enc("AFTER BLUR data for %s: %s: %s",curForm.name.c_str(), curInput.name.c_str(), curForm.inputs[curInput.name].value.c_str());

  return SGX_SUCCESS;
}

//Returns the lengh of a form in bytes. Note that this includes the values in the form's fields.
uint32_t form_len(const char *formName)
{
    std::map<std::string, form>::iterator it;
    it = forms.find((std::string)formName);
    if (it == forms.end())
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    form f = it->second;
    uint32_t len = (uint32_t)parse_form(f, true).length() + 1;
    return len;
}

//see put_secret_data for explanation of types
sgx_status_t submit_form(const char *formName, uint8_t *dest, uint32_t encr_size, uint8_t *p_gcm_mac)
{
    std::map<std::string, form>::iterator it;
    it = forms.find((std::string)formName);
    if (it == forms.end())
    {
        printf_enc("SUBMIT didnt find requested form");
        return SGX_ERROR_INVALID_PARAMETER;
    }
    form f = it->second;
    if (!f.validated)
    {
        printf_enc("SUBMIT form not validated");
        //return SGX_ERROR_INVALID_PARAMETER;
    }

    std::string str_form = parse_form(f, true);
    uint8_t aes_gcm_iv[12] = {0};
    sgx_status_t ret = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *)&g_secret,
                                                  (const uint8_t *)&str_form[0],
                                                  encr_size,
                                                  dest,
                                                  &aes_gcm_iv[0],
                                                  12,
                                                  NULL,
                                                  0,
                                                  (sgx_aes_gcm_128bit_tag_t *)(p_gcm_mac));

    if (ret != SGX_SUCCESS)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    printf_enc("SUBMIT returning normally");
    return SGX_SUCCESS;
}

//useful debugging function that decrypts and encrypted form
sgx_status_t test_decryption(uint8_t *form, uint32_t form_size, uint8_t *mac)
{
    uint8_t output[form_size] = {0};
    uint8_t aes_gcm_iv[12] = {0};
    sgx_status_t ret = sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *)&g_secret,
                                                  (const uint8_t *)&form[0],
                                                  form_size,
                                                  &output[0],
                                                  &aes_gcm_iv[0],
                                                  12,
                                                  NULL,
                                                  0,
                                                  (sgx_aes_gcm_128bit_tag_t *)mac);

    printf_enc("decrypted form\n");
    for (int i = 0; i < form_size; i++)
    {
        //printf_enc("%c", (char)output[i]);
    }
    printf_enc("\n");
}

//native function for tinyJS that enables printing
void js_print(CScriptVar *v, void *userdata)
{
    printf_enc("> %s\n", v->getParameter("text")->getString().c_str());
}

//native function updates the value in a field in a form. Note that this must be
//called in addtion to modifying the object representing the form w/in tinyJS to keep
//the "true" form and its tinyJS representation in sync.
void js_update_form(CScriptVar *v, void *userdata)
{
    std::string formName = v->getParameter("formName")->getString();
    std::string inputName = v->getParameter("inputName")->getString();
    std::string val = v->getParameter("val")->getString();
    forms[formName].inputs[inputName].value = val;
    printf_enc("set %s to %s\n", inputName.c_str(), val.c_str());
}

/*
    native function that makes HTTP requests. The request must be directed at the origin
    of the enclave. The OCALL enc_make_http_request should return a return code indicating 
    whether the request was successfully sent
*/
void js_make_http_request(CScriptVar *v, void *userdata)
{
    std::string method = v->getParameter("method")->getString();
    std::string url = v->getParameter("url")->getString();
    std::string headers = v->getParameter("headers")->getString();
    std::string post_data = v->getParameter("postData")->getString();
    std::string request_data = method + url + headers + post_data;
    if (url != origin)
    {
        printf_enc("Error: invalid origin %s", url.c_str());
        return;
    }

    uint32_t len_val = (uint32_t)request_data.length();
    uint8_t encr_val[len_val] = {0};
    uint8_t aes_gcm_iv[12] = {0};
    uint8_t p_gcm_mac[16] = {0};
    sgx_status_t ret = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *)(&g_secret),
                                                  (const uint8_t *)&request_data[0],
                                                  len_val,
                                                  &encr_val[0],
                                                  &aes_gcm_iv[0],
                                                  12,
                                                  NULL,
                                                  0,
                                                  (sgx_aes_gcm_128bit_tag_t *)(p_gcm_mac));
    if (ret != SGX_SUCCESS)
    {
        printf_enc("Error: encryption failed");
        return;
    }
    request_data = std::string((char *)&encr_val[0]);
    int ret_code = 0;
    enc_make_http_request(&ret, method.c_str(), url.c_str(), headers.c_str(), request_data.c_str(), &p_gcm_mac[0], &ret_code);
    if (ret != SGX_SUCCESS)
    {
        printf_enc("Error: request failed");
        return;
    }
    v->getReturnVar()->setInt(ret_code);
}

//this tracks whether an HTTP response is ready f
bool response_ready = false;
std::string response;

//once an HTTP response is ready use this to load the response into the enclave and
//signal that it is ready for tinyJS
void get_http_response(char *http_response, size_t response_len)
{
    response = copyString(http_response, response_len);
    response_ready = true;
}

//native function that essentially busy waits until the HTTP response is ready
void js_get_http_response(CScriptVar *v, void *userdata)
{
    if (response_ready == true)
    {
        v->getReturnVar()->setString(response);
        response_ready = false;
    }
    else
    {
        v->getReturnVar()->setUndefined();
    }
}

/*
    Native function that loads data from secure storage. A call to this function is automatically
    added to the start of every JS program run in the enclave.

    NOTE: Currently this is buggy.

    The OCALL ocall_get_file_size computes the size of buffer to allocate for the loaded 
    data.

*/
void js_load_items(CScriptVar *v, void *userdata)
{
    size_t data_len;
    ocall_get_file_size(&data_len);
    if (data_len > 0)
    {
        sgx_sealed_data_t* sealed = (sgx_sealed_data_t*)malloc(data_len);
        ocall_read_file((uint8_t*)sealed, data_len);
        //printf_enc("loading data quantity: %d\n", data_len);
        //printf_enc("reading data first bytes %x%x%x", sealed[0], sealed[1], sealed[2]);

        //uint32_t len = 610;
        uint32_t len = sgx_get_encrypt_txt_len(sealed);
        char decrypted_text[len];

        uint32_t mac_len = 4+origin.length();
        //printf_enc("len: %d", len);
        
        uint8_t* ad = (uint8_t*)malloc(mac_len);
        int oldRevNum = revNum-1;
        memcpy(ad, &oldRevNum, 4);
        memcpy(&ad[4], origin.c_str(), origin.length());

        sgx_status_t ret = sgx_unseal_data(
            sealed,
            ad,
            &mac_len,
            (uint8_t *)&decrypted_text[0],
            &len);

        if (ret != SGX_SUCCESS)
        {
            printf_enc("Error: data unsealing failed %d\n", ret);
        }
        std::string decrypted = std::string(decrypted_text);
        v->getReturnVar()->setString(decrypted);
    }
    else
    {
        v->getReturnVar()->setString("{}");
    }
    
    printf_enc("loading data: %s\n", v->getReturnVar()->getString().c_str());
    return;
}

/*
    Native function that saves to secure storage. A call to this function is automatically
    added to the end of every JS program run in the enclave.

    NOTE: Currently this is buggy.
*/
void js_save_items(CScriptVar *v, void *userdata)
{    
    std::string data_to_s = v->getParameter("data")->getString();
    uint32_t len = data_to_s.length();
    uint8_t* data_to_store = (uint8_t*)malloc(len+1);
    memcpy(data_to_store, data_to_s.c_str(), len+1);
    printf_enc("sealing data: %s\n", data_to_store);
    uint32_t sealed_size = sgx_calc_sealed_data_size(4+origin.length(), len+1);
    sgx_sealed_data_t* sealed_data = (sgx_sealed_data_t*) malloc(sealed_size);
    
    uint8_t* ad = (uint8_t*)malloc(4+origin.length());
    memcpy(ad, &revNum, 4);
    memcpy(&ad[4], origin.c_str(), origin.length());
    
    sgx_status_t ret = sgx_seal_data(
        4+origin.length(),
        ad,
        len+1,
        data_to_store,
        sealed_size,
        sealed_data);

    if (ret != SGX_SUCCESS)
    {
        printf_enc("Error: data sealing failed %d\n", ret);
    }
    //printf_enc("writing data first bytes %x%x%x", sealed_data[0], sealed_data[1], sealed_data[2]);

    ocall_write_file((uint8_t*)sealed_data, sealed_size);
    free(sealed_data);
    free(data_to_store);
    revNum++;
    return;
}

//I don't remember what this does--it comes with the tinyJS sample script
void js_dump(CScriptVar *v, void *userdata)
{
    CTinyJS *js = (CTinyJS *)userdata;
    js->root->trace(">  ");
}


//temp for safeware application
//just because I don't have the i/o system
//~saba
/*
void debug_print_form_contents(){
    printf_enc("input: %s\n", forms["safewareForm"].inputs["input"].value.c_str());
    printf_enc("matchString: %s\n", forms["safewareForm"].inputs["matchString"].value.c_str());
    printf_enc("output: %s\n", forms["safewareForm"].inputs["output"].value.c_str());
}
*/

/*
    Runs a string of JS in the tinyJS enviornment. See documentation on tinyJS at
    https://github.com/gfwilliams/tiny-js/tree/56a0c6d92b5ced9d8b2ade32eec5ddfdfdb49ef5

*/
sgx_status_t run_js(const char *formName, size_t len)
{
    std::string code = forms[formName].onsubmit;
    code += ";";
    
    printf_enc("running js! %s", code.c_str());
    //return SGX_SUCCESS;
    //TODO:Saba: add in the scripts and one line to call the desired function for the form
    //also see what's already happening here 
    
    //parse forms and add them as objects to the start of the JS code
    std::string str_forms = "";
    for (std::map<std::string, form>::iterator it = forms.begin();
         it != forms.end(); ++it)
    {
        std::string name = it->first;
        form f = it->second;
        if (!f.validated)
        {
            continue;
        }
        std::string form = parse_form(it->second, true);
        str_forms += "var " + name + " = " + form + ";";
    }

    //char tmp[len];
    //memcpy(tmp, code.c_str(), codeLen);
    //std::string enc_code = std::string(tmp);

    //printf_enc("scripts: %s\n", scripts.c_str());
    //printf_enc("str_forms: %s\n", str_forms.c_str());
    //printf_enc("code: %s\n", code.c_str());
    
    /*
     * TODO: 
     * 
     * enc_code needs to start with the content of tiny_init.js
     * and ends with the content of tiny_end.js
     *
     */
    //also add loading/saving code
    std::string start_code = "var str_data = __native_js_load_items(); var localStorage = eval(str_data);\n";
    std::string end_code = "\nstr_data = JSON.stringify(localStorage, undefined); __native_js_save_items(str_data);";
    code = start_code+"\n"+scripts +"\n"+str_forms +"\n"+ code+"\n"+end_code;

    //printf_enc("js code: %s", code.c_str());

    std::string res;
    CTinyJS *js = new CTinyJS();
    registerFunctions(js);
    js->addNative("function print(text)", js_print, 0);
    js->addNative("function dump()", js_dump, js);
    js->addNative("function update_form(formName, inputName, val)", js_update_form, js);
    js->addNative("function js_make_http_request(method, url, headers, postData)", js_make_http_request, js);
    js->addNative("function js_get_http_response()", js_get_http_response, js);
    js->addNative("function __native_js_load_items()", js_load_items, js);
    js->addNative("function __native_js_save_items(data)", js_save_items, js);
    try
    {
        js->execute(code);
    }
    catch (CScriptException *e)
    {
        printf_enc("ERROR: %s\n", e->text.c_str());
        return SGX_ERROR_UNEXPECTED;
    }
    //res = js->evaluate("result"); //note: result is just a variable defined in the code
    //memcpy(code, res.c_str(), res.length() + 1); 
    delete js;
    return SGX_SUCCESS;
}

//START OF KEYBOARD STUFF
sgx_status_t get_keyboard_chars(uint8_t *p_src){
    //printf_time("recieved char");
    sgx_thread_mutex_lock(curInputMutex);
    
    if(ENCLAVE_STATE == FOCUS_OFF) {
        //printf_enc("No input in focus.");
        sgx_thread_mutex_unlock(curInputMutex);
        return SGX_ERROR_INVALID_PARAMETER;
    }

    //call function to get nextchar (decrypted)
    uint8_t ciphertext[1];
    uint8_t iv[12];
    uint8_t tag[16];

    ciphertext[0] = p_src[0];
    printf_enc("%c", ciphertext[0]);

    for (int i = 1; i < 17; i++)
    {
        tag[i - 1] = p_src[i];
    }
    for (int i = 17; i < 29; i++)
    {
        iv[i - 17] = p_src[i];
    }

    uint8_t p_char[1]; //initialize a buffer

    sgx_status_t status;
    //enclave_times << "key decrypt => form render," << timeNow() << ",";


    status = gcm_decrypt(&ciphertext[0], 1, &p_char[0], &iv[0], &tag);
    if (p_char[0] == 0x7A)
    {                   //letter z
        p_char[0] = -1; //basically doing nothing
    }
    else if (p_char[0] == 0x71)
    {
        if (curInput.value.length() != 0)
        {
            curInput.value = curInput.value.substr(0, curInput.value.length() - 1);
        }
    }
    else if (p_char[0] != 0)
    {
        curInput.value += p_char[0];

        printf_time("added ch to enc buffer: %s", curInput.value.c_str());
        printf_enc("adding ch tp enc buffer: %c", p_char[0]);
    } else if (p_char[0] == 0) {
        printf_enc("detected an issue with decryption, not adding this char to our string");
    }
    printf_enc("KEYBOARD: Form name = %s Input name = %s, value = %s",curForm.name.c_str(), curInput.name.c_str(),  curInput.value.c_str());
    //printf_enc("KEYBOARD: Input Field X = %d", curInput.x);
    //printf_enc("KEYBOARD: Input Field Y = %d", curInput.y);
    //printf_enc("KEYBOARD: Input Field Width = %d", curInput.width);
    //printf_enc("KEYBOARD: Input Field Height= %d", curInput.height);
    //printf_enc("KEYBOARD: Char obtained: %x", p_char[0] );
    //printf_enc("KEYBOARD: new value for input = %s", curInput.value.c_str());


    // std::string fname = "loginform";
    // std::string iname = "username";
    // std::string iname2 = "password";
    //printf_enc("BEFORE ADDKEY data for loginform: username: %s", forms[fname].inputs[iname].value.c_str());
    //printf_enc("BEFORE ADDKEY data for loginform: password: %s", forms[fname].inputs[iname2].value.c_str());
    forms[curForm.name].inputs[curInput.name] = curInput;
    //printf_enc("AFTER ADDKEY data for loginform: username: %s", forms[fname].inputs[iname].value.c_str());
    //printf_enc("AFTER ADDKEY data for loginform: password: %s", forms[fname].inputs[iname2].value.c_str());

    sgx_thread_mutex_unlock(curInputMutex);
    return status;
}

sgx_status_t gcm_decrypt(uint8_t *p_src, uint32_t src_len, uint8_t *p_dst, uint8_t *p_iv, sgx_aes_gcm_128bit_tag_t *p_in_mac)
{
    sgx_status_t status;
    //printf_enc("Executing gcm_decrypt function from enclave...");
    const sgx_aes_gcm_128bit_key_t p_key = {
        0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
        0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85};

    status = sgx_rijndael128GCM_decrypt(&p_key, p_src, src_len, p_dst, p_iv, 12, NULL, 0, p_in_mac);
    for (int i = 0; i < src_len; i++)
    {
        //printf_enc("Decrypted Characters(in Enclave):%x", p_dst[i]);
    }

    printf_enc("Status_decrypt: %x\n", status);
    return status;
}
