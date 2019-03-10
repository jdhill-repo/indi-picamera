/*
 Generic CCD
 CCD Template for INDI Developers
 Copyright (C) 2012 Jasem Mutlaq (mutlaqja@ikarustech.com)

 Multiple device support Copyright (C) 2013 Peter Polakovic (peter.polakovic@cloudmakers.eu)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <memory>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#include "config.h"
#include "indidevapi.h"
#include "eventloop.h"

#include "indi_picamera.h"



#define MAX_CCD_TEMP   45   /* Max CCD temperature */
#define MIN_CCD_TEMP   -55  /* Min CCD temperature */
#define MAX_X_BIN      16   /* Max Horizontal binning */
#define MAX_Y_BIN      16   /* Max Vertical binning */
#define MAX_PIXELS     4096 /* Max number of pixels in one dimension */
#define TEMP_THRESHOLD .25  /* Differential temperature threshold (C)*/
#define MAX_DEVICES    20   /* Max device cameraCount */

static int cameraCount;
static PiCameraCCD *cameras[MAX_DEVICES];

// -------------------------------------------------------------------------------------------

#include "stream/streammanager.h"
#include <pthread.h>

static pthread_cond_t cv         = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;

// -------------------------------------------------------------------------------------------

#include <memory>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <stdio.h>

#include <sstream>

#include <fcntl.h>

#define HEADERSIZE 0    //32768
#define IDSIZE 4    // number of bytes in raw header ID string

//#define RAWBLOCKSIZE 6404096 on OV5647 sensor
//#define ROWSIZE 3264 // number of bytes per row of pixels, including 24 'other' bytes at end (OV5647 sensor)
//#define HPIXELS 2592   // number of horizontal pixels on OV5647 sensor
//#define VPIXELS 1944   // number of vertical pixels on OV5647 sensor

#define RAWBLOCKSIZE 10171392   //10270208   // IMX219 sensor
#define ROWSIZE 4128    // (IMX219 sensor)
#define HPIXELS 3280   // number of horizontal pixels on IMX219 sensor
#define VPIXELS 2464  // number of vertical pixels on IMX219 sensor

int file_length;

int framecount;
int numOfFrames;

FILE *imageFileStreamPipe;

const char* cmd = "cat /home/jdhill/Development/rawtest/image_s25000_a16_d2.jpg";

int fullframe = 0;
int binned = 0;

//Testing
int testing = 0;

//Options
int bayer = 1;

// -------------------------------------------------------------------------------------------


/**********************************************************
 *
 *  IMPORRANT: List supported camera models in initializer of deviceTypes structure
 *
 **********************************************************/

static struct
{
    int vid;
    int pid;
    const char *name;
} deviceTypes[] = { { 0x0001, 0x0001, "CCD" }, { 0x0001, 0x0002, "PiAstroCam 2" }, { 0, 0, nullptr } };

static void cleanup()
{
    for (int i = 0; i < cameraCount; i++)
    {
        delete cameras[i];
    }
}

