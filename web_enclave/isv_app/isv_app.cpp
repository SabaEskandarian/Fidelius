/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// This sample is confined to the communication between a SGX client platform
// and an ISV Application Server.

// enclave manager imports
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <stdlib.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "keyboard_driver.h"
#include "btchannel.h"

///////////////////////////////////
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string>



// Needed for definition of remote attestation messages.
#include "remote_attestation_result.h"

#include "isv_enclave_u.h"

// Needed to call untrusted key exchange library APIs, i.e. sgx_ra_proc_msg2.
#include "sgx_ukey_exchange.h"

// Needed to get service provider's information, in your real project, you will
// need to talk to real server.
#include "network_ra.h"

// Needed to create enclave and do ecall.
#include "sgx_urts.h"

// Needed to query extended epid group id.
#include "sgx_uae_service.h"

#include "service_provider.h"
#ifndef SAFE_FREE
#define SAFE_FREE(ptr)     \
    {                      \
        if (NULL != (ptr)) \
        {                  \
            free(ptr);     \
            (ptr) = NULL;  \
        }                  \
    }
#endif

// In addition to generating and sending messages, this application
// can use pre-generated messages to verify the generation of
// messages and the information flow.
#include "sample_messages.h"
#include <chrono>
#include <unistd.h>

typedef std::chrono::high_resolution_clock::time_point TimeVar;
#define duration(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()
#define absTime() std::chrono::duration_cast<std::chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()
#define ENCLAVE_PATH "isv_enclave.signed.so"
#define ADD_OVERLAY_HDR_LEN 16
#define BUFFER_LEN 200

sgx_enclave_id_t enclave_id;

double SCALE_X = 1;
double SCALE_Y = 1;

uint8_t *msg1_samples[] = {msg1_sample1, msg1_sample2};
uint8_t *msg2_samples[] = {msg2_sample1, msg2_sample2};
uint8_t *msg3_samples[MSG3_BODY_SIZE] = {msg3_sample1, msg3_sample2};
uint8_t *attestation_msg_samples[] =
    {attestation_msg_sample1, attestation_msg_sample2};

using namespace std;

bool NODEVICES_MODE = true;

KeyboardDriver KB(NODEVICES_MODE ? "keyboard_test" : "/dev/ttyACM0","/dev/ttyACM1");


ofstream myfile;
ofstream em_timestamps("em_timestamps.csv");
ofstream ex_timestamps("ex_timestamps.csv");
long attestationBeginTime;
mutex myfileLock;
mutex timesLock;

int runAttestation();

void threadSafePrint(string str) {
  myfileLock.lock();
  myfile << str << endl;
  myfile.flush();
  myfileLock.unlock();
}

void printToTimes(string str) {
  timesLock.lock();
  em_timestamps << str << "," << absTime() << endl; 
  timesLock.unlock();
}


void sendMessage(string message)
{
    unsigned int len = message.length();
    cout << char((len >> 0) & 0xFF)
         << char((len >> 8) & 0xFF)
         << char((len >> 16) & 0xFF)
         << char((len >> 24) & 0xFF)
         << message << endl;
}

uint8_t convert(char *target)
{
    uint8_t number = (int)strtol(target, NULL, 16);
    return number;
}

void hexStrtoBytes(char *char_stream, int stream_len, uint8_t *byte_array)
{
    for (int i = 0; i < stream_len; i = i + 2)
    {
        char array[2];
        array[0] = *(char_stream + i);
        array[1] = *(char_stream + i + 1);
        byte_array[i / 2] = convert(&array[0]);
    }
}

void ocall_get_file_size(size_t* len)
{
    std::string name = std::string("localStorage.data");
    ifstream file(name, ifstream::ate | ifstream::binary);
    if (file.good())
    {
        *len = file.tellg();
        file.close();
    }
    else
    {
        *len = 0;
    }
    //myfile << "saying len is " <<*len <<"\n";
}

void ocall_write_file(uint8_t* data, size_t len)
{
    std::string name = "localStorage.data";
    //myfile << "writing to file \n content: " << (int)data[0] <<"\n length: "<<len<<"\n";
    ofstream file(name, ofstream::binary);
    //file << data << endl;
    file.write ((char*)data, len);
    file.flush();
    file.close();
}

void ocall_read_file(uint8_t* data, size_t len)
{
    std::string name = "localStorage.data";
    ifstream file(name, ifstream::binary);
    file.read((char*) data, len);
    //myfile << "reading from file \n content: " << (int)data[0] <<"\n length: "<<len<<"\n";
    file.close();
}

void ocall_print_string(const char *str)
{
   threadSafePrint(str);
}

void ocall_print_time(const char *str)
{
   printToTimes(str);
}

std::string hexStr(unsigned char* data, int len)
{
    std::stringstream ss;
    ss << std::hex;
    for(int i=0;i<len;++i) {
      if ((int)data[i] < 16) {
        ss << '0' << (int)data[i];
      } else {
        ss << (int)data[i];
      }
    myfile << "str = " << ss.str().length() << endl;
    }
    return ss.str();
}

void ocall_print_hex(const char *str, size_t len)
{
   threadSafePrint(hexStr((unsigned char*)str, len));
}

sgx_status_t enc_make_http_request(const char *method, const char *url,
                                   const char *headers, const char *request_data,
                                   uint8_t *p_mac, int *ret_code)
{
    //TODO
    *ret_code = 1;
    std::string response = "reponse";
    //TODO: INTERACT WITH BROWSER
    return SGX_SUCCESS;
}



