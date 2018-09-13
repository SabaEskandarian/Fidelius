## Fidelius Chrome Extension:

These are the files of the chrome extension:
 * **manifest.JSON**: master file viewed by the chrome browser. Includes the title, icon, background scripts/pages, contents scripts, etc.
 * **background.js**: waits for the browserAction event, which is when the user clicks on the Chrome extension
 * **htmlParser.js**: logic that performs the operations of the chrome extension. Called in background.js
 * **syntaxChecker.js**: Ensures a web page’s scripts and forms are suitable for use with the Enclave by checking for signature and secure tags. Called in background.js

## Fidelius Enclave Manager (EM):

The Enclave Manager is a native program that manages the enclave and it communicates with the Chrome extension via the Chrome Native Messaging API (See: https://developer.chrome.com/extensions/nativeMessaging).

Once up and running, EM waits to receive messages from the browser extension. Messages arrive in the following format: 

* first 4 bytes are the message length;
* the subsequent bytes form arguments delimited by '\n'. The first argument is a number corresponding to the name of the command to be called. 

## Installation/Usage

0. Make sure you installed SGX driver, PSW, and SDK.  

1. First, install the bluetooth library for communicating with the secure display with:
```
$ sudo apt install libbluetooth-dev
```

2. Clone this repository into `~/.config/google-chrome/NativeMessagingHosts/` (The `.config` folder may be hidden. Press cltr+h to reveal hidden folders when viewing files on Linux). 

3. Open the terminal within ~/.config/google-chrome/NativeMessagingHosts/sgx-browser, make EnclaveManager.cpp as outlined in make.sh. 

4. Give +x mode to the EnclaveManager executable:
```
$ chmod +x EnclaveManager
```

5. Delete debug_log.txt for now. This is the file that should be produced after running the extension and EnclaveManager.

6. Move `com.google.chrome.example.echo` out of the sgx-browser folder and up one level; its path should now be `~/.config/google-chrome/NativeMessagingHosts/com.google.chrome.example.echo`.

7. Load the extension by opening `~/.config/google-chrome/NativeMessagingHosts/sgx-browser` when loading an extension in Chrome

8. Go to `~/.config/google-chrome/NativeMessagingHosts/examples` and open `index_sgx.html`.

9. Click on the extension and go to `~/.config/google-chrome/NativeMessagingHosts/sgx-browser`. You should now have `debug_log.txt` again with the scripts and forms of `index_sgx.html`

## Bluetooth Pairing Issues

Due to some complications with Bluetooth pairing, the first time this is run with a new hardware, a few extra steps must be taken.

Inside `Fidelius/tinyjs_attestation_enclave/isv_app/sgx_display/btchannel.cpp`, `BT_SCAN` must be set to `1` and recompiled. Additionally, the Raspberry Pi Bluetooth must be discoverable, this is accessible by clicking on the Bluetooth icon in the top right of the screen. If this is done correctly the host computer will scan discoverable Bluetooth devices and connect to a device with the name "raspberrypi". If using a new Raspberry Pi, the Bluetooth address must be updated underneath the `BT_SCAN` definition, the Bluetooth address should get printed in the debug log when scanning. For future uses `BT_SCAN` can be set back to `0` and making the Raspberry Pi discoverable is not necessary.

## How to Build/Execute SGX Code

1. Install Intel(R) SGX SDK for Linux* OS

2. Build the project with the prepared Makefile:

    a. Hardware Mode, Debug build:

    ```$ make```
    
    b. Hardware Mode, Pre-release build:
    
    ```$ make SGX_PRERELEASE=1 SGX_DEBUG=0```
    
    
    c. Simulation Mode, Debug build:
    
    ```$ make SGX_MODE=SIM```
    
    d. Simulation Mode, Pre-release build:
    
    ```$ make SGX_MODE=SIM SGX_PRERELEASE=1 SGX_DEBUG=0```

3. To execute the binary directly: (this should not apply for normal use of Fidelius)

    ```$ ./app```

4. Remember to "`make clean`" before switching build mode

