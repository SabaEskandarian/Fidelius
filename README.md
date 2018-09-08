# Fidelius

## Setting up Bluetooth Pairing RPI3 <-> Desktop

Make the desktop discoverable. Then, open a terminal on RPI3 and type:

```
$ sudo bluetoothctl
[NEW] Controller $MAC_ADDRESS_PI [default]
{...}

[bluetooth]# agent on
Agent registered

[bluetooth]# default-agent
Default agent request successful

[bluetooth]# scan on
Discovery started
{$list_of_devices and addresses.}
```

From the list of devices find the one of the desktop and copy the MAC address XX:XX:XX:XX:XX:XX. Then type:

```
[bluetooth] pair XX:XX:XX:XX:XX:XX
```

 You can try to connect:
 ```
 [bluetooth] connect XX:XX:XX:XX:XX:XX
 ```
 
 If that fails or it switches from yes to no, then try to do the connection from the Desktop. Keep an eye on the terminal on RPI as bluetoothctl will ask you if you allow the connection.
 
