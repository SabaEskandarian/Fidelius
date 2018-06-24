import io
import time
import picamera
import StringIO
import binascii
import sys
import logging
from enum import Enum
from struct import *
from PIL import Image
from bluetooth import *
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

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

class Form:
    def __init__(self, id, x, y, img):
        self.id = id
        self.x = x
        self.y = y
        self.img = img
        self.fields = []

class Field:
    def __init__(self, x, y, img):
        self.x = x
        self.y = y
        self.img = img

def init_bluetooth():
    server_sock=BluetoothSocket( RFCOMM )
    server_sock.bind(('',PORT_ANY))
    server_sock.listen(1)
    port = server_sock.getsockname()[1]
    logging.debug('Waiting for connection on RFCOMM channel {}'.format(port))
    client_sock, client_info = server_sock.accept()
    logging.debug('Accepted connection from {}'.format(client_info))
    return server_sock, client_sock

def receive_bluetooth():
    buffer = []
    while True:
        logging.debug('Waiting for message')
        if len(buffer) < 4:
            buffer = client_sock.recv(1024)
        # Read the first word of the message
        seq_no, op, o_id = unpack('!HBB', buffer[0:BASE_HDR_LEN])
        logging.debug('Received message. Sequence number: {}'.format(seq_no))
        logging.debug('Op code: {} Overlay ID: {}'.format(op, o_id))
        if op == OP_ADD_OVERLAY:
            if len(buffer) < ADD_OVERLAY_HDR_LEN:
                return None
            else:
                msg_len = unpack('!I', buffer[ADD_OVERLAY_HDR_LEN-4:ADD_OVERLAY_HDR_LEN])[0]
                # Read the rest of the messgae into the buffer
                while (msg_len > (len(buffer))):
                    size = len(buffer)
                    buffer += client_sock.recv(1024)
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
    else:
        aad = msg[:REMOVE_OVERLAY_HDR_LEN]
        ct = msg[REMOVE_OVERLAY_HDR_LEN:REMOVE_OVERLAY_HDR_LEN+TAG_LEN]
        nonce = msg[REMOVE_OVERLAY_HDR_LEN+TAG_LEN:]

    aesgcm = AESGCM (binascii.unhexlify(secret_key))
    try:
        plaintext = aesgcm.decrypt (nonce ,ct, aad)
        buf = buffer(aad + bytearray(plaintext))
    except:
        buf = None
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
    logging.basicConfig(format='%(asctime)s %(message)s', stream=sys.stdout, level=logging.DEBUG)
    with picamera.PiCamera() as camera:
        camera.start_preview()
        logging.debug('Bluetooth starting')
        server_sock, client_sock = init_bluetooth()
        logging.debug('Bluetooth connected')
        last = None
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
            msg = decrypt_message (msg)
            if not msg:
                logging.debug('Decryption failed')
                continue

            # Parse the message
            parse_message(msg, overlays)

            # Create a new canvas for overlays
            pad = Image.new('RGBA', (
                ((1920 +31) // 32) * 32,
                ((1080 +15) // 16) * 16,
                ))

            # Paste all our overlays onto the pad
            for form in overlays.itervalues():
                pad.paste(form.img, (form.x, form.y))
                for field in form.fields:
                    pad.paste(field.img, (field.x, field.y))

            # Create a new overlay to add to the screen
            new_overlay = camera.add_overlay(pad.tobytes(), size = pad.size)
            # Renders the overlay above the camera preview layer
            new_overlay.layer = 3

            if last:
                # Push the last layer underneath the preview then remove it
                last.layer = 0
                camera.remove_overlay(last)
            last = new_overlay