void ISInit()
{
    static bool isInit = false;
    if (!isInit)
    {
        /**********************************************************
     *
     *  IMPORRANT: If available use CCD API function for enumeration available CCD's otherwise use code like this:
     *
     **********************************************************

     cameraCount = 0;
     for (struct usb_bus *bus = usb_get_busses(); bus && cameraCount < MAX_DEVICES; bus = bus->next) {
       for (struct usb_device *dev = bus->devices; dev && cameraCount < MAX_DEVICES; dev = dev->next) {
         int vid = dev->descriptor.idVendor;
         int pid = dev->descriptor.idProduct;
         for (int i = 0; deviceTypes[i].pid; i++) {
           if (vid == deviceTypes[i].vid && pid == deviceTypes[i].pid) {
             cameras[i] = new PiCameraCCD(dev, deviceTypes[i].name);
             break;
           }
         }
       }
     }
     */

        /* For demo purposes we are creating two test devices */
        cameraCount            = 1;
        struct usb_device *dev = nullptr;
        cameras[0]             = new PiCameraCCD(dev, deviceTypes[0].name);
        //cameras[1]             = new PiCameraCCD(dev, deviceTypes[1].name);

        atexit(cleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        PiCameraCCD *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISGetProperties(dev);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        PiCameraCCD *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISNewSwitch(dev, name, states, names, num);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        PiCameraCCD *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISNewText(dev, name, texts, names, num);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        PiCameraCCD *camera = cameras[i];
        if (dev == nullptr || !strcmp(dev, camera->name))
        {
            camera->ISNewNumber(dev, name, values, names, num);
            if (dev != nullptr)
                break;
        }
    }
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    ISInit();

    for (int i = 0; i < cameraCount; i++)
    {
        PiCameraCCD *camera = cameras[i];
        camera->ISSnoopDevice(root);
    }
}

PiCameraCCD::PiCameraCCD(DEVICE device, const char *name)
{
    this->device = device;
    snprintf(this->name, 32, "PiAstroCam %s", name);
    setDeviceName(this->name);

    setVersion(GENERIC_VERSION_MAJOR, GENERIC_VERSION_MINOR);

    streamPredicate = 0;
    terminateThread = false;

}

PiCameraCCD::~PiCameraCCD()
{
}

const char *PiCameraCCD::getDefaultName()
{
    return "Generic CCD";
}

bool PiCameraCCD::initProperties()
{
    // Init parent properties first
    INDI::CCD::initProperties();

    // Most cameras have this by default, so let's set it as default.
    IUSaveText(&BayerT[2], "BGGR");

    uint32_t cap = CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME | CCD_HAS_BAYER /*| CCD_HAS_GUIDE_HEAD | CCD_HAS_STREAMING | CCD_HAS_COOLER | CCD_HAS_SHUTTER | CCD_HAS_ST4_PORT*/;
    SetCCDCapability(cap);

    addConfigurationControl();
    addDebugControl();
    return true;
}

void PiCameraCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);
}

bool PiCameraCCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        // Let's get parameters now from CCD
        setupParams();

        timerID = SetTimer(POLLMS);
    }
    else
    {
        rmTimer(timerID);
    }

    return true;
}

bool PiCameraCCD::Connect()
{
    LOG_INFO("Attempting to find PiCamera...");

    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Connect function
   *  If you encameraCounter an error, send the client a message
   *  e.g.
   *  LOG_INFO( "Error, unable to connect due to ...");
   *  return false;
   *
   *
   **********************************************************/

    /* Success! */
    LOG_INFO("Camera is online. Retrieving basic data.");

/*

    streamPredicate = 0;
    terminateThread = false;
    pthread_create(&primary_thread, nullptr, &streamVideoHelper, this);

*/


    if(!testing){
        system("camera_i2c");
    }


    return true;
}

bool PiCameraCCD::Disconnect()
{
    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD disonnect function
   *  If you encameraCounter an error, send the client a message
   *  e.g.
   *  LOG_INFO( "Error, unable to disconnect due to ...");
   *  return false;
   *
   *
   **********************************************************/
/*
    pthread_mutex_lock(&condMutex);
    streamPredicate = 1;
    terminateThread = true;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&condMutex);
*/

    terminateFrameStream();

    free(image);
    free(buffer);
    delete(pData);

    LOG_INFO("Camera is offline.");
    return true;
}

