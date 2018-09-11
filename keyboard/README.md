# Organization

Everything related to the Pi is in the "slave/" directory. Files relevant to the linux host and enclave are in the "host/" directory.

There are READMEs in each of these directories that explain usage and installation. 

# Relevant files

The main files used by the SGX application are:

slave/cpp/readkeys.cpp
slave/cpp/hid_translate.h
slave/cpp/aes_encrypt.h
slave/cpp/keycodes.h

host/keyboard_driver.h
host/keyboard_driver.cpp
host/keyboard_driver_t.cpp