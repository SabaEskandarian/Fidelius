#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "sgx_urts.h"

#include "Enclave_t.h"
#include "sgx_tcrypto.h"
#include "Enclave.h"

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print(buf);
}

/* 
 Define ECALLs here
 */
 
int generate_random_number(int one) {
	ocall_print("Executing generate_random_number function from enclave...");
    ocall_print("Processing random number generation...");
    return one;
}

void hardcoded(void){
	printf("Executing hardcoded function from enclave...");
	const sgx_aes_gcm_128bit_key_t p_key = {

		 0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
		 0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85

		};

    uint8_t p_src2[] = {"haha"};

    printf("Data to encrypt: %d", p_src2[0]);
    uint32_t src_len2 = 4;
    uint8_t p_dst2[20]; //initialize a buffer
    uint8_t p_iv2[12] = {0}; //96 bytes
    uint32_t iv_len2 = 12;
    uint8_t *p_aad2 = NULL;
    uint32_t aad_len2 = 0;
    uint8_t p_out_mac2[16];

    sgx_status_t status2;

	status2 = sgx_rijndael128GCM_encrypt(&p_key,p_src2,src_len2,&p_dst2[0], &p_iv2[0], iv_len2, p_aad2, aad_len2, &p_out_mac2);
  
    
	for (int i=0; i<sizeof(p_dst2)/sizeof(p_dst2[0]); i++){
		printf("Cipher:%x", p_dst2[i]);
	}
	
	for (int i=0; i<sizeof(p_out_mac2)/sizeof(p_out_mac2[0]); i++){
		printf("Tag:%x", p_out_mac2[i]);
	}
    printf("Status_Encrypt:%x", status2);
       
       /*
	sgx_aes_gcm_128bit_key_t p_out_mac3 = {
		 0x61, 0x74, 0x00, 0xf6, 0xa8, 0x59, 0xde, 0x0e, 0xfb, 0xb2, 0x61, 0xec, 
		 0xb6, 0x12, 0xa4, 0x4d
	};
*/


    uint32_t decrypted_data_length = src_len2;
    uint8_t decrypted_data[2];


    sgx_status_t status;
    
	status = sgx_rijndael128GCM_decrypt(&p_key,p_dst2,decrypted_data_length, &decrypted_data[0], &p_iv2[0], iv_len2, p_aad2, aad_len2, &p_out_mac2);
    
    printf("%s", "Hello everyone! Decryption is taking place");

    printf("Status_decrypt: %x", status);
	printf("Decrypted Data: %d\n", decrypted_data[0]);

}

void gcm_decrypt(uint8_t *p_src, uint32_t src_len, uint8_t *p_dst, uint8_t *p_iv,  sgx_aes_gcm_128bit_tag_t *p_in_mac){
	sgx_status_t status;
	printf("Executing gcm_decrypt function from enclave...");
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
	status = sgx_rijndael128GCM_decrypt(&p_key,p_src, src_len, p_dst, p_iv, 12, NULL, 0, p_in_mac);
	for (int i=0; i<src_len; i++){
		printf("Decrypted Characters(in Enclave):%x", p_dst[i]);
	}
	
	printf("Status_decrypt: %x\n", status);
	//return status;
}

void change_mode(){
		printf("Testing change mode from enclave...");
		const sgx_aes_gcm_128bit_key_t p_key = {
		 0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
		 0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
		};
		
		uint8_t p_src[] = {"secure"};

		printf("Data to encrypt: %d", p_src[0]);
		uint32_t src_len = sizeof(p_src) / sizeof(p_src[0]);
		uint8_t p_dst[20]; //initialize a buffer
		uint8_t p_iv[12] = {0}; //96 bytes
		uint8_t p_out_mac[16];

		sgx_status_t status;

		status = sgx_rijndael128GCM_encrypt(&p_key,p_src,src_len,&p_dst[0], &p_iv[0], 12, NULL, 0, &p_out_mac);
		printf("Change_mode_message_encrypt_status: %x\n", status);
		
}

void nextchar_decrypted(){
	//call function to get nextchar
	uint8_t ciphertext[1];
	uint8_t iv[12];
	uint8_t tag[16];

	// get keyboarddriver pointer from the untrusted code. To be passed to the next ocall
        uint8_t * kb;
	ocall_getPointerToKb(&kb);
	ocall_getKeyBoardInput(kb, &ciphertext[0], &tag[0], &iv[0] ); //invokes ocall to get keyboard input

/*
	printf("%x", ciphertext[0]);	

	for (int i = 0; i<12; i++){
		printf("%x", iv[i] );	
	}
	
	for (int i = 0; i<16; i++){
		printf("%x", tag[i] );	
	}
*/	
	uint8_t p_char[1]; //initialize a buffer
	gcm_decrypt(&ciphertext[0],1,&p_char[0], &iv[0], &tag);
	if (p_char[0] == 0x7A){//letter z
			p_char[0] = -1; 
	}
	printf("Char obtained: %x", p_char[0] );	

}

void nextchar_encrypted(){
	//call function to get nextchar
	uint8_t ciphertext[1];
	uint8_t iv[12];
	uint8_t tag[16];

	// get keyboarddriver pointer from the untrusted code. To be passed to the next ocall
        uint8_t * kb;
	ocall_getPointerToKb(&kb);
	ocall_getKeyBoardInput(kb, &ciphertext[0], &tag[0], &iv[0] ); //invokes ocall to get keyboard input
/*
	printf("%x", ciphertext[0]);	

	for (int i = 0; i<12; i++){
		printf("%x", iv[i] );	
	}
	
	for (int i = 0; i<16; i++){
		printf("%x", tag[i] );	
	}
*/	
	uint8_t e_char[29]; //initialize a buffer
	e_char[0] = ciphertext[0];

	for (int i = 0; i<16; i++){
		e_char[i+1] = tag[i];
	}
	
	for (int i = 0; i<12; i++){
		e_char[i+17] = iv[i];
	}
	for (int i = 0; i<29 ; i++){
		printf(" obtained: %x", e_char[i] );	
	}
}
