/**********************************
 * INIT: file for Web Enclave APIs
 **********************************/


// Wrapper for native functions
function __load_data() {
    var __native_data = __native_js_load_items();
    return eval(__native_data);
}

function __save_data(d) {
    var d = JSON.stringify(d);
    __native_js_save_items(d);
}

var __data = __load_data()

/*
 * Secure Web Storage 
 */

var __web_storage_data = __data['webstorage'];

class SecureWebStorage {
    setItem(k, v) {
        return __web_storage_data[k] = v
    }
    getValue(k) {
        return __web_storage_data[k];
    }
    clear() {
        __web_storage_data = {};
    }
    
    removeItem(k){
        __web_storage_data[k] = undefined;
    }
}

localStorage = new SecureWebStorage();


/*
 * Secure XMLHttpRequest
 */


class SecureXMLHttpRequest {

    open(method, url, tf){
        this.method = method;
        this.url = url;
    }
    
    setRequestHeader(header, val){
        this.header = header+'='+val;
    }
    
    send(data){
        sendRequest(this.method, this.url, this.header, data);
    }
}

function sendRequest(method, url, headers, body) {
    __native_js_send_http_request(method, url, headers, body);
}









