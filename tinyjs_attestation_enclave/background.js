var nativePort = chrome.runtime.connectNative("com.google.chrome.fidelius.echo");
var numEMs = 0;
//nativePort.postMessage("2" + '\n' + "origin" + '\n');
chrome.tabs.onUpdated.addListener(function(tabId, changeInfo, tab) {
  nativePort.postMessage("6" + '\n' + "origin" + '\n');
  if (changeInfo.status == "complete" && tab.active) {
    chrome.tabs.executeScript(null, {file: "syntaxChecker.js"});

    chrome.runtime.onMessage.addListener(function(msg) {
      
      nativePort.postMessage("6" + '\n' + "origin" + '\n');
      if (msg == "false") { 
        return;
      } else if (numEMs >= 1) {
        //alert('we cannot handle concurrent web enclaves');
      }
      else {
        start();
        numEMs++;
      }
    });
  }
});

function start() {
  /* Wait for message from HTMLParser. When a message is received, relay it to the
   * enclave manager via NativeMessaging
   */
      chrome.runtime.onConnect.addListener(function(parserPort) {
  	  parserPort.onMessage.addListener(function(msg) {
    	  console.log(msg);
          nativePort.postMessage(msg);
        });

        nativePort.onMessage.addListener(function(msg) {
	  //alert("hi2");
      	  console.log(msg);
      	  parserPort.postMessage(JSON.stringify(msg));
        });

        nativePort.onDisconnect.addListener(function() {
        	parserPort.postMessage("Disconnected");
        });
      });

      //chrome.tabs.executeScript(null, {file: "signature.js"});
      chrome.tabs.executeScript(null, {file: "htmlParser.js"});
}
