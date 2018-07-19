#!/usr/bin/python2.7
import sys
import socket
import pickle
from tk import *
if __name__ == "__main__":
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(server_addr)
    
    sock.send(pickle.dumps({}))
    sock.close()
        
