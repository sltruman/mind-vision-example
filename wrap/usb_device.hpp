#ifndef USBDEVICE_H
#define USBDEVICE_H

#include "device.hpp"
#include <iostream>

struct USB2Device : public Device {
    int capture = 0;
    virtual  void status_sync() {
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


        status_string.clear();
        status_string.seekp(0);
        status_string << statistic.iCapture << ','
             << captureFPS << ','
             << sensorFPS << ','
             << 0 << ','
             << statistic.iLost << ','
             << 0 << ','
             << 0 << ','
             << uLinkSpeed << ','
             << endl;

        capture = statistic.iCapture;
    }
};


struct USB3Device : public Device {
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

        const int IO_CONTROL_GET_FRAME_DISCARD    = 15;
        const int IO_CONTROL_FPGA_READ			= 8;
        const int IO_CONTROL_GET_RECOVER_COUNT = 24;

        CameraSpecialControl(handle, IO_CONTROL_FPGA_READ, 0x75, &sensorFPS);
        sensorFPS /= 4.f;
        CameraSpecialControl(handle, IO_CONTROL_GET_FRAME_DISCARD, 0, &iFrameLost);

        int iRecover = 0;
        CameraSpecialControl(handle, IO_CONTROL_GET_RECOVER_COUNT, 0, &iRecover);

        status_string.clear();
        status_string.seekp(0);
        status_string << statistic.iCapture << ','
             << captureFPS << ','
             << sensorFPS << ','
             << fDevTemperature << ','
             << iFrameLost << ','
             << iResend << ','
             << iRecover << ','
             << uLinkSpeed << ','
             << endl;

        capture = statistic.iCapture;

        std::cout << capture << endl;
    }
};

#endif // USB-DEVICE_H
