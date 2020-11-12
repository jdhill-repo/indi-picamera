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

#ifndef INDI_PICAMERA_H
#define INDI_PICAMERA_H

#include <indiccd.h>
#include <iostream>
#include <string>

using namespace std;

#define DEVICE struct usb_device *

class PiCameraCCD : public INDI::CCD
{
  public:
    PiCameraCCD(DEVICE device, const char *name);
    virtual ~PiCameraCCD();

    const char *getDefaultName();

    bool initProperties();
    void ISGetProperties(const char *dev);
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    int SetTemperature(double temperature);
    bool StartExposure(float duration);
    bool AbortExposure();

//    bool StartGuideExposure(float) override;
//    bool AbortGuideExposure() override;

    static void *streamVideoHelper(void *context);
    void *streamVideo();


    char * pData;
    unsigned short *image;
    unsigned short *outputframe;
    unsigned short *guideoutputframe;
    unsigned short *subframe;
    unsigned short *buffer;

  protected:
    void TimerHit();
    virtual bool UpdateCCDFrame(int x, int y, int w, int h);
    virtual bool UpdateCCDBin(int binx, int biny);
    virtual bool UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType);

    // Guide Port
    virtual IPState GuideNorth(uint32_t ms);
    virtual IPState GuideSouth(uint32_t ms);
    virtual IPState GuideEast(uint32_t ms);
    virtual IPState GuideWest(uint32_t ms);

//    virtual bool StartStreaming() override;
//    virtual bool StopStreaming() override;

    virtual bool StartStreaming();
    virtual bool StopStreaming();

  private:
    DEVICE device;
    char name[32];

    double ccdTemp;
    double minDuration;
    unsigned short *imageBuffer;

    int timerID;

    INDI::CCDChip::CCD_FRAME imageFrameType;

    struct timeval ExpStart;

    float ExposureRequest;
    float TemperatureRequest;

//    float GuideExposureRequest { 0 };

//    bool AbortGuideFrame { false };
    bool AbortPrimaryFrame { false };

    bool FrameStreamIsRunning { false };

    float CalcTimeLeft();

    int getFrame(unsigned short *image);
    int subFrame(unsigned short *image, unsigned short *subframe);
    int addtosum(unsigned short *image, unsigned short *buffer);

    int startFrameStream();
    int terminateFrameStream();

	string exec(string);

    int streamPredicate;
    pthread_t primary_thread;
    bool terminateThread;

        bool setupParams();
        bool sim;

        friend void ::ISGetProperties(const char *dev);
        friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
        friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
        friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
        friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                                char *formats[], char *names[], int n);
    };

#endif // INDI_PICAMERA_H
