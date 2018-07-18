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
from enum import Enum
from struct import *
from PIL import Image

from tk import *


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

if __name__ == "__main__":

    render_times = open("render_times.csv", 'w+')
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



    try:
        last = None
        overlays = {}
        
        camera = picamera.PiCamera()
        camera.start_preview()
        logging.info('Preview started')

        srv_sock, conn = get_conn_comm()
        
        while True:
            
            try:
                overlays = pickle.load(conn.makefile())
                logging.info('Unpickled {} objects'.format(len(overlays)))
            except EOFError as e:
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
            render_times.write("rendering overlay,%.9f\n" % (time.time()*1000))
            pad = Image.new('RGBA', (
                ((1280 +31) // 32) * 32,
                ((720 +15) // 16) * 16,
                ))

            # Paste all our overlays onto the pad
            for form in overlays.itervalues():
                pad.paste(form.img, (0, 0))
                for field in form.fields:
                    pad.paste(field.img, (field.x+7, field.y-3))

            # Create a new overlay to add to the screen
            new_overlay = camera.add_overlay(pad.tobytes(), size = pad.size)
            # Renders the overlay above the camera preview layer
            new_overlay.layer = 3

            if last:
                # Push the last layer underneath the preview then remove it
                last.layer = 0
                camera.remove_overlay(last)
            last = new_overlay
    except Exception as e:
        logging.exception(e)
        gentle_shutdown()
        
