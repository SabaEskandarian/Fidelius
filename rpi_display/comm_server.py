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

from tk import *

secret_key = '24a3e5ad48a7a6b198fe35fbe16c6685'

BASE_HDR_LEN = 4
ADD_OVERLAY_HDR_LEN = 16
REMOVE_OVERLAY_HDR_LEN = 12
TOKEN_LEN = 8
TAG_LEN = 16
IV_LEN = 12

# Op codes (byte 3)
OP_ADD_OVERLAY = 1
OP_REMOVE_OVERLAY = 2
OP_CLEAR_OVERLAYS = 3



def init_bluetooth():
    server_sock=BluetoothSocket( RFCOMM )
    server_sock.bind(('',PORT_ANY))
    server_sock.listen(1)
    port = server_sock.getsockname()[1]
    logging.debug('Waiting for connection on RFCOMM channel {}'.format(port))
    client_sock, client_info = server_sock.accept()
    logging.debug('Accepted connection from {}'.format(client_info))
    return server_sock, client_sock

def get_data_from_client():
    buffer = []
    buffer = client_sock.recv(1024)
    outputFile.write(bytearray(buffer))
    return buffer



def receive_bluetooth():
    buffer = []
    while True:
        #logging.debug('Waiting for message')
        if len(buffer) < 4:
            buffer = get_data_from_client()
           #  logging.debug(str(len(buffer)))
                        
        # Read the first word of the message
        seq_no, op, o_id = unpack('!HBB', buffer[0:BASE_HDR_LEN])
        #logging.debug('Received packet: {:5}'.format(seq_no))
        #logging.debug('Op code: {} Overlay ID: {}'.format(op, o_id))
        if op == OP_ADD_OVERLAY:
            if len(buffer) < ADD_OVERLAY_HDR_LEN:
                return None
            else:
                msg_len = unpack('!I', buffer[ADD_OVERLAY_HDR_LEN-4:ADD_OVERLAY_HDR_LEN])[0]
                # Read the rest of the messgae into the buffer
                # logging.debug(str(len(buffer)))
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
        aad = msg[:ADD_OVERLAY_HDR_LEN]
        ct = msg[ADD_OVERLAY_HDR_LEN:-IV_LEN];
        nonce = msg[-IV_LEN:]
        #logging.debug('aad: ' + aad)
        #logging.debug('ct: ' + ct)
    else:
        aad = msg[:REMOVE_OVERLAY_HDR_LEN]
        ct = msg[REMOVE_OVERLAY_HDR_LEN:REMOVE_OVERLAY_HDR_LEN+TAG_LEN]
        nonce = msg[REMOVE_OVERLAY_HDR_LEN+TAG_LEN:]
        #logging.debug('aad: ' + aad)
        #logging.debug('ct: ' + ct)

    
    aesgcm = AESGCM (binascii.unhexlify(secret_key))
    try:
        plaintext = aesgcm.decrypt (nonce ,ct, None)
        buf = buffer(aad + bytearray(plaintext))
        logging.debug('Received packet: {:5}, decryption passed'.format(seq_no))
    except:
        buf = None
        logging.debug('Received packet: {:5}, decryption failed'.format(seq_no))
    return buf

def parse_message(msg, overlays):
    seq_no, op, o_id = unpack('!HBB', msg[0:BASE_HDR_LEN])
    if op == OP_ADD_OVERLAY:
        # Add the origin bitmap
        field_iter = ADD_OVERLAY_HDR_LEN
        x, y, width, height = unpack('!HHHH', msg[field_iter:field_iter+8])
        img = Image.frombytes('RGBA', (width, height), msg[field_iter+8:])
        form = Form(o_id, x, y, img)
        field_iter += width * height * 4 + 8;
        # Add the field input bitmaps
        while(field_iter < len(msg)):
            x, y, width, height = unpack('!HHHH', msg[field_iter:field_iter+8])
            x += form.x
            y += form.y
            img = Image.frombytes('RGBA', (width, height), msg[field_iter:])
            form.fields.append(Field(x, y, img))
            # Increase our iterator by the number of bytes used for this field
            field_iter += width * height * 4 + 8;
        overlays[o_id] = form
    elif op == OP_REMOVE_OVERLAY:
        del overlays[o_id]
    elif op == OP_CLEAR_OVERLAYS:
        overlays.clear()

if __name__ == "__main__":
    numPackets = 0
    server_sock = None
    client_sock = None
    outputFile = open('socketData', 'w+')
    display_times = open('display_times.csv', 'w+')

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
        logging.debug('Bluetooth starting')
        server_sock, client_sock = init_bluetooth()
        logging.debug('Bluetooth connected')

        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(server_addr)
        logging.debug('Connected to overlay server')
        
        overlays = {}

        while True:
            # Retrive a message from the data channel
            try:
                msg = receive_bluetooth()
                
            except BluetoothError:
                client_sock.close()
                server_sock.close()
                server_sock, client_sock = init_bluetooth()
                logging.debug('Blueooth connection lost')
            if not msg:
                logging.debug('Message malformed/message error')
                continue

            # Decrypt and check the tag
            display_times.write('bitmap decryption,%.9f\n' % (time.time()*1000))
            msg = decrypt_message (msg)
            if not msg:
                #logging.debug('Decryption failed')
                continue
            #logging.debug('Decryption passed')
            # Parse the message
            parse_message(msg, overlays)
            data = pickle.dumps(overlays)
            sock.send(data)
            #logging.debug('Sending overlays {}'.format(len(data)))
            
    except Exception as e:
        logging.exception(e)
        gentle_shutdown()
        
