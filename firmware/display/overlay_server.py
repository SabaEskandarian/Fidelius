#!/usr/bin/python2.7
import io
import time
import picamera
import StringIO
import binascii
import sys
import logging
import signal
import socket
import os
import pickle
import time
import argparse
from enum import Enum
from struct import *
from PIL import Image

from tk import *

ORIGIN_Y = 693
ORIGIN_X = 80
INPUT_X = 500
NO_GREEN_BAR = False

__SIZE = (
        ((1280 +31) // 32) * 32,
        ((720+15) // 16) * 16
        )
__LAYOUT = Image.open("layout{}x{}.png".format(__SIZE[0], __SIZE[1]))

def get_conn_comm():
    if os.path.exists(server_addr):
        os.unlink(server_addr)
        
    srv_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv_sock.bind(server_addr)
    srv_sock.listen(0)
    logging.info('UDS server ready.')

    conn, _ = srv_sock.accept()
    logging.info('Accepted connection.')
    return srv_sock, conn

def create_pad():
    pad = __LAYOUT.copy()
    return pad

def main(argobj):
    render_times = open("render_times.csv", 'w+')
    pickled_overlays = open("pickled_overlays.txt", "w+")
    camera = None
    srv_sock, conn = None, None
    def gentle_shutdown(*args):
        if camera:
            camera.close()
        if conn:
            conn.close()
            
        sys.exit(0)

    signal.signal(signal.SIGINT, gentle_shutdown)
    
    logging.basicConfig(format='%(asctime)s %(message)s', stream=sys.stdout, level=logging.DEBUG)

    if argobj.overlay_only:
        logging.info("Showing naked overlay")
        pad = create_pad()
        pad.show()
        sys.exit(1)

    try:
        last = None
        overlays = {}
        
        camera = picamera.PiCamera()
        camera.start_preview()
        logging.info('Preview started')

        srv_sock, conn = get_conn_comm()
        
        while True:
            
            try:
                #logging.info("trying to read from sock")
                #lines = conn.makefile().readlines()
                #data = "".join([line + '\n' for line in lines])
                #pickled_overlays.write(data)
                overlays = pickle.load(conn.makefile())
                logging.info('Unpickled {} objects'.format(len(overlays)))
            except EOFError as e:
                pickled_overlays.close()
                logging.info("Com server died?")
                overlays = {}

                try:
                    conn.close()
                except Exception as e:
                    logging.debug("Closing connection after EOF did not succeed")

                try:
                    srv_sock.close()
                except Exception as e:
                    logging.debug("Closing server connection after EOF did not succeed")

                # reconnect
                srv_sock, conn = get_conn_comm()

            
            
            # Create a new canvas for overlays
            
            pad = create_pad()
            if NO_GREEN_BAR or len(overlays) == 0:
                pad = Image.new('RGBA', (
                    ((1280 +31) // 32) * 32,
                    ((720 +15) // 16) * 16,
                    ))

            inBuff = ""
            count = 0
            # Paste all our overlays onto the pad
            for form in overlays.itervalues():
                count += 1
                inBuff = form.inputBuff
                render_times.write("bitmap render: {},%d\n".format(form.inputBuff) % (int(time.time()*1000)))
                pad.paste(form.img, (ORIGIN_X, ORIGIN_Y))
		pad.paste(form.img2, (INPUT_X, ORIGIN_Y))
                for field in form.fields:
                    count += 1
                    pad.paste(field.img, (field.x+7, field.y-3))
            print("total number of forms + inputs: %d"%(count))
            # Create a new overlay to add to the screen
            new_overlay = camera.add_overlay(pad.tobytes(), size = pad.size)
            render_times.write("bitmap display: {},%d\n".format(inBuff) % (int(time.time()*1000)))
            render_times.flush();
            # Renders the overlay above the camera preview layer
            new_overlay.layer = 3

            if last:
                # Push the last layer underneath the preview then remove it
                last.layer = 0
                camera.remove_overlay(last)
            last = new_overlay
    except Exception as e:
        pickled_overlays.close()
        logging.exception(e)
        gentle_shutdown()

def parse_args():
    parser = argparse.ArgumentParser(description='Display server.')
    parser.add_argument('--overlay-only', action='store_true', help='Does not open and read from HDMI IN')
    return parser.parse_args()

if __name__ == "__main__":

    argobj = parse_args()
    main(argobj)
        
