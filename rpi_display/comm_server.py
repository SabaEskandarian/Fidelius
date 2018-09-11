#!/usr/bin/python2.7
import io
import time
import StringIO
import binascii
import sys
import logging
import signal
import socket
import pickle
from enum import Enum
from struct import *
from PIL import Image
from bluetooth import *
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from bitarray import bitarray
from subprocess import call
import decode
import os
import RPi.GPIO as GPIO

from tk import *

secret_key = '24a3e5ad48a7a6b198fe35fbe16c6685'

BASE_HDR_LEN = 4
ADD_OVERLAY_HDR_LEN = 16
REMOVE_OVERLAY_HDR_LEN = 12
TOKEN_LEN = 8
TAG_LEN = 16
IV_LEN = 12
BUFFER_LEN = 200

# Op codes (byte 3)
OP_ADD_OVERLAY = 1
OP_REMOVE_OVERLAY = 2
OP_CLEAR_OVERLAYS = 3




def init_bluetooth():
    server_sock=BluetoothSocket( L2CAP ) #RFCOMM )
    set_l2cap_mtu( server_sock, 65535 )
    server_sock.bind(('',PORT_ANY))
    server_sock.listen(1)
    port = server_sock.getsockname()[1]
    logging.debug('Waiting for connection on RFCOMM channel {}'.format(port))
    client_sock, client_info = server_sock.accept()
    logging.debug('Accepted connection from {}'.format(client_info))
    return server_sock, client_sock

def get_data_from_client():
    buffer = []
    buffer = client_sock.recv(65535)
    outputFile.write(bytearray(buffer))
    return buffer



def receive_bluetooth():
    buffer = []
    while True:
        if len(buffer) < 4:
            buffer = get_data_from_client()
        seq_no, op, o_id = unpack('!HBB', buffer[0:BASE_HDR_LEN])
        if op == OP_ADD_OVERLAY:
            if len(buffer) < ADD_OVERLAY_HDR_LEN:
                return None
            else:
                msg_len = unpack('!I', buffer[ADD_OVERLAY_HDR_LEN-4:ADD_OVERLAY_HDR_LEN])[0]
                while (msg_len > (len(buffer))):
                    size = len(buffer)
                    buffer += get_data_from_client()
                return buffer[0:msg_len]

        elif op == OP_REMOVE_OVERLAY:
            if len(buffer) < REMOVE_OVERLAY_HDR_LEN + TAG_LEN + IV_LEN:
                return None
            else:
                return buffer[0:REMOVE_OVERLAY_HDR_LEN+TAG_LEN+IV_LEN]

        elif op == OP_CLEAR_OVERLAYS:
            if len(buffer) < REMOVE_OVERLAY_HDR_LEN + TAG_LEN + IV_LEN:
                return None
            else:
                return buffer[0:REMOVE_OVERLAY_HDR_LEN+TAG_LEN+IV_LEN]

        else:
            return None


def decrypt_message (msg):
    seq_no, op, o_id = unpack('!HBB', msg[0:BASE_HDR_LEN])
    if op == OP_ADD_OVERLAY:
        inputVal = unpack('c'*BUFFER_LEN, msg[ADD_OVERLAY_HDR_LEN:ADD_OVERLAY_HDR_LEN+BUFFER_LEN])
        aad = msg[:ADD_OVERLAY_HDR_LEN]
        ct = msg[ADD_OVERLAY_HDR_LEN+BUFFER_LEN:-IV_LEN];
        nonce = msg[-IV_LEN:]
        aesgcm = AESGCM (binascii.unhexlify(secret_key))
        try:
            plaintext = aesgcm.decrypt (nonce ,ct, None)
            buf = buffer(aad + bytearray(inputVal) + bytearray(plaintext))
            logging.debug('Received packet: {:5}, decryption passed'.format(seq_no))
            display_times.write('bm dec: {},%d\n'.format("".join(inputVal)) % (int(time.time()*1000)))
        except:
            buf = None
            logging.debug('Received packet: {:5}, decryption failed'.format(seq_no))
        GPIO.output(18, True)
    else:
	try:
	    GPIO.output(18, False)
	except:
	    print("caught gpio exception")
        buf = None
    return buf

def bits2byte(bits8):
    result = 0;
    for bit in bits8:
        result <<=1
        result |= bit
    return result

def byteToBits(byte):
    ret = [1 if digit=='1' else 0 for digit in bin(byte)[2:]]
    while(len(ret) < 8):
        ret.append(0)
    return ret

def numBytesToEncodePixels(num):
    if num%8 == 0:
        return num/8
    return num/8 + 1;

