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

      Change Mode: Mode needs to change between secure mode and non-secure mode based on OnBlur and OnFocus events coming in from the enclave. readkeys.cpp needs to read from ttyGS1 for changes in mode. 

      Regular Keyboard: Only supports lowercase alphabetic characters in Non-secure mode. No delete and special characters supported. See "keycodes.h"