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
#include "sgx_tseal.h"
#include <bitset>
#include "sgx_thread.h"

#include <map>
#include <stdlib.h>

#define FOCUS_ON 1
#define FOCUS_OFF 0

//#define timeNow() std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count()

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
//original
static const sgx_ec256_public_t test_p_key = {
    {
        0xae, 0xc5, 0x8b, 0x2e, 0x23, 0x5b, 0xb7, 0xe7,
        0x9f, 0x1a, 0xd0, 0x5e, 0x4b, 0x7c, 0x5e, 0xf0,
        0x7c, 0x13, 0xdb, 0xf3, 0xee, 0xed, 0xbd, 0x7f,
        0x1f, 0xdd, 0x55, 0x2c, 0x7e, 0x79, 0x43, 0x58
    },
    {
        0xf6, 0x07, 0x87, 0xef, 0x5d, 0xb4, 0x47, 0xab,
        0x54, 0x46, 0x37, 0xb0, 0x5a, 0x19, 0x19, 0xda,
        0xb6, 0x98, 0xd8, 0xeb, 0xeb, 0xc3, 0x81, 0x8f,
        0x2c, 0xd2, 0x74, 0x7d, 0x6a, 0x3c, 0x9d, 0xf7
    }
};
//new--this should be used to validate forms/js--at some point, 
//we can replace g_sp_pub_key with this
/*static const sgx_ec256_public_t test_p_key = {
    {
		0x79, 0x9d, 0x98, 0x91, 0x45, 0x69, 0xa6, 0xb0, 
		0x90, 0x56, 0xfc, 0xfe, 0x3f, 0xe0, 0xb5, 0xa4, 
		0x9, 0x5d, 0x91, 0x85, 0x4b, 0x90, 0x0, 0x1c, 
		0xf3, 0x67, 0xcd, 0x82, 0x8f, 0xe1, 0x32, 0x50	
    },
    {
		0xb0, 0x76, 0x3f, 0x55, 0xb, 0x9e, 0x62, 0x3c, 
		0x2d, 0x29, 0xd0, 0x87, 0xc, 0x6d, 0x2c, 0xa4, 
		0xf8, 0xa9, 0x24, 0xc, 0x75, 0xab, 0x70, 0xd, 
		0x1f, 0x21, 0x7f, 0xc3, 0xe8, 0x80, 0xdc, 0x94
    }
};*/

//original
/*static sgx_ec256_private_t test_priv = {
    {
        0x7d, 0xf4, 0xb0, 0xd1, 0x36, 0xbf, 0xb5, 0x97, 
        0xe1, 0x79, 0xb2, 0xee, 0xc8, 0x7a, 0x7b, 0xe2,
        0x53, 0xb1, 0xbe, 0x2c, 0xa8, 0x2f, 0x34, 0x2e,
        0x7a, 0x3f, 0xd1, 0x96, 0xa7, 0xe5, 0x8b, 0xe5
    }
}; */


//new--this should be used to sign foms/js for validation
static sgx_ec256_private_t test_priv = {
    {
		0x47, 0x2b, 0xde, 0xa4, 0x66, 0x7f, 0xc0, 0x52,
		0xe0, 0x4a, 0x6d, 0x77, 0xda, 0xe7, 0x48, 0xba, 
		0x67, 0x9d, 0x22, 0x45, 0x1c, 0xf5, 0x8, 0xae, 
		0xb, 0x7f, 0x61, 0x84, 0xa2, 0xa3, 0xe0, 0x9b
	}
};


// Used to store the secret passed by the SP in the sample code. The
// size is forced to be 8 bytes. Expected value is
// 0x01,0x02,0x03,0x04,0x0x5,0x0x6,0x0x7
uint8_t g_secret[8] = {0};
short ENCLAVE_STATE;
sgx_thread_mutex_t * curInputMutex;




//------------------remote attestation stuff---------------------

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

FILE * enc_times = fopen("enc_times.csv", "w+");


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
    sgx_thread_mutex_init(curInputMutex, NULL); 
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

//----------------------our stuff------------------------//
static const form nullForm = {};
static const input nullInput = {};

