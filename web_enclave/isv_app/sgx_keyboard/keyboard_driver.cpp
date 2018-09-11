#include "keyboard_driver.h"
#include <termios.h>
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <string.h>

KeyboardDriver::KeyboardDriver(std::string characterPortName, std::string modePortName)
  : _characterPortName(characterPortName)
  , _modePortName(modePortName)
  , _characterPortHandle(-1)
  , _modePortHandle(-1)
    //  , _baud(9600)
{
  openPorts();
}

KeyboardDriver::~KeyboardDriver()
{
  closePorts();
}

void KeyboardDriver::setAttributes(int handle){
  // https://stackoverflow.com/questions/18108932/linux-c-serial-port-reading-writing
  
  struct termios tty;
  memset (&tty, 0, sizeof tty);

  // Error Handling
  if ( tcgetattr ( handle, &tty ) != 0 ) {
    std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
  }

  // Set Baud Rate
  cfsetospeed (&tty, (speed_t)B9600);
  cfsetispeed (&tty, (speed_t)B9600);

  // Setting other Port Stuff
  tty.c_cflag     &=  ~CSTOPB;
  tty.c_cflag     &=  ~CRTSCTS;           // no flow control
  tty.c_cc[VMIN]   =  1;                  // read doesn't block
  tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
  tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

  // Make raw 
  cfmakeraw(&tty);

  // Flush Port, then applies attributes
  tcflush( handle, TCIFLUSH );
  if ( tcsetattr ( handle, TCSANOW, &tty ) != 0) {
    std::cerr << "Error " << errno << " from tcsetattr" << std::endl;
  }
}

int KeyboardDriver::openPorts() {
  if ((_characterPortHandle = open(_characterPortName.c_str(), O_RDWR | O_NOCTTY , 0666)) == -1) {
    perror(_characterPortName.c_str());
  }
  if ((_modePortHandle = open(_modePortName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0666)) == -1) {
    perror(_modePortName.c_str());
  }
  if ((_characterPortHandle != -1) && (_modePortHandle != -1)) {
    setAttributes(_characterPortHandle);
    setAttributes(_modePortHandle);
    return 1;
  } else {
    return 0;
  }
}


void KeyboardDriver::closePorts() {

  // Close open ports?
  if(_characterPortHandle >=0)
    close(_characterPortHandle);
  if(_modePortHandle >=0)
    close(_modePortHandle);
  
  _characterPortHandle = -1;
  _modePortHandle = -1;

}

int KeyboardDriver::changeMode(uint8_t * p_src, int len) {
  int bytes_written = write(_modePortHandle, p_src, len);
  return (bytes_written == len);
}

int KeyboardDriver::getEncryptedKeyboardInput(uint8_t * p_dst, int len, bool block) {

  // Set up select function to check if there is data to read
  //   struct timeval {
  //     long    tv_sec;         /* seconds */
  //     long    tv_usec;        /* microseconds */
  //   };

  struct timeval t;
  t.tv_sec = 0;
  t.tv_usec = 10000;
  
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(_characterPortHandle, &rfds);
  int fds_ready = select(_characterPortHandle + 1, &rfds, NULL, NULL, &t);
  if (fds_ready <= 0) {
    return -20;
  }
  
  int bytes_read = 0;
  if (FD_ISSET(_characterPortHandle, &rfds) || block) {
     // There is data to read on the relevant file descriptor.
     bytes_read = read(_characterPortHandle, p_dst, len);
     if (bytes_read < 0) {
        fprintf(stderr, "%s",strerror(errno));
     }
     return bytes_read; 
  }

  // while (bytes_read < len) {
  //   int to_read = len - bytes_read;
  //   bytes_read += read(_characterPortHandle, p_dst + bytes_read, to_read);
  // }

  return bytes_read;
  
}