bool PiCameraCCD::setupParams()
{
    float x_pixel_size, y_pixel_size;
    int bit_depth = 10;
    int x_1, y_1, x_2, y_2;

    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Get basic CCD parameters here such as
   *  + Pixel Size X
   *  + Pixel Size Y
   *  + Bit Depth?
   *  + X, Y, W, H of frame
   *  + Temperature
   *  + ...etc
   *
   *
   *
   **********************************************************/

    ///////////////////////////
    // 1. Get Pixel size
    ///////////////////////////
    // Actucal CALL to CCD to get pixel size here
    x_pixel_size = 1.12;  // was 5.4;
    y_pixel_size = 1.12; // was 5.4;

    ///////////////////////////
    // 2. Get Frame
    ///////////////////////////

    // Actucal CALL to CCD to get frame information here
    x_1 = y_1 = 0;
    x_2       = HPIXELS;    //was 1280;
    y_2       = VPIXELS;    //was 1024;

    ///////////////////////////
    // 3. Get temperature
    ///////////////////////////
    // Setting sample temperature -- MAKE CALL TO API FUNCTION TO GET TEMPERATURE IN REAL DRIVER
    TemperatureN[0].value = 25.0;
//    LOGF_INFO("The CCD Temperature is %f", TemperatureN[0].value);
    IDSetNumber(&TemperatureNP, nullptr);

    ///////////////////////////
    // 4. Get temperature
    ///////////////////////////
    bit_depth = 16;
    SetCCDParams(x_2 - x_1, y_2 - y_1, bit_depth, x_pixel_size, y_pixel_size);


/*
    Streamer->setPixelFormat(INDI_MONO, 16);
    Streamer->setSize(PrimaryCCD.getXRes(), PrimaryCCD.getYRes());
*/

    // Now we usually do the following in the hardware
    // Set Frame to LIGHT or NORMAL
    // Set Binning to 1x1
    /* Default frame type is NORMAL */

    // Let's calculate required buffer
    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel cameraCount
    nbuf += 512;                                                                  //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    // ---------------------------------------------------------------------------
    // Allocate memory

    pData = new char[15 * 1024 * 1024];
    image = (unsigned short *)malloc((HPIXELS*VPIXELS) * sizeof(unsigned short));
    buffer = (unsigned short *)malloc((HPIXELS*VPIXELS) * sizeof(unsigned short));

    // ---------------------------------------------------------------------------


    return true;
}

int PiCameraCCD::SetTemperature(double temperature)
{
    // If there difference, for example, is less than 0.1 degrees, let's immediately return OK.
    if (fabs(temperature - TemperatureN[0].value) < TEMP_THRESHOLD)
        return 1;

    /**********************************************************
     *
     *  IMPORRANT: Put here your CCD Set Temperature Function
     *  We return 0 if setting the temperature will take some time
     *  If the requested is the same as current temperature, or very
     *  close, we return 1 and INDI::CCD will mark the temperature status as OK
     *  If we return 0, INDI::CCD will mark the temperature status as BUSY
     **********************************************************/

    // Otherwise, we set the temperature request and we update the status in TimerHit() function.
    TemperatureRequest = temperature;
    LOGF_INFO("Setting CCD temperature to %+06.2f C", temperature);
    return 0;
}

bool PiCameraCCD::StartExposure(float duration)
{
        minDuration = 1;

        if (duration < minDuration)
        {
            DEBUGF(INDI::Logger::DBG_WARNING,
                   "Exposure shorter than minimum duration %g s requested. \n Setting exposure time to %g s.", minDuration,
                   minDuration);
            duration = minDuration;
        }

        if (imageFrameType == INDI::CCDChip::BIAS_FRAME)
        {
            duration = minDuration;
            LOGF_INFO("Bias Frame (s) : %g\n", minDuration);
        }

        /**********************************************************
       *
       *
       *
       *  IMPORRANT: Put here your CCD start exposure here
       *  Please note that duration passed is in seconds.
       *  If there is an error, report it back to client
       *  e.g.
       *  LOG_INFO( "Error, unable to start exposure due to ...");
       *  return -1;
       *
       *
       **********************************************************/

        // ---------------------------------------------------------------------------

        // Clear summing buffer
        memset(buffer, 0, (HPIXELS*VPIXELS) * sizeof(unsigned short));

        //  Set Bayer
        if (fullframe && !binned && bayer)
        {
            // Has Bayer
            SetCCDCapability(GetCCDCapability() | CCD_HAS_BAYER);
        }else{
            // No Bayer when subframed or binned
            SetCCDCapability(GetCCDCapability() & ~CCD_HAS_BAYER);
        }

         // ---------------------------------------------------------------------------

        //Start Frames
        if(!FrameStreamIsRunning){
            startFrameStream(); // Add error checking . Don't set "FrameStreamIsRunning = true" unless succesful.
        }

        FrameStreamIsRunning = true;

        // ---------------------------------------------------------------------------

        // Set number of frames to collect
        numOfFrames = duration;

        PrimaryCCD.setExposureDuration(duration);
        ExposureRequest = duration;

        gettimeofday(&ExpStart, nullptr);
        LOGF_INFO("Taking a %g second image...", ExposureRequest);

        InExposure = true;

        // Reset frame count
        framecount = 0;

    return true;
}


