#include <stdio.h>
#include <iostream>
#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"

#include "keyboard_driver.h"
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>


uint8_t convert(char *target){
	uint8_t number = (int)strtol(target, NULL, 16);
	return number;
}

void hexStrtoBytes(char *char_stream, int stream_len, uint8_t *byte_array){
	
	
	for (int i =0; i < stream_len; i = i +2){
		char array[2];
		array[0] = *(char_stream+i);
		array[1] = *(char_stream+i+1);
		byte_array[i/2] = convert(&array[0]);
	}
}


/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

KeyboardDriver global_KB("/dev/ttyACM0", "/dev/ttyASM1");

// OCall implementations
void ocall_print(const char* str) {
    printf("%s\n", str);
}


void ocall_getKeyBoardInput(uint8_t *kb, uint8_t *p_src,  uint8_t *tag, uint8_t *p_iv){

  KeyboardDriver * kb_p = (KeyboardDriver *) kb;
  char input_stream[58];
  uint8_t char_stream[29];  
  //kb_p->getEncryptedKeyboardInput(&input_stream[0], 58, false);
  //function to get characters and store them in the respective buffers
  //char_stream is hardcoded for now
  
  // uint8_t char_stream[] = {
  //   0xe9, 0x13, 0x69, 0xb4, 0xce, 0x90, 0xd9, 0x62, 
  //   0x88, 0x28, 0x54, 0xd2, 0xf1, 0xf8, 0xe9, 0x0c, 
  //   0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //   0x00, 0x00, 0x00, 0x00, 0x00
  // };
  /*
   char char_stream[] = {
     0xe0, 0x61, 0xa8, 0xef, 0xd3, 0xd4, 0x6b, 0xc3, 
     0x7f, 0xc1, 0x72, 0x2e, 0xb0, 0x50, 0xdd, 0xa9, 
     0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00
   };
   */
   //char * str ="e061a8efd3d46bc37fc1722eb050dda945000000000000000000000000";
   hexStrtoBytes(&input_stream[0], 58, &char_stream[0]);

  printf("Received from Serial: %s", char_stream);
  
  p_src[0] = char_stream[0];	
  
  for (int i = 1; i<17; i++){
    tag[i-1] = char_stream[i];
  }
  for (int i = 17; i<29; i++){
    p_iv[i-17] = char_stream[i];  
  }
  printf("%s\n", "hello");
}


void ocall_getPointerToKb(uint8_t **kb){
  *kb = (uint8_t*) &global_KB;
}

int main(int argc, char const *argv[]) {
    if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0) {
        std::cout << "Fail to initialize enclave." << std::endl;
        return 1;
    }
    int ptr;
    int one = 1;

    // ECALL
    sgx_status_t status = generate_random_number(global_eid, &ptr, one);

    
    std::cout << status << std::endl;
    if (status != SGX_SUCCESS) {
        std::cout << "oh no" << std::endl;
    }
    printf("Random number output from generate_random_number function: %d\n\n", ptr);
    
    //hardcoded(global_eid);
    

    
    /* Code that will be continuously ran to decrypt stuff */
    int ptr2; //returns the status of the decryption function
    
    /* p_src is ciphertext, p_iv is the nonce, tag is the tag, all are in hex*/
    /* change these parameters accordingly*/
    
    // 1st example : "haha"
    //   uint8_t p_src[] = {
    //   0xe9, 0x90, 0x69, 0x5e
    // };
    // uint8_t p_iv[12] = {0};
    // uint8_t tag[] = {
    //   0x98, 0xea, 0xfe, 0x56, 0xe2, 0x1c, 0xf7, 0xd3, 0xd6, 0x9b, 0x68, 0x4c, 
    //   0x80, 0x91, 0x70, 0x1a
    // };
	
    // uint8_t src_len = sizeof(p_src) / sizeof(p_src[0]);
    // uint8_t p_dst[src_len];
	
    
    // 2nd example : "HolyMoly"
    //   uint8_t p_src[] = {
    //   0xc9, 0x9e, 0x6d, 0x46, 0xe6, 0x11, 0xfc, 0x13
    // };
    // uint8_t p_iv[12] = {0};
    // uint8_t tag[] = {
    //   0x4a, 0x2e, 0xda, 0x0d, 0x67, 0x94, 0x35, 0x71, 0xab, 0x27, 0x54, 0xb5, 
    //   0x7e, 0xcc, 0xc0, 0xde
    // };
    
    // uint8_t src_len = sizeof(p_src) / sizeof(p_src[0]);
    // uint8_t p_dst[1000];
    	
    
    /* 3rd example : "What is happening?" */	
    uint8_t p_src[] = {
      0xd6, 0x99, 0x60, 0x4b, 0x8b, 0x17, 0xe3, 0x4a,
      0x48, 0xba, 0x17, 0xc7, 0x4c, 0xc6, 0xb4, 0x4f, 
      0x99, 0x28
    };
    uint8_t p_iv[12] = {0};
    uint8_t tag[] = {
      0xa6, 0x14, 0x07, 0x67, 0xff, 0x97, 0x36, 0x33, 0x9e, 0x3d, 0x09, 0x22, 
      0xea, 0xec, 0x09, 0xed
    };
    
    uint8_t src_len = sizeof(p_src) / sizeof(p_src[0]);
    uint8_t p_dst[1000];
    
    // ECALL
    gcm_decrypt(global_eid, p_src, src_len, p_dst, p_iv, &tag);
	
    // for (int i=0; i<src_len; i++){
    // printf("Decrypted Characters (outside Enclave):%x\n", p_dst[i]);
    // }
    
	
    ///////////////////////////////////////////////////////////////////////
    //change_mode(global_eid);

    // ECALL
    nextchar_decrypted(global_eid);

	

    return 0;
}
