var nativePort = chrome.runtime.connectNative("com.google.chrome.fidelius.echo");
chrome.browserAction.onClicked.addListener(function(tab) {

/* Wait for message from HTMLParser. When a message is received, relay it to the
 * enclave manager via NativeMessaging
 */
    chrome.runtime.onConnect.addListener(function(parserPort) {
	  parserPort.onMessage.addListener(function(msg) {
  	    console.log(msg);
        parserPort.postMessage(msg);
        nativePort.postMessage(msg);
      });
	});

    nativePort.onMessage.addListener(function(nativePort) {
    	parserPort.postMessage(msg);
    });
    //chrome.tabs.executeScript(null, {file: "signature.js"});
    //chrome.tabs.executeScript(null, {file: "syntaxChecker.js"});
    chrome.tabs.executeScript(null, {file: "htmlParser.js"});

});
