#ifndef GIGEDEVICE_H
#define GIGEDEVICE_H

#include "device.hpp"
#include <string>

using std::string;

struct GIGEDevice : public Device {
    string camIp,camMask,camGateWay,etIp,etMask,etGateWay;
    int capture = 0;

    virtual void status_sync() {
        const int IO_CONTROL_DEVICE_TEMPERATURE	= 20;
        const int IO_CONTROL_GET_FRAME_RESEND   = 17;
        const int IO_CONTROL_GET_LINK_SPEED	= 26;
        int iResend = 0;
        float fDevTemperature = 0.f;
        int iFrameLost = 0;
        UINT sensorFPS = 0;
        UINT uLinkSpeed = 0;

        CameraSpecialControl(handle, IO_CONTROL_GET_FRAME_RESEND, 0, &iResend);
        CameraSpecialControl(handle,IO_CONTROL_DEVICE_TEMPERATURE,0,&fDevTemperature);
        CameraSpecialControl(handle, IO_CONTROL_GET_LINK_SPEED, 0, &uLinkSpeed);

        tSdkFrameStatistic statistic;
        CameraGetFrameStatistic(handle,&statistic);

        auto captureFPS = statistic.iCapture - capture;

        const int IO_CONTROL_GET_FRAME_LOST = 14;
        const int IO_CONTROL_GET_GEVREG		= 0x1000;

        int iFrameResend = 0;
        CameraSpecialControl(handle,IO_CONTROL_GET_FRAME_LOST,0,&iFrameLost);
        CameraSpecialControl(handle,IO_CONTROL_GET_FRAME_RESEND,0,&iFrameResend);

        UINT uPackSize = 0;
        CameraSpecialControl(handle, IO_CONTROL_GET_GEVREG, 0xD04, &uPackSize);
        CameraSpecialControl(handle, IO_CONTROL_GET_GEVREG, 0x10000510, &sensorFPS);
        sensorFPS /= 4.f;

        UINT TrigCount = 0;
        CameraSpecialControl(handle, IO_CONTROL_GET_GEVREG, 0x1000050C, &TrigCount);

        char GvspDevTemp[32] = { 0 };
        CameraCommonCall(handle, "get_gvsp_dev_temp()", GvspDevTemp, sizeof(GvspDevTemp));

        status_string.clear();
        status_string.seekp(0);
        status_string << statistic.iCapture << ','
             << captureFPS << ','
             << sensorFPS << ','
             << fDevTemperature << ','
             << iFrameLost << ','
             << iFrameResend << ','
             << (uPackSize & 0xffff) << ','
             << uLinkSpeed << ','
             << endl;

        capture = statistic.iCapture;
    }
};

#endif // GIGE-DEVICE_H