def parse_message(msg, overlays):
    seq_no, op, o_id = unpack('!HBB', msg[0:BASE_HDR_LEN])
    if op == OP_ADD_OVERLAY:
        logging.debug('decode bm,%d\n'% (int(time.time()*1000)))
        inputVal = unpack('c'*BUFFER_LEN, msg[ADD_OVERLAY_HDR_LEN:ADD_OVERLAY_HDR_LEN+BUFFER_LEN])
        field_iter = ADD_OVERLAY_HDR_LEN + BUFFER_LEN

        x, y, width, height = unpack('!HHHH', msg[field_iter:field_iter+8])
        encodedstring = "".join(map(lambda e: chr(ord(e)),msg[field_iter+8:field_iter+8+numBytesToEncodePixels(width*height)]))
        data = decode.system(encodedstring, 1)
        #logging.debug('draw bm,%d\n'% (int(time.time()*1000)))
        origin_img = Image.frombytes('RGBA', (width, height), data)
        field_iter += numBytesToEncodePixels(width*height) + 8

        print("adding input bm")
        x, y, width, height = unpack('!HHHH', msg[field_iter:field_iter+8])
        encodedstring = "".join(map(lambda e: chr(ord(e)),msg[field_iter+8:field_iter+8+numBytesToEncodePixels(width*height)]))
        data = decode.system(encodedstring, 1)
        #logging.debug('draw bm,%d\n'% (int(time.time()*1000)))
        input_img = Image.frombytes('RGBA', (width, height), data)
        field_iter += numBytesToEncodePixels(width*height) + 8

        form = Form(o_id, x, y, origin_img, input_img)
        form.inputBuff = "".join(inputVal)
        


        # Add the field input bitmaps
        while(field_iter < len(msg)):
            #logging.debug('decode bm,%d\n'% (int(time.time()*1000)))
            x, y, width, height = unpack('!HHHH', msg[field_iter:field_iter+8])
            x += form.x
            y += form.y            
            encodedstring = "".join(map(lambda e: chr(ord(e)),msg[field_iter+8:field_iter+8+numBytesToEncodePixels(width*height)]))
            data = decode.system(encodedstring, 0)
            img = Image.frombytes('RGBA', (width, height), (data))
            form.fields.append(Field(x, y, img))
            # Increase our iterator by the number of bytes used for this field
            field_iter += numBytesToEncodePixels(width*height) + 8;
            
        overlays[o_id] = form
    elif op == OP_REMOVE_OVERLAY:
        del overlays[o_id]
    elif op == OP_CLEAR_OVERLAYS:
        overlays.clear()
    return op

if __name__ == "__main__":
    numPackets = 0
    server_sock = None
    client_sock = None
    outputFile = open('socketData', 'w+')
    encodingFile = open('encoding.bin', 'w+')
    display_times = open('display_times.csv', 'w+')
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(18,GPIO.OUT)
    GPIO.output(18, False)

    def gentle_shutdown(*args):
        if client_sock:
            client_sock.close()
        if server_sock:
            server_sock.close()
        outputFile.close()
        sys.exit(0)


    signal.signal(signal.SIGINT, gentle_shutdown)
    
    logging.basicConfig(format='%(asctime)s %(message)s', stream=sys.stdout, level=logging.DEBUG)

    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(server_addr)
        logging.debug('Connected to overlay server')
        
        logging.debug('Bluetooth starting')
        server_sock, client_sock = init_bluetooth()
        logging.debug('Bluetooth connected')


        overlays = {}

        while True:
            # Retrive a message from the data channel
            try:
                msg = receive_bluetooth()
                
            except:
                client_sock.close()
                server_sock.close()
                logging.debug('Blueooth connection lost')
                logging.debug('Bluetooth starting')
                server_sock, client_sock = init_bluetooth()
                logging.debug('Bluetooth connected')
                overlays = {}
                continue;
            if not msg:
                logging.debug('Message malformed/message error')
                continue
            msg = decrypt_message (msg)
            if not msg:
                sock.send(pickle.dumps({}, -1))    
                continue
            #logging.debug('Decryption passed')
            # Parse the message
            op = parse_message(msg, overlays)
            #logging.debug("pickling overlay")
            #display_times.write('bm pickle: {},%d\n'.format("".join(inputVal)) % (int(time.time()*1000)))
            #display_times.flush()
	    if op == OP_ADD_OVERLAY:
		print("sending an overlay")
                data = pickle.dumps(overlays, -1)
        	sock.send(data)
	    else:
		print("clearing overlay")
		sock.send(pickle.dumps({}, -1))    
    except Exception as e:
        logging.exception(e)
        gentle_shutdown()
        
