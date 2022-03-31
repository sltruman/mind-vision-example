#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "gige_device.hpp"
#include "usb_device.hpp"
#include "wrap/gf120.hpp"

#include <CameraApi.h>
#include <CameraStatus.h>

#include <algorithm>
#include <list>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <mutex>
#include <memory>

using std::string;
using std::shared_ptr;

struct DeviceManager {
    DeviceManager(int language=1) {
        CameraSdkInit(language);    //sdk初始化  0 English 1中文
    }

    ~DeviceManager() {
    }

    std::list<shared_ptr<Device>>& list() {
        int                     cameraCounts = 100;
        tSdkCameraDevInfo       cameraEnumList[100];
        CameraEnumerateDevice(cameraEnumList,&cameraCounts);

        auto devices_copy = devices;
        devices.clear();

        for(auto i=0;i < cameraCounts;i++) {
            auto info = cameraEnumList[i];
            string series(info.acProductSeries);
            std::transform(series.begin(),series.end(),series.begin(),toupper);

            using std::make_shared;
            shared_ptr<Device> device;

            for(auto p : devices_copy) {
                device = p;
                if(string(p->info.acSn) == string(info.acSn))
                    goto OLD_DEVICE;
            }

            if(series.find("GIGE") != -1) {
                char camIp[16],camMask[16],camGateWay[16],etIp[16],etMask[16],etGateWay[16];
                CameraGigeGetIp(cameraEnumList + i,camIp,camMask,camGateWay,etIp,etMask,etGateWay);

                shared_ptr<GIGEDevice> p;
                if(string(info.acProductName).find("MV-GF") != -1) {
                    p = std::dynamic_pointer_cast<GIGEDevice>(make_shared<GF120>());
                } else {
                    p = make_shared<GIGEDevice>();
                }

                device = std::dynamic_pointer_cast<Device>(p);
                p->camIp = camIp;
                p->camMask = camMask;
                p->camGateWay = camGateWay;
                p->etIp = etIp;
                p->etMask = etMask;
                p->etGateWay = etGateWay;
            } else if (memcmp(info.acPortType, "USB3", 4) == 0) {
                device = std::dynamic_pointer_cast<Device>(make_shared<USB3Device>());
            } else {
                device = std::dynamic_pointer_cast<Device>(make_shared<USB2Device>());
            }

            device->info = cameraEnumList[i];

OLD_DEVICE:
            devices.push_back(device);
        }

        return devices;
    }

    std::list<shared_ptr<Device>> devices;
};

#endif // DEVICEMANAGER_H
