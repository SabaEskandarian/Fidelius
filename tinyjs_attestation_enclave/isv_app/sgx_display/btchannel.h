#ifndef BLUETOOTH_CHANNEL_H
#define BLUETOOTH_CHANNEL_H

class BluetoothChannel
{
  public:
    BluetoothChannel ();
    int channel_open ();
    int channel_send (char *data, int len);
    void channel_close ();
  private:
    int sock;
};

#endif // BLUETOOTH_CHANNEL_H
