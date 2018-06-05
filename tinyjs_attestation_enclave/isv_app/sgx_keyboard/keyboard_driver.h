#ifndef INCLUDED_PI_KEYBOARD_DRIVER_H
#define INCLUDED_PI_KEYBOARD_DRIVER_H

#include <string>
//#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

class  KeyboardDriver
{
  
 private:
  
  // Absolute path names to open serial ports
  std::string _characterPortName;
  std::string _modePortName;
  
  // Handles for open serial ports
  int _characterPortHandle;
  int _modePortHandle;
  
  // baud rate default to 9600
  //  int _baud;
  
  // Modifiers
  void setAttributes(int);
  int openPorts();
  void closePorts();
  
 public:
  
  KeyboardDriver(std::string characterPortName, std::string modePortName);
  ~KeyboardDriver();
  
  // Sends 'len' bytes at the destination 'p_src' to the keyboard.
  // 'p_src' should point to a string produced by AES_GCM.
  // The keyboard tracks messages sent by this function so that a
  // "replay message" has no effect on the keyboard.
  // Return 1 on write success, 0 otherwise.
  //
  // A "replay message" is a sequence of 'len' bytes that was already
  // to the keyboard on the same port that this function uses. 
  int changeMode(uint8_t * p_src, int len);
  
  // Optional blocking call to retrieve next encrypted input. 
  // Puts the encrypted ascii character into the buffer pointed to
  // by the specified "p_src".
  // Return the number of bytes read into "p_dst".
  // Return a value less than 0 on error. errno is set by 'read'
  //
  // NEED to change select()
  int getEncryptedKeyboardInput(uint8_t * p_dst, int len, bool block);
  
};

#endif
