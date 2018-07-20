# Pi Zero setup

## OS installation

### Requirements:
    - Raspberry Pi Zero W
    - Mini SD card
    - Computer to read/modify mini sd card

Steps:
    - Install Rasbian "Jessie" or "Jessie lite" using NOOBS
    - During installation connect the Pi to a router via wifi. 
    - Instructions at: https://www.raspberrypi.org/documentation/installation/noobs.md
        - As per instructions on link above, format the SD card and perform full overwrite.
	- This takes ~15 min
    - After installation is complete: Login name = "pi" and Password = "raspberry"
	

## Enable SSH on Pi
    - After Raspbian is installed enable ssh on the Pi.
    - To enable SSH: https://www.raspberrypi.org/documentation/remote-access/ssh/
        ~~~~
    	sudo systemctl enable ssh
    	sudo systemctl start ssh
    	~~~~
    - ssh to the pi using a computer connected on the same network by using 
      the address specifed by the pi `ifconfig`
    - Or you can use `ssh pi@raspberrypi.local`


## Device Configuration

The Pi needs to be set to act as a device. We will use the ConfigFS library to do this. 
First, download the sgx-web-seckeyboard repo onto the pi. (https://projects.cispa.saarland/giancarlo.pellegrino/sgx-web-seckeyboard.git)

    - Follow Step 1 on http://isticktoit.net/?p=1383
      ~~~~
      sudo BRANCH=next rpi-update
      echo "dtoverlay=dwc2" | sudo tee -a /boot/config.txt
      echo "dwc2" | sudo tee -a /etc/modules
      sudo echo "libcomposite" | sudo tee -a /etc/modules
      ~~~~

      
    - Copy over the device config file from the git repo as follows:

    `sudo cp ~/sgx-web-seckeyboard/slave/mykeyboard_usb /usr/bin/`

    Comfirm that "/usr/bin/mykeyboard_usb" is root owned and executable

    Modify "/etc/rc.local" so that it contains the line: `/usr/bin/mykeyboard_usb` right before `exit 0`
    It should look like this at the end of "/etc/rc.local": 
    
    ~~~~
    /usr/bin/mykeyboard_usb

    exit 0

    ~~~~

    Thats it! Now you can shut the pi down using `sudo shutdown now`
    When you connect it via usb to a host. The host will register the pi as a keyboard device.
     
    When connecting to a MAC you can use the command `system_profiler SPUSBDataType` from the mac terminal to verify the "Normal Keyboard SGX" has been connected. 

## Interacting with the device after its connected

On linux machines use the "sgx-web-seckeyboard/host/simple_reciever.py script to read data from the serial ports opened by the pi. On linux you will see two new device files created: "/dev/ttyACM0" and "/dev/ttyACM1". These are not regular files, but correspond to the serial ports "/dev/ttyGS0" and "/dev/ttyGS1" on the pi. Whatever is written to "/dev/ttyGS0" on the pi can be read from "/dev/ttyACM0" on the linux host. Similarly for "/dev/ttyGS1" and "/dev/ttyACM1".

Start the "simple_receiver.py" first. 

Then on the pi use the following to test the serial ports. You can also test writing to "/dev/ttyGS1" by having the simple_reciever listen to "/dev/ttyACM1".

~~~~
sudo -i
echo -ne "Hello, World" > /dev/ttyGS0

~~~~

# Other details

The cpp/ folder in this directory contains the main application used by the pi to send keys to the linux host. There is a readme there that explains more.


