const ON_BLUR = 0;
const ON_FOCUS = 1;
const ADD_FORM = 3;
const ADD_SCRIPT = 4;

var useEnclave = false;

//Get all script tags
var scriptArray = document.getElementsByTagName("script");
var scriptString = "";

//Get all form tags
var formArray = document.getElementsByTagName("form");
var formString = "";

var signature = null;

//Given an input, creates a text file of the input and link for that file
var textFile = null;

var JSport = null;
var formPort = null;

var currentFocus = null;

/* Retrieves the secureName attribute of the given input
 */
function getInputName(input) {
	if (input.nodeName == "INPUT") {
		if (input.hasAttribute("name")) return input.getAttribute("name");
	};
}

/* Repeatedly investigates parent tags until the form tag is found, then returns the name
 */
function getFormName(input) {
	while (true) {
		if (input.parentNode.nodeName == "FORM") return input.parentNode.name;
		if (input.parentNode.nodeName == "BODY") return null;
		input = input.parentNode;
	}
}

/* Gets the name of the focused input, the form name, and the xy coordinates of the input.
 * The information is stored as an object and sent to the background script,
 * which is in turn sent to the enclave manager.
 */
function onFocus() {
	//alert("focus detected");
	if (!useEnclave) return;
	currentFocus = document.activeElement;
	if (currentFocus.nodeName == "INPUT") {
		var formName = getFormName(currentFocus);
		var inputName = getInputName(currentFocus);
		var bodyRect = document.body.getBoundingClientRect(),
    	elemRect = currentFocus.getBoundingClientRect(),
    	yCoord = elemRect.top - bodyRect.top;
    	xCoord = elemRect.left - bodyRect.left;
    	var height = currentFocus.clientHeight;
    	var width = currentFocus.clientWidth;
		//var focusInfo = {formName, inputName, xCoord, yCoord};
		//formPort.postMessage("The following element has just been focused on:\n" + JSON.stringify(focusInfo));
		//console.log("The following element has just been focused on:");
		//console.log(focusInfo);
		var m = "" + ON_FOCUS + '\n' + formName + '\n' + inputName + '\n' + xCoord + '\n' + yCoord + '\n' + height + '\n' + width + '\n';
		formPort.postMessage(m);

	}
}

/* Gets the name of the unfocused input, the form name, and the xy coordinates of the input.
 * An unfocused input is one that was focused on, but is no longer due to being clicked off of.
 * The information is stored as an objected currently and sent to the background script,
 * which is in turn sent to the enclave manager.
 */
function onFocusOut() {
	if (!useEnclave) return;
	if (currentFocus.nodeName == "INPUT") {
		var formName = getFormName(currentFocus);
		var inputName = getInputName(currentFocus);
		var bodyRect = document.body.getBoundingClientRect(),
    	elemRect = currentFocus.getBoundingClientRect(),
    	yCoord = elemRect.top - bodyRect.top;
    	xCoord = elemRect.left - bodyRect.left;
		var focusInfo = {formName, inputName, xCoord, yCoord};
		currentFocus = null;
		//formPort.postMessage("The following element has just been focused on:\n" + JSON.stringify(focusInfo));
		//console.log("The following element has just been unfocused:");
		//console.log(focusInfo);
		var m = "" + ON_BLUR + '\n' + formName + '\n' + inputName + '\n' + xCoord + '\n' + yCoord + '\n';
		formPort.postMessage(m);
	}
}

/* Sets up listeners for the "focus" and "focusout" input events.
 */
function listen(form) {
	var elements = form.elements;
	for (var i = 0; i < elements.length; i++) {
		if (elements[i].tagName == "INPUT") {
			elements[i].addEventListener("focus", onFocus);
			elements[i].addEventListener("focusout", onFocusOut);
		}
	}
}

/* Checks whether a form contains the "secure" and "sign" attributes, then marks
 * a boolean to use the enclave if both of these things are true.
 */
function checkSecureForm(form) {
	if (form.hasAttribute("sign")) {
		signature = form.getAttribute("sign");
	}
	if (form.hasAttribute("secure")) {
		useEnclave = true;
		alert("SECURE FORM" + "\nSignature: " + signature);
	}
}

/* Creates a file blob of parsed HTML material. This was used for debugging.
 */
