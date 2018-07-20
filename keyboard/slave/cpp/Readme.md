# CPP dependencies

Encryption "aes_encrypt.h" uses OpenSSL. See https://www.openssl.org/ 

"readkeys.cpp" is the main application used by the pi to send characters to the linux host.
It uses "ncurses". To install use:

`sudo apt-get install libncurses5-dev libncursesw5-dev`

Make the readkeys.cpp by running: `make`

Run using: 

~~~~
sudo -i
# navigate to readkeys exeuctable using cd
./readkeys 

~~~~

USAGE: Characters typed on readkeys will be sent to the enclave on the host. 
       The host/keyboard_driver.h listens on the serial ports for encrypted inputs
       This will be installed on the enclave in a different directory       



TODO:
      Increment IV: The IV needs to be incremented more securely than it is now. Currently only the last byte is incremented so there are repeated IVs. 

      Change Mode: Mode needs to change between secure mode and non-secure mode based on OnBlur and OnFocus events coming in from the enclave. readkeys.cpp needs to read from ttyGS1 for changes in mode. Needs to incude an LED to indicate the mode that the Pi is currently in. 

      Regular Keyboard: Only supports lowercase alphabetic characters in Non-secure mode. No delete and special characters supported. See "keycodes.h"
      

# DESCRIPTION OF ENCRYPTION/DECRYPTION PROCESS: 

ENCRYPTION OF KEYSTROKES ON PI:

For every single keystroke that a user sends to the Raspberry Pi via SSH, it will be encrypted using the Rijndael AES-GCM encryption operation with a 128 bit key size. On the Pi side, this is achieved using the open-source cryptography library OpenSSL. Authenticated encryption using GCM mode via the OpenSSL EVP interface supports the encryption of the keystrokes. The keystrokes are read by the Pi as ASCII encoded characters. Each keystroke is encrypted individually and sent to the application to be processed. The Pi and the enclave are assumed to be sharing the same symmetric secret key. 
The encrypted output is as follows. The 12-bytes IV will be incremented by 1 for every keystroke that has been encrypted. A 1-byte ciphertext is produced for each encrypted keystroke. A 16-byte tag is also produced which will be used to verify the integrity of the ciphertext sent. The IV, ciphertext and the tag will then be appended together by the Pi and sent to the host application. The data that is sent is a string containing the hex representation of the encryption output. 
When the user does not type any character, a pre-determined filler character (such as the symbol ‘ ~ ‘ ) is encrypted and sent to the application at a certain frequency (which is ideally the typing speed of an average human). This is done to prevent timing attacks on the user’s input. Only the Enclave with the correct secret key will be able to differentiate between a filler character and an actual user’s character input by decrypting the ciphertext and verifying its integrity.

DECRYPTION OF KEYSTROKES ON THE INTEL SGX ENCLAVE:

After the 1-byte ciphertext, 12-bytes IV and 16-bytes tag have been received by the application via the Pi which acts as a USB gadget, the application calls an ECALL function that receives and parses the string containing the hex representation of the encrypted keystroke. A Rijndael AES-GCM decryption operation is used for decryption and only a 128-bit key size is supported by the Intel SGX SDK cryptography library. Only the enclave that has the correct symmetric secret key will be able to decrypt the character and process it accordingly (by determining whether it is a filler character or an actual user’s input). This also means that for each ECALL made, the enclave will decrypt and receive 1 keystroke. 
Since there are filler characters (such as the symbol ‘ ~ ‘ ), the filler characters will simply be discarded and ignored by the enclave. Only the actual user’s input will be used in the display. Untrusted code or applications outside the trusted enclave will not be able to decrypt any of the keystrokes. Furthermore, as mentioned previously, the Pi and the trusted enclave are assumed to be sharing the same symmetric secret key. 

