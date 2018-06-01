#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include "btchannel.h"

/* Setting this to 1 will cause channel_open to scan discoverable bluetooth
 * devices for one with a name "raspberrypi". Setting this to 0 uses the
 * hardcoded bluetooth address defined as PI_ADDR */
#define BT_SCAN 0
#define PI_ADDR "B8:27:EB:24:1F:0E"

/* Bluetooth code modified from
 * https://people.csail.mit.edu/albert/bluez-intro/ */

/* Opens a bluetooth connection to the rapsberry pi.
 *
 * returns: 0 if successful, -1 otherwise
 */
int BluetoothChannel::channel_open()
{
  inquiry_info *ii = NULL;
  int max_rsp, num_rsp, dev_id, len, flags, i;
  char rsp_addr[19] = { 0 };
  char rsp_name[248] = { 0 };
  char target[] = "raspberrypi";
  struct sockaddr_rc addr = { 0 };

#if BT_SCAN
  dev_id = hci_get_route(NULL);
  sock = hci_open_dev( dev_id );
  if (dev_id < 0 || sock < 0)
  {
    perror("opening socket");
    return -1;
  }

  len  = 8;
  max_rsp = 255;
  flags = IREQ_CACHE_FLUSH;
  ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

  num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);

  if( num_rsp < 0 )
    perror("hci_inquiry");

  for (i = 0; i < num_rsp; i++)
  {
    ba2str(&(ii+i)->bdaddr, rsp_addr);
    memset(rsp_name, 0, sizeof(rsp_name));

    if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(rsp_name), rsp_name, 0) < 0)
      strcpy(rsp_name, "[unknown]");

    printf("%s  %s\n", rsp_addr, rsp_name);

    if (strcmp(rsp_name, target) == 0)
    {
      close (sock);
      free (ii);
      sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
      addr.rc_family = AF_BLUETOOTH;
      addr.rc_channel = (uint8_t) 1;
      str2ba(rsp_addr, &addr.rc_bdaddr );
      printf("Opening RFCOMM socket \n");
      if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
        return -1;
      return 0;
    }
  }
  printf("Could not find the raspberry pi\n");
  return 0;
#else
  sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  addr.rc_family = AF_BLUETOOTH;
  addr.rc_channel = (uint8_t) 1;
  str2ba(PI_ADDR, &addr.rc_bdaddr );
  printf("Opening RFCOMM socket \n");
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
    return -1;
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
  printf("Exp send len: %d\n", resp);
  printf("Actual send len: %d\n", len);
  return resp;
}

/* Closes the RFCOMM connection. */
void BluetoothChannel::channel_close()
{
  close(sock);
}
