#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include "btchannel.h"
#include <chrono>


/* Setting this to 1 will cause channel_open to scan discoverable bluetooth
 * devices for one with a name "raspberrypi". Setting this to 0 uses the
 * hardcoded bluetooth address defined as PI_ADDR */
#define BT_SCAN 0
#define PI_ADDR "00:1A:7D:DA:71:13" //B8:27:EB:1C:41:F8"
#define absTime() std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()

/* Bluetooth code modified from
 * https://people.csail.mit.edu/albert/bluez-intro/ */

/* Opens a bluetooth connection to the rapsberry pi.
 *
 * returns: 0 if successful, -1 otherwise
 */
std::ofstream log;
int packetNum = 0;

int set_l2cap_mtu( int sock, uint16_t mtu ) {
  struct l2cap_options opts;
    socklen_t optlen = sizeof(opts), err;
    err = getsockopt( sock, SOL_L2CAP, L2CAP_OPTIONS, &opts, &optlen );
    if( ! err ) {
        opts.omtu = opts.imtu = mtu;
        err = setsockopt( sock, SOL_L2CAP, L2CAP_OPTIONS, &opts, optlen );
    }
    return err;
};

BluetoothChannel::BluetoothChannel() {
  log.open("btlog.txt");
}

int BluetoothChannel::channel_open()
{  
  log << "trying to open channel " << absTime() << std::endl;
  inquiry_info *ii = NULL;
  int max_rsp, num_rsp, dev_id, len, flags, i;
  char rsp_addr[19] = { 0 };
  char rsp_name[248] = { 0 };
  char target[] = "raspberrypi";
  //struct sockaddr_rc addr = { 0 };
  struct sockaddr_l2 addr = { 0 };
#if BT_SCAN
  log << "bt scan set to 1" << std::endl;
  dev_id = hci_get_route(NULL);
  sock = hci_open_dev( dev_id );
  if (dev_id < 0 || sock < 0)
  {
    log << "failed to open socket" << std::endl;
    log.close();
    //perror("opening socket");
    return -1;
  }

  log << "sock created" << std::endl;

  len  = 8;
  max_rsp = 255;
  flags = IREQ_CACHE_FLUSH;
  ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
  log << "trying to run hci_inquiry " << dev_id << std::endl;
  num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
  log << "passed hci_inquiry" << std::endl;
  if( num_rsp < 0 ) {
    log << "failed on hci_inquiry" << std::endl;
    log.close();
    //perror("hci_inquiry");
  }
  

  log << "found " << num_rsp << " devices, searching for rpi..." << std::endl;
  for (i = 0; i < num_rsp; i++)
  {
    ba2str(&(ii+i)->bdaddr, rsp_addr);
    memset(rsp_name, 0, sizeof(rsp_name));

    log << "name: " <<  rsp_name << std::endl;
    if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(rsp_name), rsp_name, 0) < 0)
      strcpy(rsp_name, "[unknown]");

   //printf("%s  %s\n", rsp_addr, rsp_name);
    log << "checking the " << i << "th device: " << rsp_name << std::endl;
    if (strcmp(rsp_name, target) == 0)
    {
      close (sock);
      free (ii);
      sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
      addr.rc_family = AF_BLUETOOTH;
      addr.rc_channel = (uint8_t) 1;
      str2ba(rsp_addr, &addr.rc_bdaddr );
     //printf("Opening RFCOMM socket \n");
      if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        log << "failed connection" << std::endl;
        return -1;
      }
    log << "connect exited normally" << std::endl;
    log.close();
      return 0;
    }
  }
 //printf("Could not find the raspberry pi\n");
  log << "could not find the raspberry pi" << std::endl;
  log.close();
  return -1;
#else
  log << "bt scan set to 0" << std::endl;
  sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP); //SOCK_STREAM, BTPROTO_RFCOMM
  set_l2cap_mtu(sock, 65535);

  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm = htobs(0x1001);
  str2ba(PI_ADDR, &addr.l2_bdaddr );

  // addr.rc_family = AF_BLUETOOTH;
  // addr.rc_channel = (uint8_t) 1;
  // str2ba(PI_ADDR, &addr.rc_bdaddr );
 //printf("Opening RFCOMM socket \n");
  log << "opening socket" << absTime() << std::endl;
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
  {
   log << "failed to open socket" << std::endl;
    return -1;
  }
  log << "socket opened, returning normally" << absTime() << std::endl;
  return 0;
#endif
}

/* Sends data on the RFCOMM connection.
 *
 * returns: number of bytes sent, -1 if failed.
 */
int BluetoothChannel::channel_send(char * buffer, int len)
{
  int resp = write(sock, buffer, len);
 log << "send len: " << len << std::endl;
 log << packetNum << "th packet sent, write() returned " << resp << std::endl;
 packetNum++;
  if (resp == -1)
  {
   log << "Reopening connection." << std::endl;
    if (channel_open() == -1)
    {
     log << "Failed reopening connection" << std::endl;
      return -1;
    }
    else
    {
      log << "Reopened connection." << std::endl;
      int resp = write(sock, buffer, len);
      log << "Exp send len: " << len << std::endl;
      log << "Actual send len: " << resp << std::endl;
      return resp;
    }
  }
  return resp;
}

/* Closes the RFCOMM connection. */
void BluetoothChannel::channel_close()
{
  close(sock);
}
