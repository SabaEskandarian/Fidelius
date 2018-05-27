/*
 * UI Actions
 */

function storeCreditCardData(data){
    window.localStorage.setItem("holder", data.holder)
    window.localStorage.setItem("cc", data.cc)
    window.localStorage.setItem("expiry_month", data.expiry_month)
    window.localStorage.setItem("expiry_year", data.expiry_month)
    window.localStorage.setItem("cvv", data.cvv)
}

function doPay(e) {
    //stop default form submission 
    e.preventDefault();

    //transform form to JSON
    data = toDict(document.payment);

    //form validation
    card = new cardValidator(data.cc);
    if (!card.validateCard()) {
        alert("Credit Card Number not valid.");
        return;
    }

    if (!card.validateExpiry(data.expiry_month + "/" + data.expiry_year)) {
        alert("Expiry date not valid.");
        return;
    }

    if (!card.validateCvv(data.cvv)) {
        alert("CVV not valid.");
        return;
    }
    
    //serialize
    json_str = JSON.stringify(data);

    //send asynchronous request
    var xhr = new XMLHttpRequest();
    xhr.open(document.payment.method, document.payment.action, true);
    xhr.setRequestHeader('Content-Type', 'application/json; charset=UTF-8');
    xhr.send(json_str);
    xhr.onload = function() {
        window.location = "/confirmation";
        storeCreditCardData(data);
    };
}

// register form submit handler
document.payment.onsubmit = doPay;
