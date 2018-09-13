#include "keyboard_driver.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main (int argc, char * argv[]) {
  if (argc < 3) {
    printf("Usage: test <characterPortName> <modePortName>\n");
    return 0;
  }
  KeyboardDriver kb(argv[1], argv[2]);
  //KeyboardDriver kb("/dev/ttyS1", "/dev/ttyS3");

  // check write to next Mode
  uint8_t nextMode[2] = {0xff, 0xff};
  int retval = kb.changeMode(nextMode, sizeof nextMode);
  printf("Change Mode Write: Success(1), Fail(0): %d\n", retval);

  int count = 0;

  while(count < 3000) {
    count++;
    uint8_t buf[10];
    memset(buf, 0, sizeof buf);
    int input = kb.getEncryptedKeyboardInput(buf, 4, false);
    if(input > 0) {
      printf("Recieved Encrypted Input\n");
      printf("Received: %s\n", (char *) buf);
    }
  }

  return 0;
}