int PiCameraCCD::terminateFrameStream(){

    // Terminate frame stream & close pipe

    if(FrameStreamIsRunning){

        if(testing){
            system("pkill streamraw");
        }else{
            system("pkill raspiraw");
        }

        FrameStreamIsRunning = false;
        LOG_INFO("Stream Closed");

        pclose(imageFileStreamPipe);

        LOG_INFO("Pipe Closed");

        InExposure = false;

    }

    return 0;

}


int PiCameraCCD::startFrameStream(){  // Add arguments for exposure and framerate. Maybe set in indi console later.


    // ---------------------------------------------------------------------------
    // Initialize frames

    if(FrameStreamIsRunning){
        return 0;
    }

    // ---------------------------------------------------------------------------
    // ===================================================================================
    // For Raspi

    if(!testing){

        // Create command
        ostringstream cmd;

        cmd << "raspiraw -md 2 -o /dev/stdout -t 9999999 -sr 1 -eus 950000 -g 230 -f 1";

        ///LOGF_INFO("cmd : %s\n", cmd.str().c_str());

        imageFileStreamPipe = popen(cmd.str().c_str(), "r");

    }

    // --------------------------------------------------------------------------------------
    // For testing

    if(testing){

        const char* command = "'/home/jdhill/Link to Pictures/Astro/rawtest/test1/./streamraw'";

        imageFileStreamPipe = popen(command, "r"); // used for testing with cat file

    }

    // ===================================================================================

    // set pipe as non blocking
    int d = fileno(imageFileStreamPipe);
    fcntl(d, F_SETFL, O_NONBLOCK);

    // Check pipe
    if (!imageFileStreamPipe){

        LOG_INFO("Runtime Error - popen failed! Please Check Camera");

    }else{

        LOG_INFO("Pipe Opened!");

    }

    // ===================================================================================


    return 0;

}

bool PiCameraCCD::AbortExposure()
{
    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD abort exposure here
   *  If there is an error, report it back to client
   *  e.g.
   *  LOG_INFO( "Error, unable to abort exposure due to ...");
   *  return false;
   *
   *
   **********************************************************/

    InExposure = false;

    terminateFrameStream();

    return true;
}


bool PiCameraCCD::UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType)
{
    INDI::CCDChip::CCD_FRAME imageFrameType = PrimaryCCD.getFrameType();

    if (fType == imageFrameType)
        return true;

    switch (imageFrameType)
    {
        case INDI::CCDChip::BIAS_FRAME:
        case INDI::CCDChip::DARK_FRAME:
            /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Frame type here
     *  BIAS and DARK are taken with shutter closed, so _usually_
     *  most CCD this is a call to let the CCD know next exposure shutter
     *  must be closed. Customize as appropiate for the hardware
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to set frame type to ...");
     *  return false;
     *
     *
     **********************************************************/
            break;

        case INDI::CCDChip::LIGHT_FRAME:
        case INDI::CCDChip::FLAT_FRAME:
            /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Frame type here
     *  LIGHT and FLAT are taken with shutter open, so _usually_
     *  most CCD this is a call to let the CCD know next exposure shutter
     *  must be open. Customize as appropiate for the hardware
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to set frame type to ...");
     *  return false;
     *
     *
     **********************************************************/
            break;
    }

    PrimaryCCD.setFrameType(fType);

    return true;
}

