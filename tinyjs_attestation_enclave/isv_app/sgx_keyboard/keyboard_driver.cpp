#include "keyboard_driver.h"

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

int KeyboardDriver::openPorts() {
  if ((_characterPortHandle = open(_characterPortName.c_str(), O_RDWR, 0666)) == -1) {
    perror(_characterPortName.c_str());
  }
  if ((_modePortHandle = open(_modePortName.c_str(), O_RDWR, 0666)) == -1) {
    perror(_modePortName.c_str());
  }
  return ((_characterPortHandle != -1) && (_modePortHandle != -1));
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

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(_characterPortHandle, &rfds);
  if(select(_characterPortHandle + 1, &rfds, NULL, NULL, NULL) < 0) {
    return -1;
  }

  if (FD_ISSET(_characterPortHandle, &rfds) || block) {
    // There is data to read on the relevant file descriptor.
    int bytes_read = read(_characterPortHandle, p_dst, len);
    return bytes_read; 
  }

  return 0;
}
