import sys
from serial import Serial

def run(portnum, nbytes = 10):
    port = "/dev/ttyACM%s" % portnum
    
    with Serial(port, 9600, timeout=2) as ser:
        print "Listening on port: %s" % port
        print "Reading %d bytes" % nbytes
        while(True):
            x = ser.read(nbytes)
            if len(x) > 0:
                print "Read: %s" % x
            
if __name__ == "__main__":
    argc = len(sys.argv)
    if argc == 2:
        run(int(sys.argv[1]))
    if argc > 2:
        run(int(sys.argv[1]), int(sys.argv[2]))
