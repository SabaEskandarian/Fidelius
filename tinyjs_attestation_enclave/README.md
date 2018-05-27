**manifest.JSON**: master file viewed by the chrome browser. Includes the title,
icon, background scripts/pages, contents scripts, etc.

**background.js**: waits for the browserAction event, which is when the user clicks
on the Chrome extension

**htmlParser.js**: logic that performs the operations of the chrome extension. Called in background.js

**syntaxChecker.js**: Ensures a web page’s scripts and forms are suitable for use with the Enclave by checking for signature and secure tags. Called in background.js

**EnclaveManager**: C++ program that waits to receive messages from the browser extension. 
Messages arrive in the following format: first 4 bytes are the message length,
the subsequent bytes form arguments delimited by '\n'. The first argument is a
 number corresponding to the name of the command to be called. 

**Installation/Usage**
Clone this repository into home/.config/google-chrome/NativeMessagingHosts/ (The .config folder may be hidden. Press cltr+h to reveal hidden folders when viewing files on Linux). 

Open the terminal within home/.config/google-chrome/NativeMessagingHosts/sgx-browser, make EnclaveManager.cpp as outlined in make.sh. Type “chmod +x EnclaveManager” in order to enable this program. 

Delete debug_log.txt for now. This is the file that should be produced after running the extension and EnclaveManager

Move "com.google.chrome.example.echo" out of the sgx-browser folder and up one level; its path should now be home/.config/google-chrome/NativeMessagingHosts/com.google.chrome.example.echo.

Load the extension by opening home/.config/google-chrome/NativeMessagingHosts/sgx-browser when loading an extension in Chrome

Go to home/.config/google-chrome/NativeMessagingHosts/examples and run index_sgx.html.

Click on the extension and go to home/.config/google-chrome/NativeMessagingHosts/sgx-browser. You should now have debug_log.txt again with the scripts and forms of index_sgx.html

