# indi-picamera
Raspberry Pi Camera Driver For INDI

The indi-picamera driver is an experimental INDI driver for the Raspberry Pi Camera. The current version only supports the V2 camera. The indi-picamera driver makes use of raspiraw, an example app that receives data directly from CSI sensors on the Raspberry Pi. It supports full sensor (raw bayered ), subframed, and binned images. The Bayer pattern of "BGGR" is written to the FITS header if a fullframe, unbinned image is captured. The driver uses internal automatic software sum stacking of roughly 1 second integrations to achieve very long exposures.

The driver does not currently support the following:

	Raspberry Pi V1 Camera

	Subsecond exposures

	Live video

	ST-4 guiding (Pulse guiding is supported)

	Median stacking


To do:

Add INDI config tab -

	User Gain Setting - The gain is currently set near maximum.
	
	Bayer Enable / Disable (Needed for modified cameras)

	User Config Sum / Median Stacking

---------------------------------------------------------------------------------------------------------

# Required for the indi-picamera driver:

raspiraw - Download and install pre-compiled binaries from https://github.com/jdhill-repo/raspiraw-bin or build from source https://github.com/6by9/raspiraw.

wiringPi - http://wiringpi.com/download-and-install/ - It is highly recommended that you update wiringpi, even if it is already installed.

indi development build - https://github.com/indilib/indi - if compiling driver from source. 

	Note: The driver is available as pre-compiled binaries at https://github.com/jdhill-repo/indi-picamera-bin. The indi development build would not be needed if these are used instead of compiling from source.

-------------------------------------------------------

# Build and install the indi-picamera driver

	cd ~/indi/3rdparty

	git clone https://github.com/jdhill-repo/indi-picamera.git

	cd indi-picamera

	mkdir build

	cd build

	cmake -DCMAKE_INSTALL_PREFIX=/usr ..

	make

	sudo make install

-------------------------------------------------------

# Running indi-picamera:

The driver name is 'indi_picamera_ccd'.

To run, add it to the indiserver command line with your other drivers. For example: indiserver -v indi_canon_ccd indi_celestron_gps indi_picamera_ccd

For setting it up with to use with Ekos, select it from list of drivers, or alternatively, 'indi_picamera_ccd' can be typed directly into the driver field box if it does not appear on the drop down list. 

On the INDI configuration tab, set Polling to 250(ms). This seems to work well, especially for guiding.

For guiding, I have tested using the internal guider with Exposures of 1 or 2 seconds, Binning 4x4, Rapid Guide enabled*, with Auto Loop and Send Image checked under the Rapid Guide tab and adjusting the settings from those. These settings have given me sub arc-second guiding. Of course, the driver is installed on the Raspberry Pi that the camera is connected to.

*When not guiding, disable Rapid Guide.

-------------------------------------------------------

# Notes:

1 - If building raspiraw from source see https://github.com/jdhill-repo/indi-picamera/blob/master/raspiraw_source_install.md.


2 - There is a system call from the driver to run the camera_i2c script automatically each time the driver is launched, but this currently only works if using a modified version of camera_i2c. If using the raspiraw pre-compiled binaries from https://github.com/jdhill-repo/raspiraw-bin, camera_i2c has already been modified and you should not need to change it. 

If raspiraw is compiled from source, you will need to modify the camera_i2c script for your install. Change line 111 (./rpi3-gpiovirtbuf s 133 1) to include the full directory of raspiraw where rpi3-gpiovirtbuf is located.

sudo nano camera_i2c

	change

	./rpi3-gpiovirtbuf s 133 1

	to

	~/raspiraw/rpi3-gpiovirtbuf s 133 1


Alternatively, the camera_i2c script would have to be executed before the beginning of the INDI session at least once when used with the Raspberry Pi 3. It will not work properly unless it is executed from the raspiraw directory.



