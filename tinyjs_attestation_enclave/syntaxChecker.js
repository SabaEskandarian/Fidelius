var useEnclave = true;

//Get all script tags
var scriptArray = document.getElementsByTagName("script");
var scriptString = "";

//Get all form tags
var formArray = document.getElementsByTagName("form");
var formString = "";

//Iterate through all script tags and check syntax for our system
function parseScriptTags() {
	for (var i = 0; i < scriptArray.length; i++) {
		if (!scriptArray[i].hasAttribute("sign")) {
      console.error("The following script doesn't have a signature: " + scriptArray[i]);
      useEnclave = false;
    }
    if (!scriptArray[i].hasAttribute("secure")) {
      console.error("The following script doesn't have the 'secure' attribute: " + scriptArray[i]);
      useEnclave = false;
    }
	}
}

function inspectInputs(form) {
  for (var i = 0; i < form.elements.length; i++) {
    if (form.elements[i].nodeName == "INPUT") {
      var acceptedTypes = ["text", "number", "tel", "email", "password", "url"];
      var type = form.elements[i].getAttribute("type");
      if (acceptedTypes.indexOf(type) == -1) continue;
    } else if (form.elements[i].nodeName != "TEXTAREA") continue;

    input = form.elements[i];

    if (!input.hasAttribute("secure")) {
      console.error("The following input doesn't have the 'secure' attribute: " + input.name);
      useEnclave = false;
    }
    if (!input.hasAttribute("secureName")) {
      console.error("The following input doesn't have a secureName: " + input.name);
      useEnclave = false;
    }
  }
}

//Iterate through all form tags and check syntax for our system
function parseFormTags() {
	for (var i = 0; i < formArray.length; i++) {
		var form = formArray[i];
    if (!form.hasAttribute("secure")) {
      console.error("The following form doesn't have the 'secure' attribute: " + form.formName);
      useEnclave = false;
    }
    if (!form.hasAttribute("sign")) {
      console.error("The following form doesn't have a signature: " + form.name);
      useEnclave = false;
    }
    inspectInputs(form);
	}
}

function main() {
    if (document.getElementsByTagName("head")[0].hasAttribute("secure")) {
      chrome.runtime.sendMessage("true");
      console.log("secure");
    } 
    else { 
      chrome.runtime.sendMessage("false");
      console.log("insecure");
    }

  /*parseScriptTags();
  parseFormTags();
  if (!useEnclave) throw "Incorrect secure tag setup. Enclave will not be used.";*/
}

main();
