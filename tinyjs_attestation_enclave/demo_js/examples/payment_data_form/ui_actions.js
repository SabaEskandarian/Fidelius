/*
 * UI Actions
 */

function doPay(e) {
    //transform form to dict
    data = toDict(document.payment);

    //form validation
    card = new cardValidator(data.cc);
    if (!card.validateCard()) {
        alert("Credit Card Number not valid.");
        e.preventDefault();
        return;
    }

    if (!card.validateExpiry(data.expiry_month + "/" + data.expiry_year)) {
        alert("Expiry date not valid.");
        e.preventDefault();
        return;
    }

    if (!card.validateCvv(data.cvv)) {
        alert("CVV not valid.");
        e.preventDefault();
        return;
    }

    //submission happens automagically
}

// register form submit handler
document.payment.onsubmit = doPay;
