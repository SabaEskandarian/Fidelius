# Keyboard Driver
We provide a keyboard_driver component that can be used as an API by the enclave to comunicate with the Raspberry Pi via the serial ports opened by the pi (/dev/ttyACM0 and /dev/ttyACM1). More extensive documentation is available in "keyboard_driver.h".


# Permissions
Be sure to modify the device files "/dev/ttyACM0" and "/dev/ttyACM1" so that they can be read and written to by everyone:

~~~~
sudo chmod o+rw /dev/ttyACM0
sudo chmod o+rw /dev/ttyACM1
~~~~

# Build

To build use `make`
To build test `make test`

The test can be used similarly to the simple_reciever.py to listen on a serial port and print them on a screen.

# Usage 
Usage examples are in keyboard_driver_t.cpp