bool PiCameraCCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    /* Add the X and Y offsets */
    long x_1 = x;
    long y_1 = y;

    long bin_width  = x_1 + (w/* / PrimaryCCD.getBinX()*/);
    long bin_height = y_1 + (h/* / PrimaryCCD.getBinY()*/);

    if (bin_width > PrimaryCCD.getXRes()/* / PrimaryCCD.getBinX()*/)
    {
        LOGF_INFO("Error: invalid width requested %d", w);
        return false;
    }
    else if (bin_height > PrimaryCCD.getYRes()/* / PrimaryCCD.getBinY()*/)
    {
        LOGF_INFO("Error: invalid height request %d", h);
        return false;
    }

    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Frame dimension call
   *  The values calculated above are BINNED width and height
   *  which is what most CCD APIs require, but in case your
   *  CCD API implementation is different, don't forget to change
   *  the above calculations.
   *  If there is an error, report it back to client
   *  e.g.
   *  LOG_INFO( "Error, unable to set frame to ...");
   *  return false;
   *
   *
   **********************************************************/

    if ((x_1 == 0) && (y_1 == 0) && (w == 3280) && (h == 2464))
    {
        // Is fullframe image

        fullframe = 1;

    }else{

        fullframe = 0;

    }

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x_1, y_1, w, h);

    int nbuf;
    nbuf = (bin_width * bin_height * PrimaryCCD.getBPP() / 8); //  this is pixel count
    nbuf += 512;                                               //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    LOGF_DEBUG("Setting frame buffer size to %d bytes.", nbuf);

    return true;
}

bool PiCameraCCD::UpdateCCDBin(int binx, int biny)
{
    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Binning call
   *  If there is an error, report it back to client
   *  e.g.
   *  LOG_INFO( "Error, unable to set binning to ...");
   *  return false;
   *
   *
   **********************************************************/

    if ((binx == 1) && (biny == 1))
    {
        // Is binned

        binned = 0;

    }else{

        binned = 1;

    }


    PrimaryCCD.setBin(binx, biny);

    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

float PiCameraCCD::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = ExposureRequest - timesince;

    return timeleft;
}



int PiCameraCCD::getFrame(unsigned short *image){

    size_t result = 0;
    long int totalBytesread = 0;
    int loopcount = 0;
    int recheck = 0;


        ///LOG_INFO("getFrame called");

        // -----------------------------------------
        if(!imageFileStreamPipe){   // pipe is not open

            return 0;

        }
        // -----------------------------------------


        do{

                loopcount++;

        // Copy the file into the buffer:
        result = fread (pData + totalBytesread,1,RAWBLOCKSIZE,imageFileStreamPipe);

             file_length = totalBytesread = result + totalBytesread;

             // ============================================================
             //  Unpack raw file

             if(file_length == RAWBLOCKSIZE){ // Retrieved all of image

                recheck = 0;

                ///LOGF_INFO("loopcount = %i", loopcount);

                ///LOGF_INFO("... Frame received. -> %li bytes. ", file_length);

                unsigned long offset;  // offset into file to start reading pixel data
                unsigned char split;        // single byte with 4 pairs of low-order bits

                offset = (file_length - RAWBLOCKSIZE) + HEADERSIZE;  // location in file the raw pixel data starts

                unsigned long p; // position in pData (incrementer)
                unsigned long iPos; // position in image (incrementer)

                p = offset; // set to beginning of raw data block

                for (int row=0; row < VPIXELS; row++) {  // iterate over pixel rows

                    for (int col = 0; col < HPIXELS; col+= 4) {  // iterate over pixel columns

                        iPos = (row * HPIXELS) + col;

                        image[iPos+0] = pData[p++] << 8;
                        image[iPos+1] = pData[p++] << 8;
                        image[iPos+2] = pData[p++] << 8;
                        image[iPos+3] = pData[p++] << 8;
                        split = pData[p++];    // low-order packed bits from previous 4 pixels
                        image[iPos+0] += (split & 0b11000000);  // unpack them bits, add to 16-bit values, left-justified
                        image[iPos+1] += (split & 0b00110000)<<2;
                        image[iPos+2] += (split & 0b00001100)<<4;
                        image[iPos+3] += (split & 0b00000011)<<6;

                    }

                p+= 28; // Extra bytes at end of each row

                } // end

                ///LOG_INFO("Raw Data Unpacked");

                // Increment frame count
                framecount ++;

                LOGF_INFO("Frame %i of %i", framecount, numOfFrames);

                //Reset in buffer
                totalBytesread = 0;


                // ************** Perform Image Operations *****************
                // such as summing, averaging, noise clip, etc

                // Summming operation
                for (int pixel=0; pixel < HPIXELS * VPIXELS; pixel++) {  // iterate over pixels
                        buffer[pixel] += image[pixel] >> 6; // remove shift created during image unpacking
                }
                // *********************************************************
/*
                // ************** Perform Image Operations *****************
                // For video streaming

                // Summming operation
                for (int pixel=0; pixel < HPIXELS * VPIXELS; pixel++) {  // iterate over pixels

                        buffer[pixel] = image[pixel]; // To Do: Change to memcopy

                }
                // *********************************************************
*/

            }


            // ============================================================


        }while(totalBytesread > 0);

    return 0;

}


