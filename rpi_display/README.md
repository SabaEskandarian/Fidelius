# Install

1. Download and install Raspian (we are using version 9, however the the latest
   version should suffice)
2. Install python 2.7/pip if it is not already installed
3. Run the following commands to install the necessary libraries:
```
$ sudo apt install gcc libffi-dev libssl-dev python-dev
$ sudo pip install cryptography
$ sudo pip install pybluez
```
4. Clone this repository onto the Raspberry PI

# Usage

The code of the RPI 3 have two main components:

1. `comm_server.py` receives encrypted overlays from the Bluetooth channel, decrypts them, and writes them into a UNIX socket
2. `display_server.py` reads from the UNIX socket and shows them on top of the HDMI input signal coming directly from the computer running the browser.

The two servers are separated to increase reliability of the RPI using the Auvidea board. The board is unstable and opening consecutive times the HDMI stream may bring the chipset into a faulty state from which it may not recover. Only after multiple reboots, power cycles, prayers, and optimal alignments of planets will manage to get the Auvida back to work.

## Start the Overlay Server 

We suggest to run the server within a screen session so to access to the logs on screen via SSH: 

```
$ screen

[inside screen]
screen$ cd Fidelius/rpi_display/
screen$ ./start.sh
```

## Start the Comm Server

### Case 1: After power-cycling the hell out of the RPI

From the PC, open an SSH connection to the RPI:

```
$ ssh pi@192.168.0.100
pi$ screen -dr

[press CTRL+A C to spawn a new bash inside the screen session]

screen$ cd Fidelius/rpi_display/
screen$ python comm_server.py
```

### Case 2: General case

From the PC, open an SSH connection to the RPI:

```
$ ssh pi@192.168.0.100
pi$ screen -dr

[press CTRL+A N to move to next shell, if needed]

screen$ cd Fidelius/rpi_display/
screen$ python comm_server.py
```

## No internet on the raspberry pi

Most likely there is a routing rule that forward internet traffic to our router. To remove this rule use:

```
pi$ cd Fidelius/rpi_display/
pi$ ./fix_routes.sh
```