function makeTextFile(text) {
	var data = new Blob([text], {type: "text/plain"});

	// If we are replacing a previously generated file we need to
	// manually revoke the object URL to avoid memory leaks.
	if (textFile !== null) {
	  window.URL.revokeObjectURL(textFile);
	}

	textFile = window.URL.createObjectURL(data);

	return textFile;
}

function extractJS(filename) {
	var xhttp = createRequest("GET", filename);
	xhttp.onload = function() {
		var responseText = xhttp.responseText;
		scriptString += responseText;
	};

	xhttp.onerror = function() {
		console.log("There was an error!");
	};

	xhttp.send();
}

//Iterate through all script tags, parse them, and write their contents to a string
function parseScriptTags() {
	for (var i = 0; i < scriptArray.length; i++) {
		if (scriptArray[i].hasAttribute("sign")) {
			signature = scriptArray[i].getAttribute("sign");

			scriptString += (ADD_SCRIPT + "\n");
			scriptString += signature;
			scriptString += "\n#EOF#\n";

			if (scriptArray[i].hasAttribute("src")) {
				extractJS(scriptArray[i].src);
			} else {
				scriptString += scriptArray[i].innerHTML;
			}

			scriptString += "\n#EOF#\n";
		} else {
			throw "This JavaScript has an invalid signature";
		}
	}
}

function getInputCoords(form, input) {
    elemRect = input.getBoundingClientRect();
    formRect = form.getBoundingClientRect();

	yCoord = elemRect.top - formRect.top;
	xCoord = elemRect.left - formRect.left;

	var height = input.clientHeight;
	var width = input.clientWidth;

	formString += width + "\n";
	formString += height + "\n";
	formString += xCoord + "\n";
	formString += yCoord + "\n";
}

function getInputs(form) {
  for (var i = 0; i < form.elements.length; i++) {
    if (form.elements[i].nodeName == "INPUT") {
      var acceptedTypes = ["text", "number", "tel", "email", "password", "url"];
      var type = form.elements[i].getAttribute("type");
      if (acceptedTypes.indexOf(type) == -1) continue;
    } else if (form.elements[i].nodeName != "TEXTAREA") continue;

    input = form.elements[i];

    formString += input.name + "\n";
    getInputCoords(form, input);

  }
}

//Iterate through all form tags, parse them, and write their contents to a string
function parseFormTags() {
	for (var i = 0; i < formArray.length; i++) {
		var form = formArray[i];
		formString += ADD_FORM.toString() + "\n";
		formString += form.name + "\n";
		formString += form.getAttribute("sign") + "\n";
		formString += location.origin + "\n";
		formRect = form.getBoundingClientRect();

		formX = window.screenX + formRect.left;
		formY = window.screenY + formRect.top + window.outerHeight - window.innerHeight;

		formString += formX + "\n";
		formString += formY + "\n";
		getInputs(form);
		formPort.postMessage(formString);
		formString = "";
		listen(form);

		if (!useEnclave) checkSecureForm(form);
	}
	if (!useEnclave) alert("Insecure form.");
}

/*
 *
 */
function createRequest(method, url) {
  var xhttp = new XMLHttpRequest();
  if ("withCredentials" in xhttp) {
    // "withCredentials" only exists on XMLHTTPRequest2 objects.
    xhttp.open(method, url, false);

  } else if (typeof XDomainRequest != "undefined") {

    //For IE
    xhttp = new XDomainRequest();
    xhttp.open(method, url);

  } else {

    //Not supported by browser
    xhttp = null;

  }
  return xhttp;
}

function initializeMessaging() {
	JSport = chrome.runtime.connect({name: "JavaScript"});
	JSport.onMessage.addListener(function(msg) {
  		console.log("received JS: \n" + msg.valueOf());
	});
	formPort = chrome.runtime.connect({name: "Form"});
	formPort.onMessage.addListener(function(msg) {
			console.log("received Form: \n" + msg.valueOf());
	});
}

function main() {
	initializeMessaging();
	parseFormTags();
	//parseScriptTags();
	if (!useEnclave) return;

	var file = makeTextFile(scriptString + formString);

	if (scriptString != "") JSport.postMessage(scriptString);

	//Redirects to file
	/*if (window.confirm("Click \"OK\" to view to parsed HTML info.")) {
	window.location.href = file;
	}*/
}

main();
