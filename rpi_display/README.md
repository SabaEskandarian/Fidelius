# What to do when power-cycling RPI

```
$ screen

[inside screen]
screen$ cd Fidelius/rpi_display/
screen$ ./start
```
At this point, the overlay server should show the desktop of our PC. From the PC, connect to the RPI using SSH and:

```
$ ssh pi@192.168.0.100
pi$ screen -dr
pi$ cd Fidelius/rpi_display/
pi$ python comm_server.py
```
