//g++ openssl.cpp -lcrypto
#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

/* AES-GCM test data from NIST public test vectors */
int handleErrors(){
	return -1;
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,
	int aad_len, unsigned char *key, unsigned char *iv, int iv_len,
	unsigned char *ciphertext, unsigned char *tag)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int ciphertext_len;


	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

	/* Initialise the encryption operation. */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length if default 12 bytes (96 bits) is not appropriate */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

	/* Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	//if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
		//handleErrors();

	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();
	ciphertext_len = len;

	/* Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
	ciphertext_len += len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors();

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

int main(){
	unsigned char plaintext[] ={'a'};
	int plaintext_len = 1;
	unsigned char *aad;
	int aad_len = 0;
	unsigned char key[] = {
	0x24, 0xa3, 0xe5, 0xad, 0x48, 0xa7, 0xa6, 0xb1,
	0x98, 0xfe, 0x35, 0xfb, 0xe1, 0x6c, 0x66, 0x85
	};
	unsigned char iv[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	int iv_len = 12;
	unsigned char ciphertext[1];
	unsigned char tag[16];
	int test = encrypt(plaintext,plaintext_len,aad,aad_len,&key[0],&iv[0],iv_len,&ciphertext[0],&tag[0]);
	printf("Ciphertext: %x\n", ciphertext[0]);
	for (int i = 0; i < 16; i ++){
		printf("Tag: %x\n", tag[i]);
	}
	
	return -1;
}
