
# Installing and setting up raspiraw from source

cd

git clone https://github.com/6by9/raspiraw.git

cd raspiraw

./buildme

-------------------------------------------------------

The following only needs to be performed once:

sudo nano ~/.bashrc

	Add:

	PATH=~/raspiraw:~/raspiraw/tools:$PATH

sudo nano /boot/config.txt

	Add:

    start_file=start_x.elf
    dtparam=i2c_vc=on
    (dtparam=i2c_arm=on (already existed in file))

sudo nano /etc/rc.local

	Add:

    sudo modprobe i2c_bcm2708 # Needed to probe this for /dev/i2cX to show up
    sudo modprobe i2c_dev


sudo reboot


