const ON_BLUR = 0;
const ON_FOCUS = 1;
const ADD_FORM = 3;
const ADD_SCRIPT = 4;
const SUBMIT_FORM = 5;
const RUN_SCRIPT = 10;
const RUN_SCRIPT_JS = 11;

//Get all script tags
var scriptArray = document.getElementsByTagName("script");
var scriptString = "";

var origin = null;

//Get all form tags
var formArray = document.getElementsByTagName("form");
var formString = "";

//Given an input, creates a text file of the input and link for that file
var textFile = null;

//var JSport = null;
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
		var focusInfo = {formName, inputName, xCoord, yCoord};
		//formPort.postMessage("The following element has just been focused on:\n" + JSON.stringify(focusInfo));
		console.log("The following element has just been focused on:");
		
		var m = "" + ON_FOCUS + '\n' + formName + '\n' + inputName + '\n' + xCoord + '\n' + yCoord + '\n' + height + '\n' + width + '\n';
		console.log(m);
		formPort.postMessage(m);

	}
}

/* Gets the name of the unfocused input, the form name, and the xy coordinates of the input.
 * An unfocused input is one that was focused on, but is no longer due to being clicked off of.
 * The information is stored as an objected currently and sent to the background script,
 * which is in turn sent to the enclave manager.
 */
function onFocusOut() {
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
		console.log("The following element has just been unfocused:");
		console.log(focusInfo);
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
		if (scriptArray[i].hasAttribute("secure") && scriptArray[i].hasAttribute("sign")) {
            
			scriptString += (ADD_SCRIPT + "\n");
			scriptString += scriptArray[i].getAttribute("sign") + "\n";

			if (scriptArray[i].hasAttribute("src")) {
				extractJS(scriptArray[i].src);
			} else {
				scriptString += scriptArray[i].innerHTML.replace(/(\r\n\t|\n|\r\t)/gm,"") + "\n";
			}

			//using formPort for everything since I don't understand this 
			//communication b/w extension and manager stuff ~saba
            formPort.postMessage(scriptString);
            
            scriptString = "";
            
            
		} else {
			//throw "This JavaScript has an invalid signature";
		}
	}
}

function getInputCoords(form, input) {
    elemRect = input.getBoundingClientRect();
    formRect = form.getBoundingClientRect();

	yCoord = (elemRect.top - formRect.top);
	xCoord = (elemRect.left - formRect.left);

	var height = input.clientHeight;
	var width = input.clientWidth;

	formString += width + "\n";
	formString += height + "\n";
	formString += xCoord + "\n";
	formString += yCoord + "\n";
}

function formSubmission(e) {
	e = e || window.event;
    var target = e.target || e.srcElement;
    e.preventDefault();
    console.log(SUBMIT_FORM + "\n" + getFormName(target) + "\n");
    formPort.postMessage(SUBMIT_FORM + "\n" + getFormName(target) + "\n");
}

//copying form submission but changing to run js. ~saba
function runJS(e) {
    e = e || window.event;
    var target = e.target || e.srcElement;
    e.preventDefault();
    console.log(RUN_SCRIPT + "\n" + getFormName(target) + "\n");
    formPort.postMessage(RUN_SCRIPT + "\n" + getFormName(target) + "\n");
}

function getInputs(form) {
  for (var i = 0; i < form.elements.length; i++) {
    if (form.elements[i].nodeName == "INPUT") {
      var acceptedTypes = ["text", "number", "tel", "email", "password", "url"];
      var type = form.elements[i].getAttribute("type");

      if (form.elements[i].hasAttribute("type")) {
    	if (form.elements[i].getAttribute("type") == "submit") {
            if(!form.hasAttribute("onsubmit")){//normal submission
                form.elements[i].addEventListener("click", formSubmission); 
            }
            else{//call script
                form.elements[i].addEventListener("click", runJS);
            }
    	}
      }

      if (acceptedTypes.indexOf(type) == -1) continue;

    } 
    else continue;

    input = form.elements[i];

    formString += input.name + "\n";
    getInputCoords(form, input);

  }
}

function getOrigin() {
	if (document.getElementsByTagName("head")[0].hasAttribute("origin")) {
		origin = document.getElementsByTagName("head")[0].getAttribute("origin");
	} else {
		throw "No origin specified in HTML Head";
		return;
	}
}

//Iterate through all form tags, parse them, and write their contents to a string
function parseFormTags() {
	for (var i = 0; i < formArray.length; i++) {
		var form = formArray[i];
		formString += ADD_FORM.toString() + "\n";
		formString += form.name + "\n";
		formString += form.getAttribute("sign") + "\n";
		getOrigin();
		formString += origin + "\n";
		formRect = form.getBoundingClientRect();

		formX = window.screenX + formRect.left;
		formY = window.screenY + formRect.top + window.outerHeight - window.innerHeight;
		console.log("Form x,y: " + formX + ", " + formY);
		formString += formX + "\n";
		formString += formY + "\n";
        if(form.hasAttribute("onsubmit")){
            formString += form.getAttribute("onsubmit") + "\n";
        }
        else{
            formString += " " + "\n";
        }
		getInputs(form);
		formPort.postMessage(formString);
		formString = "";
		listen(form);

	}
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
	formPort = chrome.runtime.connect({name: "Message"});
	formPort.onMessage.addListener(function(msg) {
		if (msg.search("form_submit") != -1) {
			var submission = JSON.parse(msg);
			console.log(btoa(submission.form_submit));
			window.location = origin + "?submit=" + submission.form_submit;
		} else console.log("received Message: \n" + msg.valueOf());
	});
}

function isSecure() {
	return document.getElementsByTagName("head")[0].hasAttribute("secure");
}

function main() {
	if (!isSecure()) {
		alert("insecure form");
		return;
	}

	initializeMessaging();

	parseFormTags();
	parseScriptTags();

	var file = makeTextFile(scriptString + formString);

	//if (scriptString != "") JSport.postMessage(scriptString);

	//Redirects to file
	/*if (window.confirm("Click \"OK\" to view to parsed HTML info.")) {
	window.location.href = file;
	}*/
}

main();