//entry point to form datastructure
std::map<std::string, form> forms;

//these track the form/input currently in focus
form curForm = nullForm;
input curInput = nullInput;

std::string origin = "";

/*
    Given a form, prints the number of elements in it
    and each input field's name
*/
void printForm(form f){
    printf_enc("# elements in form: %d", f.inputs.size());
    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
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
std::string parse_form(form f, bool include_vals) {
    printForm(f);
    std::string start = "{\"formname\": \"" + f.name + "\", "; //"formname" is a reserved input name 
    std::string end = "}";                                     
    std::string parsed = "" + start;

    for(std::map<std::string, input>::const_iterator it = f.inputs.begin();
    it != f.inputs.end(); ++it)
    {
        std::string key = it->first;
        std::string value = "";
        if(include_vals || key == "formName") {
            value = it->second.value;
        } 
        std::string pair = std::string("\"") + key + std::string("\": \"") + value + std::string("\", ");
        parsed = parsed + pair;
        if(key == "password") {
            parsed.pop_back();
        }
    }
    parsed.pop_back();
    parsed = parsed + end;
    return parsed; 
}


//copies a string into the enclave
std::string copyString(const char* s, size_t len) {
    char es_temp[len];
    memcpy(es_temp, s, len);
    std::string es = std::string(es_temp);
    return es;
}

/*
    Validates a form or JS program (or any arbitrary data, really). Validation
    involves hashing the messagae and then verifiying that the signiture is valid.

    Params:
        --p_message: a pointer to the data being validated
        --message_size: the length of the data
        --p_signiture: a pointer to the signiture
*/

sgx_status_t validate(uint8_t *p_message, uint32_t message_size,
                      sgx_ec256_signature_t* p_signature) {
    sgx_ecc_state_handle_t ecc_handle;
    sgx_status_t sample_ret = sgx_ecc256_open_context(&ecc_handle);
    if(SGX_SUCCESS != sample_ret)
    {
        printf_enc("\nError, cannot get ECC context");
    }
    uint8_t result;
    sgx_sha256_hash_t hash;
    sgx_status_t ret = sgx_sha256_msg(p_message,message_size,&hash);

    if(ret != SGX_SUCCESS) {
        return ret;
    }

    ret = sgx_ecdsa_verify((uint8_t*) &hash, (uint32_t) sizeof(sgx_sha256_hash_t),&test_p_key,p_signature,
            &result,ecc_handle);
    if(ecc_handle)
    {
        sgx_ecc256_close_context(ecc_handle);
    }
    if(ret != SGX_SUCCESS) {
        return ret;
    }
    if((sgx_generic_ecresult_t) result != SGX_EC_VALID) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    return ret;
}

//adds a new form to the map of forms
sgx_status_t add_form(const char* name, size_t len, 
                        const char* this_origin, size_t origin_len, uint16_t x, uint16_t y) {


    std::string oName = copyString(this_origin, origin_len);
    
    //if the manager is attempting to change the origin, something has gone wrong
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
        new_form.name = eName;
        new_form.x = x;
        new_form.y = y;
        new_form.validated = false;
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
sgx_status_t add_input(const char * form_name, size_t len_form, const char* input_name, size_t len_input,
                    const uint8_t *p_sig_form, size_t sig_form_size, int val, uint16_t x, uint16_t y, uint16_t height, uint16_t width) {
    std::string formName = copyString(form_name, len_form);
    std::string inputName = copyString(input_name, len_input);
    if(forms.count(formName) == 0) {
        printf_enc("ENCLAVE: form named: %s not found", formName.c_str());
        return SGX_ERROR_INVALID_PARAMETER; //error if form does not exist
    } else {  
        std::map<std::string, form>::iterator it;
        it = forms.find((std::string) formName);
        form f = it->second;
        if(f.validated) { //you can't add inputs to an already validated form
            printf_enc("TRIED TO ADD TO AN ALREADY VALIDATED FORM");
            return SGX_ERROR_INVALID_PARAMETER;
        }


        if(f.inputs.count(inputName) > 0) {
            printf_enc("ENCLAVE: tried to add already existing input named %s to form %s", inputName.c_str(), formName.c_str());
            return SGX_ERROR_INVALID_PARAMETER; //error if input already exists
        } else {
            input new_input;
            new_input.name = inputName;
            new_input.x = x;
            new_input.y = y;
            new_input.width = width;
            new_input.height = height;
            new_input.value = "";
            f.inputs.insert(std::pair<std::string, input>(inputName, new_input));


            // after all inputs added, parse form and validate form
            if(val == 1) {
                std::string form = parse_form(f, false);
                printf_enc("VALIDATING: %s", form.c_str());
                if(SGX_SUCCESS == validate((uint8_t*) form.c_str(), (uint32_t) form.length(), (sgx_ec256_signature_t*) p_sig_form)) {        
                    f.validated = true;
                }
                else {
                    // delete form, return failure
                    printf_enc("FORM SIGNATURE DOES NOT MATCH");
                    f.validated = true;
                    //forms.erase((std::string) formName);
                    //return SGX_ERROR_INVALID_PARAMETER;
                }
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
  std::string fname = "loginform";
  std::string iname = "username";
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
uint32_t form_len(const char* formName) {
  std::map<std::string, form>::iterator it;
  it = forms.find((std::string) formName);
  if(it == forms.end()) {
    return SGX_ERROR_INVALID_PARAMETER;
  }
  form f = it->second;
  uint32_t len = (uint32_t) parse_form(f, true).length() + 1;
  return len;
}

//see put_secret_data for explanation of types
sgx_status_t submit_form(const char* formName, uint8_t* dest, uint32_t encr_size, uint8_t* p_gcm_mac) {
  std::map<std::string, form>::iterator it;
  it = forms.find((std::string) formName);
  if(it == forms.end()) {
    printf_enc("SUBMIT didnt find requested form");
    return SGX_ERROR_INVALID_PARAMETER;
  }
  form f = it->second;
  if(!f.validated) {
    printf_enc("SUBMIT form not validated");
    return SGX_ERROR_INVALID_PARAMETER;
  }

  std::string str_form = parse_form(f, true);
  uint8_t aes_gcm_iv[12] = {0};
  sgx_status_t ret = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *) &g_secret,
        (const uint8_t*) &str_form[0],
        encr_size,  
        dest,
        &aes_gcm_iv[0],
        12,
        NULL,
        0,
        (sgx_aes_gcm_128bit_tag_t *) (p_gcm_mac));

  if(ret  != SGX_SUCCESS) {
    return SGX_ERROR_INVALID_PARAMETER;
  }
  printf_enc("SUBMIT returning normally");
  return SGX_SUCCESS;
}

//useful debugging function that decrypts and encrypted form
sgx_status_t test_decryption(uint8_t* form, uint32_t form_size, uint8_t* mac) {
    uint8_t output[form_size] = {0};
    uint8_t aes_gcm_iv[12] = {0};
    sgx_status_t ret =  sgx_rijndael128GCM_decrypt((const sgx_aes_gcm_128bit_key_t *) &g_secret,
        (const uint8_t*)  &form[0],
        form_size,
        &output[0],
        &aes_gcm_iv[0],
        12,
        NULL,
        0,
        (sgx_aes_gcm_128bit_tag_t *) mac);

    printf_enc("decrypted form\n");
    for(int i = 0; i < form_size; i++) {
        //printf_enc("%c", (char)output[i]);
    }
    printf_enc("\n");
}


//native function for tinyJS that enables printing
void js_print(CScriptVar *v, void *userdata) {
    printf_enc("> %s\n", v->getParameter("text")->getString().c_str());
}


//native function updates the value in a field in a form. Note that this must be 
//called in addtion to modifying the object representing the form w/in tinyJS to keep
//the "true" form and its tinyJS representation in sync.
void js_update_form(CScriptVar *v, void *userdata) {
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
void js_make_http_request(CScriptVar *v, void* userdata) {
    std::string method = v->getParameter("method")->getString();
    std::string url = v->getParameter("url")->getString();
    std::string headers = v->getParameter("headers")->getString();
    std::string post_data = v->getParameter("postData")->getString();
    std::string request_data = method + url + headers + post_data;
    if(url != origin) {
        printf_enc("Error: invalid origin %s", url.c_str());
        return;
    }

    uint32_t len_val = (uint32_t) request_data.length();
    uint8_t encr_val[len_val] = {0};
    uint8_t aes_gcm_iv[12] = {0};
    uint8_t p_gcm_mac[16] = {0};
    sgx_status_t ret = sgx_rijndael128GCM_encrypt((const sgx_aes_gcm_128bit_key_t *) (&g_secret),
            (const uint8_t*) &request_data[0],
            len_val,  
            &encr_val[0],
            &aes_gcm_iv[0],
            12,
            NULL,
            0,
            (sgx_aes_gcm_128bit_tag_t *) (p_gcm_mac));
    if(ret != SGX_SUCCESS){
        printf_enc("Error: encryption failed");
        return;    
    }
    request_data = std::string((char *) &encr_val[0]);
    int ret_code = 0;
    enc_make_http_request(&ret, method.c_str(), url.c_str(), headers.c_str(), request_data.c_str(), &p_gcm_mac[0], &ret_code);
    if(ret != SGX_SUCCESS) {
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
void get_http_response(char* http_response, size_t response_len) {
    response = copyString(http_response, response_len);
    response_ready = true;
}

//native function that essentially busy waits until the HTTP response is ready
void js_get_http_response(CScriptVar *v, void* userdata) {
    if(response_ready == true) {
        v->getReturnVar()->setString(response);
        response_ready = false;
    } else {
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
void js_load_items(CScriptVar *v, void *userdata) {
    std::string sealed;
    size_t data_len;
    ocall_get_file_size(&data_len, (origin + ".txt").c_str());
    if(data_len > 0) {
        ocall_read_file((origin + ".txt").c_str(), &sealed[0], data_len);

        uint32_t len = sgx_get_encrypt_txt_len((const sgx_sealed_data_t*) &sealed);
        char decrypted_text[len];
        uint32_t mac_len = 0;
        sgx_status_t ret = sgx_unseal_data(
            (const sgx_sealed_data_t*) &sealed,
            NULL,
            &mac_len,
            (uint8_t*) &decrypted_text[0],
            &len 
        );

        if(ret != SGX_SUCCESS) {
            printf_enc("Error: data unsealing failed\n");
        }
        std::string decrypted = std::string(decrypted_text);
        v->getReturnVar()->setString(decrypted);
    } else {
        v->getReturnVar()->setString("{}");

    }
    return; 
}

/*
    Native function that saves to secure storage. A call to this function is automatically
    added to the end of every JS program run in the enclave.

    NOTE: Currently this is buggy.
*/
void js_save_items(CScriptVar *v, void *userdata) {
    std::string data_to_store = v->getParameter("data")->getString();
    uint32_t len = (uint32_t) data_to_store.length();
    uint32_t sealed_size = sgx_calc_sealed_data_size(0, len);
    sgx_sealed_data_t sealed_data[sealed_size]; 
    sgx_status_t ret = sgx_seal_data(
        0,
        NULL,
        len,
        (uint8_t*) &data_to_store[0],
        sealed_size,
        &sealed_data[0]
    );

    if(ret != SGX_SUCCESS) {
        printf_enc("Error: data sealing failed\n");
    }
    ocall_write_file((origin + ".txt").c_str(), (char*)&sealed_data[0], sealed_size);
    return;
}

//I don't remember what this does--it comes with the tinyJS sample script
void js_dump(CScriptVar *v, void *userdata) {
    CTinyJS *js = (CTinyJS*)userdata;
    js->root->trace(">  ");
}

/*
    Runs a string of JS in the tinyJS enviornment. See documentation on tinyJS at
    https://github.com/gfwilliams/tiny-js/tree/56a0c6d92b5ced9d8b2ade32eec5ddfdfdb49ef5

*/
sgx_status_t run_js(char* code, size_t len, const uint8_t *p_sig_code, size_t len_sig){
    
    //comment this out to avoid validation while debugging
    if(SGX_SUCCESS != validate((uint8_t*) code, (uint32_t) len, (sgx_ec256_signature_t*) p_sig_code)) {        
       return SGX_ERROR_INVALID_PARAMETER;
    }


    //parse forms and add them as objects to the start of the JS code
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
        str_forms += "var " + name + " = " + form + ";";
    }

    char tmp[len];
    memcpy(tmp, code, len);
    std::string enc_code = std::string(tmp);
    
    //also add loading/saving code
    enc_code = "var str_data = __native_js_load_items(); var local_storage_data = eval(str_data);\n" + enc_code;
    enc_code = str_forms + enc_code;  
    enc_code += "\nstr_data = JSON.stringify(local_storage_data, undefined); __native_js_save_items(str_data);";
    
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
    try {
        js->execute(enc_code);
    } catch (CScriptException *e) {
        printf_enc("ERROR: %s\n", e->text.c_str());
        return SGX_ERROR_UNEXPECTED;
    }
    res = js->evaluate("result"); //note: result is just a variable defined in the code
    memcpy(code, res.c_str(), res.length()+1);
  delete js;
  return SGX_SUCCESS;
}

//START OF KEYBOARD STUFF
sgx_status_t get_keyboard_chars(uint8_t *p_src){
    sgx_thread_mutex_lock(curInputMutex);
    
    if(ENCLAVE_STATE == FOCUS_OFF) {
        printf_enc("No input in focus.");
        sgx_thread_mutex_unlock(curInputMutex);
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
    //enclave_times << "key decrypt => form render," << timeNow() << ",";
    
    status = gcm_decrypt(&ciphertext[0],1,&p_char[0], &iv[0], &tag);
    if (p_char[0] == 0x7A){//letter z
        
        p_char[0] = -1; //basically doing nothing
    }
    else if(p_char[0] == 0x7f) {
        if (curInput.value.length() != 0) {
            curInput.value = curInput.value.substr(0, curInput.value.length()-1);
        }
    }
    else{
        printf_enc("DETECTED KEY PRESS: %d", p_char[0]);
        curInput.value += p_char[0];
    }
    printf_enc("KEYBOARD: Form name = %s Input name = %s, value = %s",curForm.name.c_str(), curInput.name.c_str(),  curInput.value.c_str());
    //printf_enc("KEYBOARD: Input Field X = %d", curInput.x);
    //printf_enc("KEYBOARD: Input Field Y = %d", curInput.y);
    //printf_enc("KEYBOARD: Input Field Width = %d", curInput.width);
    //printf_enc("KEYBOARD: Input Field Height= %d", curInput.height);
    //printf_enc("KEYBOARD: Char obtained: %x", p_char[0] );    
    //printf_enc("KEYBOARD: new value for input = %s", curInput.value.c_str());


    std::string fname = "loginform";
    std::string iname = "username";
    printf_enc("BEFORE ADDKEY data for loginform: username: %s", forms[fname].inputs[iname].value.c_str());
    forms[curForm.name].inputs[curInput.name] = curInput;
    printf_enc("AFTER ADDKEY data for loginform: username: %s", forms[fname].inputs[iname].value.c_str());

    sgx_thread_mutex_unlock(curInputMutex);
    return status;
}

sgx_status_t gcm_decrypt(uint8_t *p_src, uint32_t src_len, uint8_t *p_dst, uint8_t *p_iv,  sgx_aes_gcm_128bit_tag_t *p_in_mac){
    sgx_status_t status;
    //printf_enc("Executing gcm_decrypt function from enclave...");
    const sgx_aes_gcm_128bit_key_t p_key = {
         0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
         0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
        };
    
    status = sgx_rijndael128GCM_decrypt(&p_key,p_src, src_len, p_dst, p_iv, 12, NULL, 0, p_in_mac);
    for (int i=0; i<src_len; i++){
        //printf_enc("Decrypted Characters(in Enclave):%x", p_dst[i]);
    }
    
    //printf_enc("Status_decrypt: %x\n", status);
    return status;
}


