#include <iostream>
#include <fstream>
#include <curses.h>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <keycodes.h>
#include <hid_translate.h>
#include <termios.h>
#include <aes_encrypt.h>


// Set z as the filler character that is filtered to prevent
// timing attacks 
#define FILLER 'z'
#define timeNow() std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

std::queue<int> secure_inputs;
std::mutex secure_inputs_mutex;
std::mutex mode_mutex;
bool KEYBOARD_SECURE_MODE = 0;

std::ofstream keyb_times;

// Resets terminal to original settings
void endCurses()
{
  nocbreak();
  echo();
  keypad(stdscr, 0);
  endwin();
}

void startCurses() {
  initscr();          // init window
  cbreak();           // do not wait for enter to accept input
  noecho();           // do not echo input to screen
  keypad(stdscr, 1);  // kaypad mode treats delete, arrow keys as input (KEY_LEFT, etc.)

  // register a function to be called at exit
  atexit(endCurses);
}

void echoToWindow(int ch)
{
  addch(ch);
}

// Send a secure input every 1 seconds. 
// If there is no secure input waiting to be sent then send a dummy input.
// 
//
//



void secureSend(int secure_fd) {
  int iv_count = 0;
  while (TRUE) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::lock_guard<std::mutex> guard(secure_inputs_mutex);

    int ch = FILLER;
    if (!secure_inputs.empty()) {
      ch = secure_inputs.front();
      //echoToWindow(ch);
      secure_inputs.pop();
      if (ch == 0) exit(0);
    } else {

      // Dummy input
      // echoToWindow('E');
    }
    if (ch != FILLER) {
	std::string msg = "key encryption: ";
	msg += ch;
        keyb_times << msg << "," << timeNow() << std::endl;
    }
    std::string cyphertext = next_encrypt(ch,iv_count);
    iv_count++;
    if (ch != FILLER) {
	std::string msg = "USB sent key: ";
	msg += ch;
        keyb_times << msg << "," << timeNow() << std::endl;
    }
    write(secure_fd, cyphertext.c_str(), 58);
    //std::cout << cyphertext << std::endl;
    
  }
}


// 
//
//
void changeModeReceive(int changeMode_fd) {

  while (TRUE) {    
    //std::lock_guard<std::mutex> guard(mode_mutex);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(changeMode_fd , &rfds);
    select(changeMode_fd+1, &rfds, NULL, NULL, NULL);
    int bytes_read = 0;
    unsigned char mode;
    if (FD_ISSET(changeMode_fd, &rfds)) {

      bytes_read = read(changeMode_fd, &mode, 1);
      if(bytes_read != 1) {
        std::cout << "bad read call" << std::endl;
      } else {
	//unsigned char old_mode = mode;
        mode_mutex.lock();
	if (mode) {
	  system("./turn_on_led.py");
	  std::cout << "secure mode" << std::endl;
	}
	if (!mode) {
	  system("./turn_off_led.py");
	  std::cout << "insecure mode" << std::endl;
	}
	unsigned char old_mode = KEYBOARD_SECURE_MODE;
	KEYBOARD_SECURE_MODE = mode;
	mode_mutex.unlock();
	if (KEYBOARD_SECURE_MODE && !old_mode) sleep(3); //enforces a sleep if we are turning the secure mode off
      }
    }
   // sleep(3);
  }
}


// Send the specified character 'ch' to the the appropriate device
// based on the value of SECURE_KEYBOARD_MODE.
// If SECURE_KEYBOARD_MODE is TRUE, then 'ch' is added to the 'secure_inputs'
// queue to be encrypted. Otherwise, 'ch' is sent to the keyboard hid device.
void processInput(int fd, const char* filename, int ch)
{
  echoToWindow(ch);
  mode_mutex.lock();
  if (!KEYBOARD_SECURE_MODE) {
    if (ch == k_DELETE) {
      //echoToWindow('d');
    } else {
      std::cout << send_key(fd, filename, ch) << std::endl;
    }
  } else {
    std::lock_guard<std::mutex> guard(secure_inputs_mutex);
    secure_inputs.push(ch);
  }
  mode_mutex.unlock();
}

void setAttributes(int handle){
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


int getPortInput(int fd, uint8_t * p_dst, int len) {

  struct timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;
  
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  int fds_ready = select(fd + 1, &rfds, NULL, NULL, &t);
  if (fds_ready <= 0) {
    return fds_ready;
  }
  
  int bytes_read = 0;
  if (FD_ISSET(fd, &rfds)) {
    // There is data to read on the relevant file descriptor.
    bytes_read = read(fd, p_dst, len);
    return bytes_read; 
  }
  return bytes_read;
}


// Return file descriptor for the device we are writing to.
// "filename" is either "/dev/hidg0" or "/dev/tty###"
int openPort(const char * filename) {
  int fd;
  if ((fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY, 0666)) == -1) { 
    perror(filename);
  }
  return fd;
}

int main(int argc, char * argv[]) {  
  keyb_times.open("keyb_times.csv");
  const char * HIDName = "/dev/hidg0";
  const char * secureName = "/dev/ttyGS0";
  const char * changeModeName = "/dev/ttyGS1";
  system("./turn_off_led.py");

//   if (argc < 3) {
//     fprintf(stderr, "Usage: %s keyboardname secDevname\n", argv[0]);
//     return 1;
//   }
  
//  if (argc > 1) {
//          
//  }
//  if (argc > 2) {
//    secureName = argv[2];
//  }
//  if (argc > 3) {
//    changeModeName = argv[3];
//  }

  // Required init and exit register
  startCurses();

  int hid_fd = openPort(HIDName);
  int secure_fd = openPort(secureName);
  int changeMode_fd = openPort(changeModeName);

  setAttributes(changeMode_fd);

  std::thread secure_send_thread(secureSend, secure_fd);  
  std::thread change_mode_thread(changeModeReceive, changeMode_fd);

  int ch;
  if (argc > 1) {
     //if (strcmp(argv[1], "test1") == 0) {
	sleep(2);
        for (int i = 0; i < 200; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          keyb_times << "keypress: a," << timeNow() << std::endl;
          processInput(hid_fd, HIDName, 'a');
	  std::cout << "a" << std::endl;
        }
	std::cout << "done" << std::endl;
     //}
  } else {
    while(TRUE) {
      ch = getch();
      std::string msg = "keypress: ";
      msg += ch;
      keyb_times << msg << "," << timeNow() << std::endl;
      processInput(hid_fd, HIDName, ch);
    }
  }
  secure_send_thread.join();
  change_mode_thread.join();

  return 0;
} 