int PiCameraCCD::addtosum(unsigned short *image, unsigned short *buffer){

    // Summming operation
    for (int pixel=0; pixel < HPIXELS * VPIXELS; pixel++) {  // iterate over pixels
            buffer[pixel] += image[pixel];
    }

  return 0;

}



int PiCameraCCD::subFrame(unsigned short *image, unsigned short *subframe){

    // Subframe parameters
    int x_1 = PrimaryCCD.getSubX();
    int y_1 = PrimaryCCD.getSubY();
    int x_2 = x_1 + PrimaryCCD.getSubW();
    int y_2 = y_1 + PrimaryCCD.getSubH();

    long pixcount = 0;

    for(int v = y_1; v < y_2; v++){

        for(int h = x_1; h < x_2; h++){

            subframe[pixcount] = image[(v*HPIXELS)+h];
            pixcount++;

        }

    }


    ///LOG_INFO("Subframe Completed");

    return 0;
}


void PiCameraCCD::TimerHit()
{
    uint32_t nextTimer = POLLMS;

    //  No need to reset timer if we are not connected anymore
    if (!isConnected())
        return;

    if (InExposure)
    {

    if (AbortPrimaryFrame)
    {
        InExposure        = false;
        AbortPrimaryFrame = false;
    }
    else
    {
        float timeleft;
        timeleft = CalcTimeLeft();

        if (timeleft < 0)
            timeleft = 0;

        PrimaryCCD.setExposureLeft(timeleft);

        // =========================================================================
        // Get next frame, process, and add to buffer

        // Grab frame
        if(framecount < numOfFrames){

            getFrame(image);

        }

        // =========================================================================


        if (timeleft < 1.0)
        {
            if (timeleft <= 0.001)
            {

            // =========================================================================
            // Get last frame & collect missing frames

            ///LOGF_INFO("%i frames were skipped of %i", (numOfFrames - framecount), numOfFrames);
            ///LOG_INFO("Hit - Exposure done. Final getFrame calls.");

            if(framecount < numOfFrames)
            {

                  getFrame(image);

            }else{

                InExposure = false;

                // =========================================================================
                // Finalize, convert, and send/write image

                // **** Perform subframe ****
                subFrame(buffer, (unsigned short *)PrimaryCCD.getFrameBuffer());

                // Binning
                PrimaryCCD.binFrame();

                ExposureComplete(&PrimaryCCD);

                LOG_INFO("Image complete.");

                // =========================================================================

                }

            }else{

                //  set a shorter timer
                nextTimer = timeleft * 1000;
            }

        }

    }

    }else{

        if(FrameStreamIsRunning){ // '

        // ******************************************************************************************
        // Read and dispose of unused frame

        size_t result = 0;
        long int totalBytesread = 0;
        int loopcount = 0;

        do{

            loopcount++;

            // Copy the file into the buffer:
            result = fread (pData + totalBytesread,1,RAWBLOCKSIZE,imageFileStreamPipe);

            file_length = totalBytesread = result + totalBytesread;

            if(file_length == RAWBLOCKSIZE){ // Retrieved all of frame

                ///LOGF_INFO("... Frame received and deleted. -> %li bytes. ", file_length);

                //Reset in buffer
                totalBytesread = 0;

                }

            }while(totalBytesread > 0);

        // ******************************************************************************************

        framecount = 0;

        // =========================================================================
        // Terminate frame stream and close pipe

            terminateFrameStream();

        // =========================================================================


          }


     }


    SetTimer(nextTimer);
}