// Some utility functions to output some of the data structures passed between
// the ISV app and the remote attestation service provider.
void PRINT_BYTE_ARRAY(
    FILE *file, void *mem, uint32_t len)
{
    if (!mem || !len)
    {
        //fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t *array = (uint8_t *)mem;
    //fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++)
    {
        //fprintf(file, "0x%x, ", array[i]);
        //if(i % 8 == 7) //fprintf(file, "\n");
    }
    //fprintf(file, "0x%x ", array[i]);
    //fprintf(file, "\n}\n");
}

void PRINT_ATTESTATION_SERVICE_RESPONSE(
    FILE *file,
    ra_samp_response_header_t *response)
{
    if (!response)
    {
        //fprintf(file, "\t\n( null )\n");
        return;
    }

    //fprintf(file, "RESPONSE TYPE:   0x%x\n", response->type);
    //fprintf(file, "RESPONSE STATUS: 0x%x 0x%x\n", response->status[0],
    // response->status[1]);
    //fprintf(file, "RESPONSE BODY SIZE: %u\n", response->size);

    if (response->type == TYPE_RA_MSG2)
    {
        sgx_ra_msg2_t *p_msg2_body = (sgx_ra_msg2_t *)(response->body);

        //fprintf(file, "MSG2 gb - ");
        PRINT_BYTE_ARRAY(file, &(p_msg2_body->g_b), sizeof(p_msg2_body->g_b));

        //fprintf(file, "MSG2 spid - ");
        PRINT_BYTE_ARRAY(file, &(p_msg2_body->spid), sizeof(p_msg2_body->spid));

        //fprintf(file, "MSG2 quote_type : %hx\n", p_msg2_body->quote_type);

        //fprintf(file, "MSG2 kdf_id : %hx\n", p_msg2_body->kdf_id);

        //fprintf(file, "MSG2 sign_gb_ga - ");
        PRINT_BYTE_ARRAY(file, &(p_msg2_body->sign_gb_ga),
                         sizeof(p_msg2_body->sign_gb_ga));

        //fprintf(file, "MSG2 mac - ");
        PRINT_BYTE_ARRAY(file, &(p_msg2_body->mac), sizeof(p_msg2_body->mac));

        //fprintf(file, "MSG2 sig_rl - ");
        PRINT_BYTE_ARRAY(file, &(p_msg2_body->sig_rl),
                         p_msg2_body->sig_rl_size);
    }
    else if (response->type == TYPE_RA_ATT_RESULT)
    {
        sample_ra_att_result_msg_t *p_att_result =
            (sample_ra_att_result_msg_t *)(response->body);
        //fprintf(file, "ATTESTATION RESULT MSG platform_info_blob - ");
        PRINT_BYTE_ARRAY(file, &(p_att_result->platform_info_blob),
                         sizeof(p_att_result->platform_info_blob));

        //fprintf(file, "ATTESTATION RESULT MSG mac - ");
        PRINT_BYTE_ARRAY(file, &(p_att_result->mac), sizeof(p_att_result->mac));

        //fprintf(file, "ATTESTATION RESULT MSG secret.payload_tag - %u bytes\n",
        //p_att_result->secret.payload_size);

        //fprintf(file, "ATTESTATION RESULT MSG secret.payload - ");
        PRINT_BYTE_ARRAY(file, p_att_result->secret.payload,
                         p_att_result->secret.payload_size);
    }
    else
    {
        //fprintf(file, "\nERROR in printing out the response. "
        //"Response of type not supported %d\n", response->type);
    }
}

// enclave manager required functions

bool DEBUG_MODE = 1;


#define ON_BLUR 0
#define ON_FOCUS 1
#define INITIALIZE_ENCLAVE 2
#define ADD_FORM 3
#define ADD_SCRIPT 4
#define RUN_SCRIPT 10
#define SUBMIT_FORM 5
#define TERMINATE_ENCLAVE 6
#define SUBMIT_HTTP_REQ 7
#define SHUTDOWN_EM 8
#define N_COMMANDS 11

/*
this ivar is for debugging only, it will close the debug
log after the EM recieves/parses the specified number of commands.
*/
int eventsUntilCloseLog = 400;



bool web_enclave_setup = false;
bool runningManager = true;
bool done_with_overlay = false;
pair<string, string> focusInput = make_pair("None", "None");
uint8_t sharedOverlayPacket[524288];
uint32_t sharedOutLen;
condition_variable_any cv;
condition_variable_any overlaycv;
bool overlayPacketInitialized = false;
mutex keyboard_mutex;
mutex overlay_mutex;
mutex curInputMutex;

int to_int(int c) {
  if (not isxdigit(c)) return -1; // error: non-hexadecimal digit found
  if (isdigit(c)) return c - '0';
  if (isupper(c)) c = tolower(c);
  return c - 'a' + 10;
}

template<class InputIterator, class OutputIterator> int unhexlify(InputIterator first, InputIterator last, OutputIterator ascii) {
  while (first != last) {
    int top = to_int(*first++);
    int bot = to_int(*first++);
    if (top == -1 or bot == -1)
      return -1; // error
    *ascii++ = (top << 4) + bot;
  }
  return 0;
}

void handleOnBlur(string formName, string inputName, double mouseX, double mouseY) {
    curInputMutex.lock();
    myfileLock.lock();
    if (DEBUG_MODE) 
      myfile << "BLUR on form: " << formName << " input: " << inputName << " at: " << mouseX << ", " << mouseY << endl;
    myfileLock.unlock();
    focusInput = make_pair("None","None");

    uint8_t b = 0;
    KB.changeMode(&b, 1);
    sgx_status_t ret;
    onBlur(enclave_id, &ret); //ECALL
    cv.notify_all();
    curInputMutex.unlock();
}

void handleOnFocus(string formName, string inputName, double x, double y, double h, double w)
{
    //sendMessage("{message: Hello from EM}");
    curInputMutex.lock();
    x *= SCALE_X;
    y *= SCALE_Y;
    h *= SCALE_Y;
    w *= SCALE_X;
    myfileLock.lock();
    if (DEBUG_MODE) 
        myfile << "FOCUS on form: " << formName << " input: " << inputName << " at: "
        << x << ", " << y 
        << " width: " << w 
        << " height: " << h << endl;
    myfileLock.unlock();
    focusInput = make_pair(formName, inputName);
    uint8_t b = 1;
    KB.changeMode(&b, 1);
    sgx_status_t ret;
    onFocus(enclave_id, &ret, formName.c_str(), inputName.c_str(), 
          (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h); //ECALL
    if (ret != SGX_SUCCESS) 
        myfile << "onFocus ecall FAILED" << endl;
    curInputMutex.unlock();
    cv.notify_all();
}

void addForm(vector<string> argv)
{
    //TO DO: make ecall for adding a form
    sgx_status_t ret;
    string name = argv[1];

    string signature = argv[2];
    char ascii[signature.length()/2+1];
    ascii[signature.length()/2] = '\0';
    unhexlify(argv[2].c_str(), argv[2].c_str() + signature.length(), ascii);

    string origin = argv[3];

    uint16_t formX = (uint16_t)(stod(argv[4], NULL)*SCALE_X);
    uint16_t formY = (uint16_t)(stod(argv[5], NULL)*SCALE_Y);
    
    string onsub = argv[6];
    myfile << "ADD FORM (x,y): (" << formX << "," << formY << ")" << endl; 
    myfile << "with name: " << name  << " signature: " << signature << endl;
    add_form(enclave_id, &ret, name.c_str(), name.length()+1, 
             origin.c_str(), origin.length()+1, formX, formY, onsub.c_str(), onsub.length()+1); //ECALL
    if (ret != SGX_SUCCESS) 
        myfile << "add_form ecall FAILED " << ret << endl;
	
	
	for (int i = 7; i < argv.size(); i+=5) 
  {
      string inputName = argv[i];
      uint16_t w = (uint16_t)(stod(argv[i + 1], NULL) * SCALE_X);
      uint16_t h = (uint16_t)(stod(argv[i + 2], NULL) * SCALE_Y);
      int16_t x = (uint16_t)(stod(argv[i + 3], NULL) * SCALE_X);
      uint16_t y = (uint16_t)(stod(argv[i + 4], NULL) * SCALE_Y);
      //uint16_t x = 0;
      //uint16_t y = 0;
      if (DEBUG_MODE)
          myfile << "\tADD INPUT: " << inputName << " (" << x << "," << y << ") " << endl;
      if (!(i + 5 < argv.size()))
          myfile << "RUNNING VALIDATION" << endl;
      add_input(enclave_id, &ret, name.c_str(), name.length() + 1, inputName.c_str(), inputName.length() + 1,
                (uint8_t *)(ascii), signature.length()/2+1,
                (i + 5 < argv.size()) ? 0 : 1, x, y, h, w); //ECALL
                                                              //0,x,y,h,w);
      if (ret != SGX_SUCCESS)
          myfile << "add_input ecall FAILED " << ret << endl;
    }
    web_enclave_setup = true;
    cv.notify_all();
    ex_timestamps << "added a form," << absTime() - attestationBeginTime << endl;
}
void addScript(string sign, string script)
{
    sgx_status_t ret;
    add_script(enclave_id, &ret, sign.c_str(), sign.length(), script.c_str(), script.length());
    if (DEBUG_MODE)
        myfile << "ADDED SCRIPT"<< endl;
    if (ret != SGX_SUCCESS) 
        myfile << "!!!add_script ecall FAILED" << endl;
    ex_timestamps << "added a script," << absTime() - attestationBeginTime << endl;
}

//The second parameter should really be some kind of struct to take parameters 
//for a function. Using the string as a placeholder for now. ~saba
void runScript(string formName)
{
    sgx_status_t ret;
    //call the onsubmit function for a form
    //that's all we'll have for now in terms of ways to start running js
    run_js(enclave_id, &ret, formName.c_str(), formName.length()+1);
    
    
    /* from void handleFormSubmission(string formName)
    sgx_status_t re;
    uint32_t len;
    form_len(enclave_id, &len, formName.c_str());
    uint8_t form_buf[(len * 4)] = {0};
    uint8_t mac[16] = {0};

    myfile << "submitting form now!" << endl;
    submit_form(enclave_id, &re, formName.c_str(), &form_buf[0], len, &mac[0]);
    myfile << form_buf << endl;

    string resultString = "{\"form_submit\" : \"";
    resultString += base64_encode(reinterpret_cast<const unsigned char *>(form_buf), len);
    resultString += "\"}";

    myfile << "Result: " << resultString << endl;
    ;

    sendMessage(resultString);
     */
    
}

void initializeEnclave(string origin)
{
    //TO DO: write code to initialize an enclave, should be similar to SampleEnclave init()
    if (DEBUG_MODE)
        myfile << "INIT enclave with origin: " << origin << endl;
      attestationBeginTime = absTime();
    //runAttestation();
}

void terminateEnclave(string origin)
{
  if (web_enclave_setup) {
    uint8_t b = 0;
    KB.changeMode(&b, 1);
    sgx_status_t ret;
    onBlur(enclave_id, &ret); //ECALL
    done_with_overlay = true;
    cv.notify_all();
    //connection.channel_close();
    if (DEBUG_MODE)
        myfile << "TERMINATE enclave with id: " << enclave_id << endl;

    //sgx_destroy_enclave(enclave_id);
  }

}
void submitHttpReq(string request)
{

    //TO DO: needs to forward request to the extension
    if (DEBUG_MODE)
        myfile << "sending HTTP request: " << request << endl;
}

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";


static inline bool is_base64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

void handleFormSubmission(string formName)
{
    sgx_status_t re;
    uint32_t len;
    form_len(enclave_id, &len, formName.c_str());
    uint8_t form_buf[(len * 4)] = {0};
    uint8_t mac[16] = {0};

    myfile << "submitting form now!" << endl;
    submit_form(enclave_id, &re, formName.c_str(), &form_buf[0], len, &mac[0]);
    myfile << form_buf << endl;

    string resultString = "{\"form_submit\" : \"";
    resultString += base64_encode(reinterpret_cast<const unsigned char *>(form_buf), len);
    resultString += "\"}";

    myfile << "Result: " << resultString << endl;
    ;

    sendMessage(resultString);
}

/* I think this is no longer needed. ~saba
void parseScript(string message)
{
    vector<string> argv;
    string sigsAndScripts = message.substr(3, message.length() - 3); //skips over 8\n
    string delimiter = "\\n#EOF#\\n";
    size_t pos = 0;
    string token;

    while ((pos = sigsAndScripts.find(delimiter)) != string::npos)
    {
        token = sigsAndScripts.substr(0, pos);
        argv.push_back(token);
        sigsAndScripts.erase(0, pos + delimiter.length());
    }
    for (int i = 0; i < argv.size() / 2; i++)
    {
        addScript(argv[i * 2], argv[i * 2 + 1]);
        myfile << "\n\n\n";
    }
}
*/

bool parseMessage(string message)
{
    eventsUntilCloseLog--;

    vector<string> argv;

    char *comstr = &message.at(0);
    int command = atoi(comstr);

    //if (command == ADD_SCRIPT)
    //{
    //    parseScript(message);
    //    return true;
    //}
    string delimiter = "\\n";
    size_t pos = 0;
    string token;

    string s = message;
    while ((pos = s.find(delimiter)) != string::npos)
    {
        token = s.substr(0, pos);
        argv.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    //myfile << "EM found: " << argv.size() << " arguments delimited by \\n" << endl;

    if (command < 0 || command >= N_COMMANDS)
    {
        myfile << "no command: " << command << " found" << endl;
        return true;
    }

    //TODO: ADD CASE FOR HANDELING HTTP RESPONSE
    switch (command)
    {
    case ON_BLUR:
        handleOnBlur(argv[1], argv[2], stod(argv[3], NULL), stod(argv[4], NULL));
        break;
    case ON_FOCUS:
        handleOnFocus(argv[1], argv[2], stod(argv[3], NULL), stod(argv[4], NULL), stod(argv[5], NULL), stod(argv[6], NULL));
        break;
    case INITIALIZE_ENCLAVE:
        initializeEnclave(argv[1]);
        break;
    case ADD_FORM:
        addForm(argv);
        break;
    case SUBMIT_FORM:
        myfile << "REQUESTED FORM SUBMIT" << endl;
        handleFormSubmission(argv[1]);
        break;
    case ADD_SCRIPT:
        myfile << "ADDING SCRIPT" << endl;
        addScript(argv[1], argv[2]);
        break;
    case RUN_SCRIPT:
        myfile << "REQUESTED FORM SUBMIT -- running script" << endl;
        runScript(argv[1]);
        break;
    case TERMINATE_ENCLAVE:
        terminateEnclave(argv[1]);
        break;
    case SUBMIT_HTTP_REQ:
        submitHttpReq(argv[1]);
        break;
    case SHUTDOWN_EM:
        return false;
    }
    myfile << "\n\n";
    if (eventsUntilCloseLog == 0)
    {
        return false;
    }
    return true;
}

string getPacketStr(uint8_t *outBuff) {
    char str[BUFFER_LEN+1];
    str[BUFFER_LEN] = '\0';
    memcpy(str, (char*)outBuff + ADD_OVERLAY_HDR_LEN, BUFFER_LEN);
    return str;
}

void sendToDisplay()
{

    if (NODEVICES_MODE) {
        return;
    }
    BluetoothChannel connection;

    threadSafePrint("CREATED BT THREAD");
    
    ofstream em_times("em_bt_times.csv");
    
    int numPacketsSent = 0;
    if (connection.channel_open() < 0)
    {
        myfile << "FAILED BT CONNECT" << endl;
        myfile.close();
        return;
    }
    threadSafePrint("BT CONNECTED");
    uint8_t outBuff[524288];
    mutex overlayInit_mutex;
    unique_lock<mutex> lk(overlayInit_mutex);
    uint32_t out_len;
    while(true) {
           
      overlaycv.wait(lk,[]{return overlayPacketInitialized;});
      if (done_with_overlay) break; 
      overlay_mutex.lock();
      out_len = sharedOutLen;
      memcpy(outBuff, sharedOverlayPacket, out_len);
      overlayPacketInitialized = false;
      overlay_mutex.unlock();
    	//TimeVar btsendStart = timeNow();

    	em_times << "bm send: " << getPacketStr(outBuff) << "," << absTime() << endl;
      //threadSafePrint("sending pack: " + getPacketStr(outBuff));
      if (connection.channel_send((char *)outBuff, (int)out_len) < 0) {
        //threadSafePrint("BT FAILED TO SEND PACKET\n");
      } else {
        //auto dur = chrono::duration_cast<chrono::microseconds>(timeNow() - btsendStart);
        //threadSafePrint("BT SENT PACKET time elapsed: " + to_string(dur.count()) + " num packets sent: " + to_string(numPacketsSent));
        numPacketsSent++;
      }
      //em_times << "bluetooth sent," << absTime() << endl;
      usleep(150000);
    }
    create_remove_overlay_msg(enclave_id, outBuff, &out_len, focusInput.first.c_str());
    connection.channel_send((char *)outBuff, (int)out_len);
    myfile << "sending remove overlay message" << endl;

    
}


void listenForKeyboard()
{
    if (NODEVICES_MODE) {
        return;
    }
    ofstream em_times("em_kb_times.csv");
    int numPacketsSent = 0;
    threadSafePrint("CREATED KB THREAD");
    uint8_t outBuff[524288];
    uint32_t out_len;
    while (true)
    {
        unique_lock<mutex> lk(keyboard_mutex);
        cv.wait(lk,[]{return web_enclave_setup || done_with_overlay;});
        if(!done_with_overlay) {
          curInputMutex.lock();
          create_add_overlay_msg(enclave_id, outBuff, &out_len, focusInput.first.c_str()); //make display ECALL
          em_times << "wait for refresh: " << getPacketStr(outBuff) << "," << absTime() << endl;
          curInputMutex.unlock();
        }
        
        overlay_mutex.lock();
        memcpy(sharedOverlayPacket, outBuff, out_len);
        sharedOutLen = out_len;
        overlayPacketInitialized = true;
        overlaycv.notify_all();
        overlay_mutex.unlock();
        uint8_t keyboardBuff[58] = {0};
        int enc_bytes = KB.getEncryptedKeyboardInput(keyboardBuff, 58, false);
        em_times << "got new char," << absTime() << endl;
        uint8_t bytebuff[29];
        hexStrtoBytes((char *)keyboardBuff, 58, bytebuff);
        sgx_status_t ret;

        
        get_keyboard_chars(enclave_id, &ret, bytebuff); //make keyboard ECALL
        //if (ret != SGX_SUCCESS) 
          //threadSafePrint("ERROR DECRYPTING CHAR");
    }
    
}

////////////////////////////////
// This sample code doesn't have any recovery/retry mechanisms for the remote
// attestation. Since the enclave can be lost due S3 transitions, apps
// susceptible to S3 transitions should have logic to restart attestation in
// these scenarios.
#define _T(x) x
int main(int argc, char *argv[])
{
   
    myfile.open ("debug_log.txt");
    runAttestation();
    threadSafePrint("done with attestation");


    thread kb_thread(listenForKeyboard);
    thread ds_thread(sendToDisplay);
    
    //addForm(vector<string>({"3", "myForm", "sig", "origin", "123", "312", "inp1", "1", "1", "1", "1", "inp2", "12", "32", "2", "2"}));
    while (1)
    {
        unsigned int length = 0;
        //read the first four bytes (=> Length)
        for (int i = 0; i < 4; i++)
        {
            unsigned int read_char = getchar();
            length = length | (read_char << i * 8);
        }


        string msg = "";

        for (int i = 0; i < length; i++)
        {
            msg += getchar();
        }

        msg = msg.substr(1, msg.length() - 2);

        if (!parseMessage(msg))
        {
            break;
        }
    }

    myfile << "SHUTTING DOWN EM" << endl;

    myfile.close();

    sgx_destroy_enclave(enclave_id);

    kb_thread.join();
    ds_thread.join();
    return 0;
}

int runAttestation()
{
  //attestationBeginTime = absTime();
  ex_timestamps << "attestation begin," << absTime() << endl; 
  int ret = 0;
    ra_samp_request_header_t *p_msg0_full = NULL;
    ra_samp_response_header_t *p_msg0_resp_full = NULL;
    ra_samp_request_header_t *p_msg1_full = NULL;
    ra_samp_response_header_t *p_msg2_full = NULL;
    sgx_ra_msg3_t *p_msg3 = NULL;
    ra_samp_response_header_t *p_att_result_msg_full = NULL;
    enclave_id = 0;
    int enclave_lost_retry_time = 1;
    int busy_retry_time = 4;
    sgx_ra_context_t context = INT_MAX;
    sgx_status_t status = SGX_SUCCESS;
    ra_samp_request_header_t *p_msg3_full = NULL;

    int32_t verify_index = 1; // -1;
    int32_t verification_samples = sizeof(msg1_samples) / sizeof(msg1_samples[0]);

    FILE *OUTPUT = fopen("EM_report.txt", "w"); //stdout;

    /*if (NODEVICES_MODE){
        fprintf(OUTPUT, "Running in NODEVICES_MODE\n");
        fflush(OUTPUT);
    }*/
    ////fprintf(stdout, "\nhelp1 \n");

#define VERIFICATION_INDEX_IS_VALID() (verify_index > 0 && \
                                       verify_index <= verification_samples)
#define GET_VERIFICATION_ARRAY_INDEX() (verify_index - 1)
    /*fprintf(OUTPUT, "argc: %d \n", argc);
    for (int i = 0; i < argc; i++)
    {

        fprintf(OUTPUT, "argv[%d]: %s \n", i, argv[i]);
    }
    if (argc > 2)
    {

        verify_index = atoi(argv[1]);
        fprintf(OUTPUT, "verify index: %d\n", verify_index);
        if (VERIFICATION_INDEX_IS_VALID())
        {
            fprintf(OUTPUT, "\nVerifying precomputed attestation messages using precomputed values# %d\n", verify_index);
        }
        else
        {
            fprintf(OUTPUT, "\nValid invocations are:\n");
            fprintf(OUTPUT, "\n\tisv_app\n");
            fprintf(OUTPUT, "\n\tisv_app <verification index>\n");
            fprintf(OUTPUT, "\nValid indices are [1 - %d]\n", verification_samples);
            fprintf(OUTPUT, "\nUsing a verification index uses precomputed messages to assist debugging the remote attestation service provider.\n");
            return -1;
        }
    }*/

    // Preparation for remote attestation by configuring extended epid group id.
    {
        uint32_t extended_epid_group_id = 0;
        ret = sgx_get_extended_epid_group_id(&extended_epid_group_id);
        if (SGX_SUCCESS != ret)
        {
            ret = -1;
            fprintf(OUTPUT, "\nError, call sgx_get_extended_epid_group_id fail [%s].", __FUNCTION__);
            return ret;
        }
        fprintf(OUTPUT, "\nCall sgx_get_extended_epid_group_id success.");

        p_msg0_full = (ra_samp_request_header_t *)
            malloc(sizeof(ra_samp_request_header_t) + sizeof(uint32_t));
        if (NULL == p_msg0_full)
        {
            ret = -1;
            goto CLEANUP;
        }
        p_msg0_full->type = TYPE_RA_MSG0;
        p_msg0_full->size = sizeof(uint32_t);

        *(uint32_t *)((uint8_t *)p_msg0_full + sizeof(ra_samp_request_header_t)) = extended_epid_group_id;
        {

            fprintf(OUTPUT, "\nMSG0 body generated -\n");

            PRINT_BYTE_ARRAY(OUTPUT, p_msg0_full->body, p_msg0_full->size);
        }
        // The ISV application sends msg0 to the SP.
        // The ISV decides whether to support this extended epid group id.
        fprintf(OUTPUT, "\nSending msg0 to remote attestation service provider.\n");

        ret = ra_network_send_receive("http://SampleServiceProvider.intel.com/",
                                      p_msg0_full,
                                      &p_msg0_resp_full);
        if (ret != 0)
        {
            fprintf(OUTPUT, "\nError, ra_network_send_receive for msg0 failed "
                            "[%s].",
                    __FUNCTION__);
            goto CLEANUP;
        }
        fprintf(OUTPUT, "\nSent MSG0 to remote attestation service.\n");
    }

    // Remote attestation will be initiated the ISV server challenges the ISV
    // app or if the ISV app detects it doesn't have the credentials
    // (shared secret) from a previous attestation required for secure
    // communication with the server.
    {
        // ISV application creates the ISV enclave.
        int launch_token_update = 0;
        sgx_launch_token_t launch_token = {0};
        memset(&launch_token, 0, sizeof(sgx_launch_token_t));
        do
        {
            ret = sgx_create_enclave(_T(ENCLAVE_PATH),
                                     SGX_DEBUG_FLAG,
                                     &launch_token,
                                     &launch_token_update,
                                     &enclave_id, NULL);
            if (SGX_SUCCESS != ret)
            {
                ret = -1;
                fprintf(OUTPUT, "\nError, call sgx_create_enclave fail [%s].", __FUNCTION__);
                goto CLEANUP;
            }
            fprintf(OUTPUT, "\nCall sgx_create_enclave success.");

            ret = enclave_init_ra(enclave_id,
                                  &status,
                                  false,
                                  &context);
            //Ideally, this check would be around the full attestation flow.
        } while (SGX_ERROR_ENCLAVE_LOST == ret && enclave_lost_retry_time--);

        if (SGX_SUCCESS != ret || status)
        {
            ret = -1;
            fprintf(OUTPUT, "\nError, call enclave_init_ra fail [%s].", __FUNCTION__);
            goto CLEANUP;
        }
        fprintf(OUTPUT, "\nCall enclave_init_ra success.");

        // isv application call uke sgx_ra_get_msg1
        p_msg1_full = (ra_samp_request_header_t *)
            malloc(sizeof(ra_samp_request_header_t) + sizeof(sgx_ra_msg1_t));
        if (NULL == p_msg1_full)
        {
            ret = -1;
            goto CLEANUP;
        }
        p_msg1_full->type = TYPE_RA_MSG1;
        p_msg1_full->size = sizeof(sgx_ra_msg1_t);
        do
        {
            ret = sgx_ra_get_msg1(context, enclave_id, sgx_ra_get_ga,
                                  (sgx_ra_msg1_t *)((uint8_t *)p_msg1_full + sizeof(ra_samp_request_header_t)));
            sleep(3); // Wait 3s between retries
        } while (SGX_ERROR_BUSY == ret && busy_retry_time--);
        if (SGX_SUCCESS != ret)
        {
            ret = -1;
            fprintf(OUTPUT, "\nError, call sgx_ra_get_msg1 fail [%s].", __FUNCTION__);
            goto CLEANUP;
        }
        else
        {
            fprintf(OUTPUT, "\nCall sgx_ra_get_msg1 success.\n");

            fprintf(OUTPUT, "\nMSG1 body generated -\n");

            PRINT_BYTE_ARRAY(OUTPUT, p_msg1_full->body, p_msg1_full->size);
        }

        if (VERIFICATION_INDEX_IS_VALID())
        {

            memcpy_s(p_msg1_full->body, p_msg1_full->size,
                     msg1_samples[GET_VERIFICATION_ARRAY_INDEX()],
                     p_msg1_full->size);

            fprintf(OUTPUT, "\nInstead of using the recently generated MSG1, we will use the following precomputed MSG1 -\n");

            PRINT_BYTE_ARRAY(OUTPUT, p_msg1_full->body, p_msg1_full->size);
        }

        // The ISV application sends msg1 to the SP to get msg2,
        // msg2 needs to be freed when no longer needed.
        // The ISV decides whether to use linkable or unlinkable signatures.
        fprintf(OUTPUT, "\nSending msg1 to remote attestation service provider.Expecting msg2 back.\n");

        ret = ra_network_send_receive("http://SampleServiceProvider.intel.com/",
                                      p_msg1_full,
                                      &p_msg2_full);

        if (ret != 0 || !p_msg2_full)
        {
            fprintf(OUTPUT, "\nError, ra_network_send_receive for msg1 failed [%s].", __FUNCTION__);
            if (VERIFICATION_INDEX_IS_VALID())
            {
                fprintf(OUTPUT, "\nBecause we are in verification mode we will ignore this error.\n");
                fprintf(OUTPUT, "\nInstead, we will pretend we received the following MSG2 - \n");

                SAFE_FREE(p_msg2_full);
                ra_samp_response_header_t *precomputed_msg2 =
                    (ra_samp_response_header_t *)msg2_samples[GET_VERIFICATION_ARRAY_INDEX()];
                const size_t msg2_full_size = sizeof(ra_samp_response_header_t) + precomputed_msg2->size;
                p_msg2_full =
                    (ra_samp_response_header_t *)malloc(msg2_full_size);
                if (NULL == p_msg2_full)
                {
                    ret = -1;
                    goto CLEANUP;
                }
                memcpy_s(p_msg2_full, msg2_full_size, precomputed_msg2,
                         msg2_full_size);

                //PRINT_BYTE_ARRAY(OUTPUT, p_msg2_full,
                //               sizeof(ra_samp_response_header_t)
                //             + p_msg2_full->size);
            }
            else
            {
                goto CLEANUP;
            }
        }
        else
        {
            // Successfully sent msg1 and received a msg2 back.
            // Time now to check msg2.
            if (TYPE_RA_MSG2 != p_msg2_full->type)
            {

                fprintf(OUTPUT, "\nError, didn't get MSG2 in response to MSG1. [%s].", __FUNCTION__);

                if (VERIFICATION_INDEX_IS_VALID())
                {
                    fprintf(OUTPUT, "\nBecause we are in verification mode we will ignore this error.");
                }
                else
                {
                    goto CLEANUP;
                }
            }

            fprintf(OUTPUT, "\nSent MSG1 to remote attestation service provider. Received the following MSG2:\n");
            //PRINT_BYTE_ARRAY(OUTPUT, p_msg2_full,
            //               sizeof(ra_samp_response_header_t)
            //             + p_msg2_full->size);

            fprintf(OUTPUT, "\nA more descriptive representation of MSG2:\n");
            PRINT_ATTESTATION_SERVICE_RESPONSE(OUTPUT, p_msg2_full);

            if (VERIFICATION_INDEX_IS_VALID())
            {
                // The response should match the precomputed MSG2:
                ra_samp_response_header_t *precomputed_msg2 =
                    (ra_samp_response_header_t *)
                        msg2_samples[GET_VERIFICATION_ARRAY_INDEX()];
                if (memcmp(precomputed_msg2, p_msg2_full,
                           sizeof(ra_samp_response_header_t) + p_msg2_full->size))
                {
                    fprintf(OUTPUT, "\nVerification ERROR. Our precomputed value for MSG2 does NOT match.\n");
                    fprintf(OUTPUT, "\nPrecomputed value for MSG2:\n");
                    //PRINT_BYTE_ARRAY(OUTPUT, precomputed_msg2,
                    //               sizeof(ra_samp_response_header_t)
                    //              + precomputed_msg2->size);
                    fprintf(OUTPUT, "\nA more descriptive representation of precomputed value for MSG2:\n");
                    PRINT_ATTESTATION_SERVICE_RESPONSE(OUTPUT, precomputed_msg2);
                }
                else
                {
                    fprintf(OUTPUT, "\nVerification COMPLETE. Remote attestation service provider generated a matching MSG2.\n");
                }
            }
        }

        sgx_ra_msg2_t *p_msg2_body = (sgx_ra_msg2_t *)((uint8_t *)p_msg2_full + sizeof(ra_samp_response_header_t));

        uint32_t msg3_size = 0;
        if (VERIFICATION_INDEX_IS_VALID())
        {
            // We cannot generate a valid MSG3 using the precomputed messages
            // we have been using. We will use the precomputed msg3 instead.
            msg3_size = MSG3_BODY_SIZE;
            p_msg3 = (sgx_ra_msg3_t *)malloc(msg3_size);
            if (NULL == p_msg3)
            {
                ret = -1;
                goto CLEANUP;
            }
            memcpy_s(p_msg3, msg3_size,
                     msg3_samples[GET_VERIFICATION_ARRAY_INDEX()], msg3_size);
            ////fprintf(OUTPUT, "\nBecause MSG1 was a precomputed value, the MSG3 "
            //"we use will also be. PRECOMPUTED MSG3 - \n");
        }
        else
        {
            busy_retry_time = 2;
            // The ISV app now calls uKE sgx_ra_proc_msg2,
            // The ISV app is responsible for freeing the returned p_msg3!!
            do
            {
                ret = sgx_ra_proc_msg2(context,
                                       enclave_id,
                                       sgx_ra_proc_msg2_trusted,
                                       sgx_ra_get_msg3_trusted,
                                       p_msg2_body,
                                       p_msg2_full->size,
                                       &p_msg3,
                                       &msg3_size);
            } while (SGX_ERROR_BUSY == ret && busy_retry_time--);
            if (!p_msg3)
            {
                ////fprintf(OUTPUT, "\nError, call sgx_ra_proc_msg2 fail. "
                // "p_msg3 = 0x%p [%s].", p_msg3, __FUNCTION__);
                ret = -1;
                goto CLEANUP;
            }
            if (SGX_SUCCESS != (sgx_status_t)ret)
            {
                ////fprintf(OUTPUT, "\nError, call sgx_ra_proc_msg2 fail. "
                // "ret = 0x%08x [%s].", ret, __FUNCTION__);
                ret = -1;
                goto CLEANUP;
            }
            else
            {
                ////fprintf(OUTPUT, "\nCall sgx_ra_proc_msg2 success.\n");
                ////fprintf(OUTPUT, "\nMSG3 - \n");
            }
        }

        //PRINT_BYTE_ARRAY(OUTPUT, p_msg3, msg3_size);

        p_msg3_full = (ra_samp_request_header_t *)malloc(
            sizeof(ra_samp_request_header_t) + msg3_size);
        if (NULL == p_msg3_full)
        {
            ret = -1;
            goto CLEANUP;
        }
        p_msg3_full->type = TYPE_RA_MSG3;
        p_msg3_full->size = msg3_size;
        if (memcpy_s(p_msg3_full->body, msg3_size, p_msg3, msg3_size))
        {
            ////fprintf(OUTPUT,"\nError: INTERNAL ERROR - memcpy failed in [%s].",
            //__FUNCTION__);
            ret = -1;
            goto CLEANUP;
        }

        // The ISV application sends msg3 to the SP to get the attestation
        // result message, attestation result message needs to be freed when
        // no longer needed. The ISV service provider decides whether to use
        // linkable or unlinkable signatures. The format of the attestation
        // result is up to the service provider. This format is used for
        // demonstration.  Note that the attestation result message makes use
        // of both the MK for the MAC and the SK for the secret. These keys are
        // established from the SIGMA secure channel binding.
        ret = ra_network_send_receive("http://SampleServiceProvider.intel.com/",
                                      p_msg3_full,
                                      &p_att_result_msg_full);
        if (ret || !p_att_result_msg_full)
        {
            ret = -1;
            ////fprintf(OUTPUT, "\nError, sending msg3 failed [%s].", __FUNCTION__);
            goto CLEANUP;
        }

        sample_ra_att_result_msg_t *p_att_result_msg_body =
            (sample_ra_att_result_msg_t *)((uint8_t *)p_att_result_msg_full + sizeof(ra_samp_response_header_t));
        if (TYPE_RA_ATT_RESULT != p_att_result_msg_full->type)
        {
            ret = -1;
            ////fprintf(OUTPUT, "\nError. Sent MSG3 successfully, but the message "
            //"received was NOT of type att_msg_result. Type = "
            //"%d. [%s].", p_att_result_msg_full->type,
            //__FUNCTION__);
            goto CLEANUP;
        }
        else
        {
            ////fprintf(OUTPUT, "\nSent MSG3 successfully. Received an attestation "
            // "result message back\n.");
            if (VERIFICATION_INDEX_IS_VALID())
            {
                if (memcmp(p_att_result_msg_full->body,
                           attestation_msg_samples[GET_VERIFICATION_ARRAY_INDEX()],
                           p_att_result_msg_full->size))
                {
                    ////fprintf(OUTPUT, "\nSent MSG3 successfully. Received an "
                    //"attestation result message back that did "
                    //"NOT match the expected value.\n");
                    ////fprintf(OUTPUT, "\nEXPECTED ATTESTATION RESULT -");
                    //PRINT_BYTE_ARRAY(OUTPUT,
                    //  attestation_msg_samples[GET_VERIFICATION_ARRAY_INDEX()],
                    //p_att_result_msg_full->size);
                }
            }
        }

        ////fprintf(OUTPUT, "\nATTESTATION RESULT RECEIVED - ");
        //PRINT_BYTE_ARRAY(OUTPUT, p_att_result_msg_full->body,
        //                p_att_result_msg_full->size);

        if (VERIFICATION_INDEX_IS_VALID())
        {
            ////fprintf(OUTPUT, "\nBecause we used precomputed values for the "
            // "messages, the attestation result message will "
            //"not pass further verification tests, so we will "
            //"skip them.\n");
            goto CLEANUP;
        }

        // Check the MAC using MK on the attestation result message.
        // The format of the attestation result message is ISV specific.
        // This is a simple form for demonstration. In a real product,
        // the ISV may want to communicate more information.
        ret = verify_att_result_mac(enclave_id,
                                    &status,
                                    context,
                                    (uint8_t *)&p_att_result_msg_body->platform_info_blob,
                                    sizeof(ias_platform_info_blob_t),
                                    (uint8_t *)&p_att_result_msg_body->mac,
                                    sizeof(sgx_mac_t));
        if ((SGX_SUCCESS != ret) ||
            (SGX_SUCCESS != status))
        {
            ret = -1;
            ////fprintf(OUTPUT, "\nError: INTEGRITY FAILED - attestation result "
            //"message MK based cmac failed in [%s].",
            //__FUNCTION__);
            goto CLEANUP;
        }

        bool attestation_passed = true;
        // Check the attestation result for pass or fail.
        // Whether attestation passes or fails is a decision made by the ISV Server.
        // When the ISV server decides to trust the enclave, then it will return success.
        // When the ISV server decided to not trust the enclave, then it will return failure.
        if (0 != p_att_result_msg_full->status[0] || 0 != p_att_result_msg_full->status[1])
        {
            ////fprintf(OUTPUT, "\nError, attestation result message MK based cmac "
            //"failed in [%s].", __FUNCTION__);
            attestation_passed = false;
        }

        // The attestation result message should contain a field for the Platform
        // Info Blob (PIB).  The PIB is returned by attestation server in the attestation report.
        // It is not returned in all cases, but when it is, the ISV app
        // should pass it to the blob analysis API called sgx_report_attestation_status()
        // along with the trust decision from the ISV server.
        // The ISV application will take action based on the update_info.
        // returned in update_info by the API.
        // This call is stubbed out for the sample.
        //
        // sgx_update_info_bit_t update_info;
        // ret = sgx_report_attestation_status(
        //     &p_att_result_msg_body->platform_info_blob,
        //     attestation_passed ? 0 : 1, &update_info);

        // Get the shared secret sent by the server using SK (if attestation
        // passed)
        if (attestation_passed)
        {
            ret = put_secret_data(enclave_id,
                                  &status,
                                  context,
                                  p_att_result_msg_body->secret.payload,
                                  p_att_result_msg_body->secret.payload_size,
                                  p_att_result_msg_body->secret.payload_tag);
            if ((SGX_SUCCESS != ret) || (SGX_SUCCESS != status))
            {
                ////fprintf(OUTPUT, "\nError, attestation result message secret "
                //"using SK based AESGCM failed in [%s]. ret = "
                // "0x%0x. status = 0x%0x", __FUNCTION__, ret,
                // status);
                goto CLEANUP;
            }
        }
        ////fprintf(OUTPUT, "\nSecret successfully received from server.");
        ////fprintf(OUTPUT, "\nRemote attestation success!");
    }

CLEANUP:
    // Clean-up
    // Need to close the RA key state.
    if (INT_MAX != context)
    {
        int ret_save = ret;
        ret = enclave_ra_close(enclave_id, &status, context);
        if (SGX_SUCCESS != ret || status)
        {
            ret = -1;
            ////fprintf(OUTPUT, "\nError, call enclave_ra_close fail [%s].",
            //__FUNCTION__);
        }
        else
        {
            // enclave_ra_close was successful, let's restore the value that
            // led us to this point in the code.
            ret = ret_save;
        }
        ////fprintf(OUTPUT, "\nCall enclave_ra_close success.\n");
    }
    ra_free_network_response_buffer(p_msg0_resp_full);
    ra_free_network_response_buffer(p_msg2_full);
    ra_free_network_response_buffer(p_att_result_msg_full);

    // p_msg3 is malloc'd by the untrusted KE library. App needs to free.
    SAFE_FREE(p_msg3);
    SAFE_FREE(p_msg3_full);
    SAFE_FREE(p_msg1_full);
    SAFE_FREE(p_msg0_full);

    // enclave manager main code
    ////fprintf(stdout, "\n help");


    std::string code = "local_storage_data = {}; var x = 1; local_storage_data[\"test\"] = x;print(x);print(JSON.stringify(local_storage_data, undefined));";
    //std::string code = "var test = {\"b\":\"c\"}; var s = JSON.stringify(test, undefined); var t = eval(s); print(t['b']);";
    //printf("init enclave_id: %d\n", enclave_id);
    //std::string code = "print('starting'); update_form('loginform', 'password', 'pwd');var x=js_make_http_request('b', 'a', 'c', 'd');\n if(1) {print(x);};\n for (var i=1;i<10;i++){print(i);}\n var result=2;";
    //std::string code = "{ var a = 4; var b = 1; while (a>0) { b = b * 2; a = a - 1; } var c = 5; }";
    //std::string code = "x = 1; var y = 2;";
    //printf("init enclave_id: %d\n", enclave_id);
    //printf("testing %d\n", t);
    //printf("testing %d\n", t);
    //run_js(enclave_id, &re, (char*) &code[0], code.length()+1, (uint8_t*)&sig, sig_size);//should fail bc we have not generated a signiture
    //code = "print(str_data); var x = local_storage_data[\"text\"]; print(x);";
    //run_js(enclave_id, &re, (char*) &code[0], code.length()+1, (uint8_t*)&sig, sig_size);//should fail bc we have not generated a signiture

    //--------------------------end testing-----------------------------

    ex_timestamps << "attestation end," <<  absTime() << endl; 
}
