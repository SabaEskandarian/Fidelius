## How to start the overlay server 
```
$ screen

[inside screen]
screen$ cd Fidelius/rpi_display/
screen$ ./start.sh
```

## How to start the comm server

### Case 1: After powercycling

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