IPState PiCameraCCD::GuideNorth(uint32_t ms)
{
    INDI_UNUSED(ms);
    /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Guide call
   *  Some CCD API support pulse guiding directly (i.e. without timers)
   *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
   *  will have to start a timer and then stop it after the 'ms' milliseconds
   *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
   *  available in INDI 3rd party repository
   *  If there is an error, report it back to client
   *  e.g.
   *  LOG_INFO( "Error, unable to guide due ...");
   *  return IPS_ALERT;
   *
   *
   **********************************************************/

    return IPS_OK;
}

IPState PiCameraCCD::GuideSouth(uint32_t ms)
{
    INDI_UNUSED(ms);
    /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Guide call
     *  Some CCD API support pulse guiding directly (i.e. without timers)
     *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
     *  will have to start a timer and then stop it after the 'ms' milliseconds
     *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
     *  available in INDI 3rd party repository
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to guide due ...");
     *  return IPS_ALERT;
     *
     *
     **********************************************************/

    return IPS_OK;
}

IPState PiCameraCCD::GuideEast(uint32_t ms)
{
    INDI_UNUSED(ms);
    /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Guide call
     *  Some CCD API support pulse guiding directly (i.e. without timers)
     *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
     *  will have to start a timer and then stop it after the 'ms' milliseconds
     *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
     *  available in INDI 3rd party repository
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to guide due ...");
     *  return IPS_ALERT;
     *
     *
     **********************************************************/

    return IPS_OK;
}

IPState PiCameraCCD::GuideWest(uint32_t ms)
{
    INDI_UNUSED(ms);
    /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Guide call
     *  Some CCD API support pulse guiding directly (i.e. without timers)
     *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
     *  will have to start a timer and then stop it after the 'ms' milliseconds
     *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
     *  available in INDI 3rd party repository
     *  If there is an error, report it back to client
     *  e.g.
     *  LOG_INFO( "Error, unable to guide due ...");
     *  return IPS_ALERT;
     *
     *
     **********************************************************/

    return IPS_OK;
}

// *****************************************************************************************

bool PiCameraCCD::StartStreaming()
{

    startFrameStream();
    FrameStreamIsRunning = true;

    ExposureRequest = 1.0 / Streamer->getTargetFPS();
    pthread_mutex_lock(&condMutex);
    streamPredicate = 1;
    pthread_mutex_unlock(&condMutex);
    pthread_cond_signal(&cv);

    return true;
}

bool PiCameraCCD::StopStreaming()
{
    pthread_mutex_lock(&condMutex);
    streamPredicate = 0;
    pthread_mutex_unlock(&condMutex);
    pthread_cond_signal(&cv);

    return true;
}

void *PiCameraCCD::streamVideoHelper(void *context)
{
    return ((PiCameraCCD *)context)->streamVideo();
}

void *PiCameraCCD::streamVideo()
{
    struct itimerval tframe1, tframe2;
    double s1, s2, deltas;

    while (true)
    {
        pthread_mutex_lock(&condMutex);

        while (streamPredicate == 0)
        {
            pthread_cond_wait(&cv, &condMutex);
            ExposureRequest = 1.0 / Streamer->getTargetFPS();
        }

        if (terminateThread)
            break;

        // release condMutex
        pthread_mutex_unlock(&condMutex);

        // Simulate exposure time
        //usleep(ExposureRequest*1e5);

        // 16 bit
//        DrawCcdFrame(&PrimaryCCD);


        getitimer(ITIMER_REAL, &tframe1);

        s1 = ((double)tframe1.it_value.tv_sec) + ((double)tframe1.it_value.tv_usec / 1e6);
        s2 = ((double)tframe2.it_value.tv_sec) + ((double)tframe2.it_value.tv_usec / 1e6);
        deltas = fabs(s2 - s1);

        if (deltas < ExposureRequest)
            usleep(fabs(ExposureRequest-deltas)*1e6);

        uint32_t size = PrimaryCCD.getFrameBufferSize() /* / (PrimaryCCD.getBinX()*PrimaryCCD.getBinY())*/;
        Streamer->newFrame(PrimaryCCD.getFrameBuffer(), size);

        getitimer(ITIMER_REAL, &tframe2);
    }

    pthread_mutex_unlock(&condMutex);
    return 0;
}



