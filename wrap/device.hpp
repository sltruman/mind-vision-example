#ifndef DEVICE_H
#define DEVICE_H

#include "defectpixelalg.hpp"
#include "brightness.hpp"

#include <CameraApi.h>

#include <stdexcept>
#include <vector>
#include <sstream>
#include <string>
#include <ctime>
#include <regex>
#include <list>
#include <array>

using std::runtime_error;
using std::vector;
using std::string;
using std::stringstream;
using std::endl;
using std::ios;
using std::regex;
using std::list;
using std::array;

#include <memory.h>

struct Device {
    Device() {

    }

    ~Device() {}

    tSdkCameraDevInfo info;

    bool busying() {
        int busying = 0;
        return CAMERA_STATUS_SUCCESS == CameraIsOpened(&info,&busying) && busying;
    }

    bool opened() {
        return handle;
    }

    CameraHandle handle = 0;
    tSdkCameraCapbility capability;
    vector<unsigned char> frame_buffer;
    tSdkFrameHead frame_head;

    bool open() {
        auto status = CameraInit(&info,-1,-1,&handle);

        if(status != CAMERA_STATUS_SUCCESS) {
            return false;
        }

        status = CameraGetCapability(handle,&capability);

        if(status != CAMERA_STATUS_SUCCESS) {
            return false;
        }

        if(capability.sIspCapacity.bMonoSensor) status = CameraSetIspOutFormat(handle,CAMERA_MEDIA_TYPE_MONO8);
        else status = CameraSetIspOutFormat(handle,CAMERA_MEDIA_TYPE_RGB8);

        if(status != CAMERA_STATUS_SUCCESS) {
            return false;
        }

        auto length_max = capability.sResolutionRange.iWidthMax * capability.sResolutionRange.iHeightMax * (capability.sIspCapacity.bMonoSensor ? 1 : 3);
        frame_buffer.assign(length_max,0);

        status_sync();
        return true;
    }

    void close() {
        CameraUnInit(handle);
        handle = 0;
    }

    bool playing;
    void play() {
        playing = true;
        CameraPlay(handle);
    }

    void pause() {
        playing = false;
        CameraPause(handle);
    }

    void stop() {
        playing = false;
        CameraStop(handle);
    }

    bool snapshoting = false;
    int snapshot_period = -1;
    string snapshot_dir;
    int snapshot_format;
    void snapshot_start(string dir,int format,int period) {
        snapshoting = true;
        snapshot_period = period;
        snapshot_dir = dir;
        snapshot_format = format;
    }

    void snapshot_stop() {
        snapshoting = false;
    }

    bool recording;
    int record_period;
    string record_dir;
    int record_format;
    int record_quality;
    void record_start(string dir,int format,int quality) {
        recording = true;
        record_dir = dir;
        record_format = format;
        record_quality = quality;

        stringstream filename;
        filename << dir << "/mind-vision-" << std::time(0);

        CameraInitRecord(handle,format,const_cast<char*>(filename.str().c_str()),0,quality,100);
    }

    void record_stop() {
        recording = false;
        CameraStopRecord(handle);
    }

    clock_t frame_clock_later;
    auto& frame() {
        unsigned char* raw_buffer = nullptr;

        if (CAMERA_STATUS_SUCCESS != CameraGetImageBufferPriority(handle,&frame_head,&raw_buffer,2000,CAMERA_GET_IMAGE_PRIORITY_NEWEST))
            return frame_buffer;

        CameraImageProcess(handle,raw_buffer,frame_buffer.data(),&frame_head);

        if(snapshoting && (snapshot_period > frame_clock_later || snapshot_period == -1)) {
            stringstream filename; filename << snapshot_dir << "/mind-vision-" << std::time(0);
            CameraSaveImage(handle, const_cast<char*>(filename.str().c_str()), frame_buffer.data(), &frame_head, snapshot_format, 100);
            if(snapshot_period == -1) snapshoting = false;
        }

        if(recording) CameraPushFrame(handle,frame_buffer.data(),&frame_head);

#ifdef WIN32
        CameraFlipFrameBuffer(frame_buffer.data(),&frame_head,1);
#endif

        CameraReleaseImageBuffer(handle,raw_buffer);
        frame_clock_later = std::clock();
        return frame_buffer;
    }

    stringstream status_string;
    virtual void status_sync() {}

    auto exposure(int full) {
        BOOL            mode = 0;
        int             brightness = 0;
        double          exposureTime = 0;
        int             analogGain = 0;
        BOOL            flicker = 0;
        int             frequencySel = 0;
        double	        expLineTime = 0; //当前的行曝光时间，单位为us
        double exposureRangeMinimum=0.,exposureRangeMaximum=0.;
        int uiAnalogGainMin=0,uiAnalogGainMax=0;
        double exposureRangeMinimum2=0.,exposureRangeMaximum2=0.;

        int threshould = 0;
        int x,y,w,h;

        if(full) {
            CameraGetAeState(handle,&mode);
            CameraGetAeTarget(handle,&brightness);
            CameraGetAntiFlick(handle,&flicker);
            CameraGetLightFrequency(handle,&frequencySel);
            CameraGetExposureTimeRange(handle,&exposureRangeMinimum,&exposureRangeMaximum,&expLineTime);
            CameraGetAeThreshold(handle,&threshould);
            CameraGetAeWindow(handle,&x,&y,&w,&h);
            CameraGetAeAnalogGainRange(handle,&uiAnalogGainMin,&uiAnalogGainMax);
            CameraGetAeExposureRange(handle,&exposureRangeMinimum2,&exposureRangeMaximum2);
        }

        CameraGetAnalogGain(handle,&analogGain);
        CameraGetExposureTime(handle,&exposureTime);

        stringstream ss;
        ss.precision(2);
        ss << "True\n"
             << mode << ','
             << capability.sExposeDesc.uiTargetMin << ','
             << capability.sExposeDesc.uiTargetMax << ','
             << brightness << ','
             << flicker << ','
             << frequencySel << ','
             << threshould << ','
             << (int)(capability.sExposeDesc.fAnalogGainStep * capability.sExposeDesc.uiAnalogGainMin * 100.) << ','
             << (int)(capability.sExposeDesc.fAnalogGainStep * capability.sExposeDesc.uiAnalogGainMax * 100.) << ','
             << (int)(capability.sExposeDesc.fAnalogGainStep * analogGain * 100.) << ','
             << (int)(capability.sExposeDesc.uiExposeTimeMin * expLineTime) << ','
             << (int)(capability.sExposeDesc.uiExposeTimeMax * expLineTime) << ','
             << (int)exposureTime << ','
             << (int)exposureRangeMinimum << ','
             << (int)exposureRangeMaximum << ","
             << (int)(capability.sExposeDesc.fAnalogGainStep * uiAnalogGainMin * 100.) << ','
             << (int)(capability.sExposeDesc.fAnalogGainStep * uiAnalogGainMax * 100.) << ','
             << (int)exposureRangeMinimum2 << ','
             << (int)exposureRangeMaximum2 << ",\n"
             << x << ',' << y << ',' << w << ',' << h << ','
             << endl;

        return ss.str();
    }

    void exposure_mode(int value) {
        CameraSetAeState(handle,value);
    }

    void brightness(int value) {
        CameraSetAeTarget(handle,value);
    }

    void threshold(int value) {
        CameraSetAeThreshold(handle,value);
    }

    void flicker(int value) {
        CameraSetAntiFlick(handle,value);
    }

    void gain(int value) {
        CameraSetAnalogGain(handle,value / capability.sExposeDesc.fAnalogGainStep / 100.f);
    }

    void gain_range(int minimum,int maximum) {
        CameraSetAeAnalogGainRange(handle,minimum / capability.sExposeDesc.fAnalogGainStep / 100.f,maximum / capability.sExposeDesc.fAnalogGainStep / 100.f);
    }

    void exposure_time(int pos) {
        CameraSetExposureTime(handle,pos);
    }

    void exposure_time_range(double minimum,double maximum) {
        CameraSetAeExposureRange(handle,minimum,maximum);
    }

    void frequency(int value) {
        CameraSetLightFrequency(handle,value);
    }

    void exposure_window(int x,int y,int w,int h) {
        CameraSetAeWindow(handle,x,y,w,h);
    }

    auto white_balance() {
        int saturation=0;
        BOOL mode= -1;
        int r=0,g=0,b=0;
        int algorithm = -1;
        BOOL monochrome = 0,inverse = 0;
        int color_temrature = -1;
        int color_temrature_mode;

        CameraGetWbMode(handle,&mode);
        CameraGetGain(handle,&r,&g,&b);
        CameraGetSaturation(handle,&saturation);
        CameraGetMonochrome(handle,&monochrome);
        CameraGetInverse(handle,&inverse);
        CameraGetBayerDecAlgorithm(handle,ISP_PROCESSSOR_PC,&algorithm);
        CameraGetClrTempMode(handle,&color_temrature_mode);
        switch(color_temrature_mode) {
        case CT_MODE_AUTO:
            color_temrature = 0;
            break;
        case CT_MODE_PRESET:
            CameraGetPresetClrTemp(handle,&color_temrature);
            color_temrature += 1;
            break;
        case CT_MODE_USER_DEF:
            color_temrature = 4;
            break;
        }

        int x,y,w,h;
        CameraGetWbWindow(handle,&x,&y,&w,&h);

        stringstream colorTemplates;
        colorTemplates << "Automation,";
        for(auto i=0;i < capability.iClrTempDesc;i++)
            colorTemplates << capability.pClrTempDesc[i].acDescription << ',';

        constexpr int IO_CONTROL_ENABLE_FPN = 37; // 启用、禁用FPN
        int fpn = 0;
        CameraSpecialControl(handle, IO_CONTROL_ENABLE_FPN, 0, &fpn);

        stringstream ss;
        ss << "True" << '\n'
             << mode << ','
             << capability.sRgbGainRange.iRGainMin << ',' << capability.sRgbGainRange.iRGainMax << ',' << r << ','
             << capability.sRgbGainRange.iGGainMin << ',' << capability.sRgbGainRange.iGGainMax << ',' << g << ','
             << capability.sRgbGainRange.iBGainMin << ',' << capability.sRgbGainRange.iBGainMax << ',' << b << ','
             << capability.sSaturationRange.iMin << ',' << capability.sSaturationRange.iMax << ',' << saturation << ','
             << monochrome << ',' << inverse << ',' << algorithm << ',' << color_temrature << ','
             << x << ',' << y << ',' << w << ',' << h << ',' << fpn << '\n'
             << colorTemplates.str()
             << endl;

        return ss.str();
    }


    void white_balance_mode(int index) {
        CameraSetWbMode(handle,index);
    }

    void color_temrature(int index) {
        switch(index) {
        case 0:
            CameraSetClrTempMode(handle,CT_MODE_AUTO);
            CameraSetOnceWB(handle);
            break;
        case 1:
        case 2:
        case 3:
            CameraSetClrTempMode(handle,CT_MODE_PRESET);
            CameraSetPresetClrTemp(handle,index - 1);
            break;
        case 4:
            CameraSetClrTempMode(handle,CT_MODE_USER_DEF);
            break;
        }
    }

    void once_white_balance() {
        CameraSetOnceWB(handle);

    }

    void white_balance_window(int x,int y,int w,int h) {
        CameraSetWbWindow(handle,x,y,w,h);
    }

    void rgb(int r,int g,int b) {
        CameraSetGain(handle,r,g,b);
    }

    void saturation(int value) {
        CameraSetSaturation(handle,value);
    }

    void monochrome(int enable) {
        CameraSetMonochrome(handle,enable);
    }

    void inverse(int enable) {
        CameraSetInverse(handle,enable);
    }

    void algorithm(int index) {
#ifdef WIN32
        CameraSetBayerDecAlgorithm(handle,ISP_PROCESSSOR_PC,index);
#endif
    }

    auto lookup_table_mode() {
        int mode = -1;
        CameraGetLutMode(handle,&mode);
        return mode;
    }

    void lookup_table_mode(int index) {
        CameraSetLutMode(handle,index);
    }

    auto lookup_tables_for_dynamic() {
        int                 gamma=0;
        int                 contrast=0;
        unsigned short r[4096];

        CameraGetGamma(handle,&gamma);
        CameraGetContrast(handle,&contrast);
        CameraGetCurrentLut(handle,LUT_CHANNEL_ALL,r);

        stringstream ss;
        for(auto i=0;i < 4096;i++)
            ss << r[i] << ',';
        ss.seekp(-1,ios::end);
        ss << " ";

        stringstream res;
        res << "True "
             << capability.sGammaRange.iMin << ' ' << capability.sGammaRange.iMax << ' ' << gamma << ' '
             << capability.sContrastRange.iMin << ' ' << capability.sContrastRange.iMax << ' ' << contrast << ' '
             << ss.str()
             << endl;

        return res.str();
    }

    auto lookup_tables_preset() {
        int preset = -1;
        unsigned short r[4096];
        CameraGetLutPresetSel(handle,&preset);
        CameraGetCurrentLut(handle,LUT_CHANNEL_ALL,r);

        stringstream ss;
        for(auto i=0;i < 4096;i++)
            ss << r[i] << ',';
        ss.seekp(-1,ios::end);
        ss << " ";

        stringstream res;
        res << "True "
             << preset << ' '
             << ss.str()
             << endl;

        return res.str();
    }

    void lookup_table_preset(int index) {
        CameraSelectLutPreset(handle,index);
    }

    auto lookup_tables_for_custom(int index) {
        USHORT r[4096];
        CameraGetCustomLut(handle,index,r);

        stringstream ss;
        for(auto i=0;i < 4096;i++)
            ss << r[i] << ',';
        ss.seekp(-1,ios::end); ss << " ";

        stringstream res;
        res << "True "
             << ss.str()
             << endl;

        return res.str();
    }

    void gamma(int value) {
        CameraSetGamma(handle,value);
    }

    void contrast_ratio(int value) {
        CameraSetContrast(handle,value);
    }

    auto isp() {
        BOOL        m_bHflip=FALSE,m_bHflipHard=-1;
        BOOL        m_bVflip=FALSE,m_bVflipHard=-1;
        int         m_Sharpness=0,noise = 0,noise3D = 0,count = 0,weight =0,rotate=0;
        float weights ;

        //获得图像的镜像状态。
        CameraGetMirror(handle, MIRROR_DIRECTION_HORIZONTAL, &m_bHflip);
        CameraGetMirror(handle, MIRROR_DIRECTION_VERTICAL,   &m_bVflip);

        //获取当前锐化设定值。
        CameraGetSharpness(handle, &m_Sharpness);
        CameraGetNoiseFilterState(handle,&noise );
        CameraGetDenoise3DParams(handle,&noise3D,&count,&weight,&weights);
        CameraGetRotate(handle,&rotate);
        int flat_field_corrent=0;
        CameraFlatFieldingCorrectGetEnable(handle,&flat_field_corrent);

        int dead_pixels_correct=0;
        CameraGetCorrectDeadPixel(handle,&dead_pixels_correct);

        int undistort;
        CameraGetUndistortEnable(handle,&undistort);

        CameraGetHardwareMirror(handle, MIRROR_DIRECTION_HORIZONTAL, &m_bHflipHard);
        CameraGetHardwareMirror(handle, MIRROR_DIRECTION_VERTICAL,   &m_bVflipHard);

        stringstream res;
        res  << m_bHflip << ' ' << m_bVflip << ' '
             << capability.sSharpnessRange.iMin << ' ' <<  capability.sSharpnessRange.iMax << ' ' << m_Sharpness << ' '
             << noise << ' ' << noise3D << ' ' << count << ' '
             << rotate << ' '
             << flat_field_corrent << ' '
             << dead_pixels_correct << ' '
             << '?' << ' '
             << undistort << ' '
             << m_bHflipHard << ' ' << m_bVflipHard
             << endl;

        return res.str();
    }

    auto dead_pixels() {
#ifdef WIN32
        unsigned int pixels_count = 0;
        CameraReadDeadPixels(handle,nullptr,nullptr,&pixels_count);

        vector<unsigned short> x_array(pixels_count),y_array(pixels_count);
        CameraReadDeadPixels(handle,y_array.data(),x_array.data(),&pixels_count);

        list<array<int,2>> f;

        for(int i=0;i < x_array.size();i++)
            f.push_back({x_array[i],y_array[i]});
#endif
        return f;
    }

    void horizontal_mirror(int hard,int value) {
        if(hard) {
            CameraSetHardwareMirror(handle, MIRROR_DIRECTION_HORIZONTAL, value);
        } else {
            CameraSetMirror(handle, MIRROR_DIRECTION_HORIZONTAL, value);
        }
    }

    void vertical_mirror(int hard,int value) {
        if(hard) {
            CameraSetHardwareMirror(handle, MIRROR_DIRECTION_VERTICAL, value);
        } else {
            CameraSetMirror(handle, MIRROR_DIRECTION_VERTICAL, value);
        }

    }

    void acutance(int value) {
        CameraSetSharpness(handle,value);

    }

    void noise(int enable) {
        CameraSetNoiseFilter(handle,enable);

    }

    void noise3d(int enable,int value) {
        CameraSetDenoise3DParams(handle,enable,value,nullptr);

    }

    void rotate(int value) {
        CameraSetRotate(handle,value);
    }

    void flat_field_corrent(int enable) {
        CameraFlatFieldingCorrectSetEnable(handle,enable);
    }

    vector<unsigned char> dark_buffer,light_buffer;
    tSdkFrameHead light_frame_head,dark_frame_head;
    bool flat_field_init(int light) {
        tSdkFrameHead	frameHead;
        BYTE			*rawBuffer;
        unsigned char* rgb_buffer;

        auto status = CameraGetImageBuffer(handle,&frameHead,&rawBuffer,2000);
        if(status != CAMERA_STATUS_SUCCESS) {
            return false;
        }

        if(light) {
            if(light_buffer.size()) light_buffer.assign(0,0);
            light_buffer.assign(frameHead.uBytes,0);
            rgb_buffer = light_buffer.data();
            light_frame_head = frameHead;
        } else {
            if(dark_buffer.size()) dark_buffer.assign(0,0);
            light_buffer.assign(0,0);
            dark_buffer.assign(frameHead.uBytes,0);
            rgb_buffer = dark_buffer.data();
            dark_frame_head = frameHead;
        }

        memcpy(rgb_buffer,rawBuffer,frameHead.uBytes);
        CameraReleaseImageBuffer(handle, rawBuffer);

        if(dark_buffer.size() && light_buffer.size()) {
            status = CameraFlatFieldingCorrectSetParameter(handle,dark_buffer.data(),&dark_frame_head,light_buffer.data(),&light_frame_head);
        }

        if(status != CAMERA_STATUS_SUCCESS) {
            return false;
        }

        return true;
    }

    void flat_field_params_save(string filepath) {
        CameraFlatFieldingCorrectSaveParameterToFile(handle,filepath.c_str());
    }

    void flat_field_params_load(string filepath) {
        CameraFlatFieldingCorrectLoadParameterFromFile(handle,filepath.c_str());
    }

    void dead_pixels_correct(int enable) {
        CameraSetCorrectDeadPixel(handle,enable);

    }

    void dead_pixels(string x_list,string y_list) {
#ifdef WIN32
        CameraRemoveAllDeadPixels(handle);

        if(x_list != "None") {
            regex p(R"(,)");

            std::sregex_token_iterator x_begin(x_list.begin(),x_list.end(),p,-1),y_begin(y_list.begin(),y_list.end(),p,-1),end;
            vector<unsigned short> x_array,y_array;

            std::transform(x_begin,end,back_inserter(x_array),[](auto x)->unsigned short {
                return (unsigned short)stoi(x);
            });

            std::transform(y_begin,end,back_inserter(y_array),[](auto y)->unsigned short {
                return (unsigned short)stoi(y);
            });

            CameraAddDeadPixels(handle,y_array.data(),x_array.data(),x_array.size());
        }

        CameraSaveDeadPixels(handle);
#endif
    }

    auto dead_pixels_analyze_for_bright(int threshold) {
        vector<MvPoint16> brightPixels;

        tSdkFrameHead frameHead;
        unsigned char* rawBuffer = nullptr;
        CameraGetImageBuffer(handle,&frameHead,&rawBuffer,10000);
        AnalyzeBrightPixelFromImage(brightPixels,rawBuffer,threshold,frameHead.iWidth,frameHead.iHeight);
        CameraReleaseImageBuffer(handle,rawBuffer);

        stringstream f;

        for(auto pixel : brightPixels) {
            f << pixel.x << ',' << pixel.y << ",\n";
        }

        return f.str();
    }

    auto dead_pixels_analyze_for_dead(int threshold) {
        vector<MvPoint16> deadPixels;

        tSdkFrameHead frameHead;
        unsigned char* rawBuffer = nullptr;
        CameraGetImageBuffer(handle,&frameHead,&rawBuffer,10000);

        AnalyzeDefectPixelFromImage(deadPixels,rawBuffer,threshold,frameHead.iWidth,frameHead.iHeight);
        CameraReleaseImageBuffer(handle,rawBuffer);

        stringstream f;

        for(auto pixel : deadPixels) {
            f << pixel.x << ',' << pixel.y << ",\n";
        }

        return f.str();
    }

    void undistort(int enable) {
        CameraSetUndistortEnable(handle,enable);
    }

    void undistory_params(int w,int h,string camera_matrix,string distort_coeffs) {
        regex p(R"(,)");

        std::sregex_token_iterator m1_begin(camera_matrix.begin(),camera_matrix.end(),p,-1),m2_begin(distort_coeffs.begin(),distort_coeffs.end(),p,-1),end;
        vector<double> m1,m2;

        std::transform(m1_begin,end,back_inserter(m1),[](auto v) { return (double)stod(v); });
        std::transform(m2_begin,end,back_inserter(m2),[](auto v) { return (double)stod(v); });

        CameraSetUndistortParams(handle,w,h,m1.data(),m2.data());
    }

    auto video() {
        int speed=0,hz=0,index=-1,bits=3,max_bits=8;
        CameraGetFrameSpeed(handle,&speed);
        CameraGetFrameRate(handle,&hz);
        CameraGetMediaType(handle,&index);

#ifdef WIN32
        CameraGetRawStartBit(handle,&bits);
        CameraGetRawMaxAvailBits(handle,&max_bits);
#endif

        stringstream ss;

        for(int i=0;i<capability.iMediaTypdeDesc;i++)
            ss << capability.pMediaTypeDesc[i].acDescription << ',';

        stringstream frameSpeedTypes;

        for(int i=0;i < capability.iFrameSpeedDesc;i++)
            frameSpeedTypes << capability.pFrameSpeedDesc[i].acDescription << "Speed" << ',';

        stringstream res;
        res << "True\n"
             << speed << ',' << hz << ',' << index << ',' << bits << ',' << max_bits << ",\n"
             << ss.str() << '\n'
             << frameSpeedTypes.str()
             << endl;
        return res.str();
    }

    void frame_rate_speed(int index) {
        CameraSetFrameSpeed(handle,index);
    }

    void frame_rate_limit(int value) {
        CameraSetFrameRate(handle,value);

    }

    void raw_output_format(int index) {
        CameraSetMediaType(handle,index);
    }

    void raw_output_range(int value) {
#ifdef WIN32
        CameraSetRawStartBit(handle,value);
#endif
    }

    auto resolutions() {
        tSdkImageResolution resolution;
        CameraGetImageResolution(handle,&resolution);

        stringstream ss,ss2;
        for(auto i=0;i < capability.iImageSizeDesc;i++)
            ss << capability.pImageSizeDesc[i].acDescription << ',';
        ss.seekp(-1,ios::end); ss << ",";

        ss2 << resolution.iHOffsetFOV << ',' << resolution.iVOffsetFOV << ',' << resolution.iWidth << ',' << resolution.iHeight;


        stringstream res;
        res << "True" << endl
             << (resolution.iIndex != 0xFF ? 0 : 1) << ',' << resolution.iIndex << '\n'
             << ss2.str() << '\n'
             << ss.str()
             << endl;
        return res.str();
    }

    void resolution(int index) {
        CameraSetImageResolution(handle,&capability.pImageSizeDesc[index]);
    }

    void resolution(int x,int y,int w,int h) {
        w = w < 16 ? 16 : w;
        h = h < 4 ? 4 : h;
        x -= x % 16;
        y -= y % 4;
        w -= w % 16;
        h -= h % 4;

        tSdkImageResolution sRoiResolution = { 0 };
        sRoiResolution.iIndex = 0xff;
        sRoiResolution.iWidth = w;
        sRoiResolution.iWidthFOV = w;
        sRoiResolution.iHeight = h;
        sRoiResolution.iHeightFOV = h;
        sRoiResolution.iHOffsetFOV = x;
        sRoiResolution.iVOffsetFOV = y;
        sRoiResolution.iWidthZoomSw = 0;
        sRoiResolution.iHeightZoomSw = 0;
        sRoiResolution.uBinAverageMode = 0;
        sRoiResolution.uBinSumMode = 0;
        sRoiResolution.uResampleMask = 0;
        sRoiResolution.uSkipMode = 0;

        CameraSetImageResolution(handle, &sRoiResolution);
    }

    auto io() {
        unsigned int state;
        int mode;

        stringstream res;
        for(int i=0;i<capability.iInputIoCounts;i++) {
            CameraGetInPutIOMode(handle,i,&mode);
            CameraGetIOState(handle,i,&state);
            res << "Input," << mode << ',' << state << ",\n";
        }

        for(int i=0;i<capability.iOutputIoCounts;i++) {
            CameraGetOutPutIOMode(handle,i,&mode);
            CameraGetOutPutIOState(handle,i,&state);
            res << "Output," << mode << ',' << state << ",\n";
        }

        return res.str();
    }

    void io_mode(string type,int index,int value) {
        if(type=="Input")
            CameraSetInPutIOMode(handle,index,value);
        else
            CameraSetOutPutIOMode(handle,index,value);
    }

    void io_state(string type,int index,int value) {
        if(type=="Input") {}
        else CameraSetIOStateEx(handle,index,value);
    }

    auto controls() {
        int  trigger_mode=0,trigger_count=0,trigger_signal=0;
        unsigned int trigger_delay=0,trigger_interval=0;
        int trigger_type=0;
        unsigned int trigger_jitter=0;
        CameraGetTriggerMode(handle,&trigger_mode);
        CameraGetTriggerCount(handle,&trigger_count);
        CameraGetTriggerDelayTime(handle,&trigger_delay);
        CameraGetExtTrigIntervalTime(handle,&trigger_interval);
        CameraGetExtTrigSignalType(handle,&trigger_signal);
        CameraGetExtTrigShutterType(handle,&trigger_type);
        CameraGetExtTrigJitterTime(handle,&trigger_jitter);

        int strobe_mode=0,strobe_polarity=0;
        unsigned int strobe_delay=0,strobe_pulse_width =0;
        CameraGetStrobeMode(handle,&strobe_mode);
        CameraGetStrobePolarity(handle,&strobe_polarity);
        CameraGetStrobeDelayTime(handle,&strobe_delay);
        CameraGetStrobePulseWidth(handle,&strobe_pulse_width);

        UINT mask;
        CameraGetExtTrigCapability(handle,&mask);

        stringstream signalTypes,shutterTypes;
        signalTypes << "RisingEdge,FallingEdge,";
        if(mask & EXT_TRIG_MASK_LEVEL_MODE) signalTypes << "HighLevel,LowLevel,";
        if(mask & EXT_TRIG_MASK_DOUBLE_EDGE) signalTypes << "DoubleEdge,";

        shutterTypes << "Standard,";
        if(mask & EXT_TRIG_MASK_GRR_SHUTTER) shutterTypes << "GRR,";

        stringstream res;
        res << "True" << endl
             << trigger_mode << ','
             << trigger_count << ','
             << trigger_delay << ','
             << trigger_interval << ','
             << trigger_signal << ','
             << trigger_jitter << ','
             << strobe_mode << ','
             << strobe_polarity << ','
             << strobe_delay << ','
             << strobe_pulse_width << ','
             << trigger_type << ',' << '\n'
             << signalTypes.str() << '\n'
             << shutterTypes.str()
             << endl;
        return res.str();
    }

    void trigger_mode(int value) {
        CameraSetTriggerMode(handle,value);
    };

    void once_soft_trigger() {
        CameraSoftTrigger(handle);
    }

    void trigger_frames(int value) {
        CameraSetTriggerCount(handle,value);
    }

    void trigger_delay(unsigned int value) {
        CameraSetTriggerDelayTime(handle,value);
    }

    void trigger_interval(unsigned int value) {
        CameraSetExtTrigIntervalTime(handle,value);
    }

    void outside_trigger_mode(int value) {
        CameraSetExtTrigSignalType(handle,value);
    }

    void outside_trigger_debounce(unsigned int value) {
        CameraSetExtTrigJitterTime(handle,value);
    }

    void outside_shutter(int index) {
        CameraSetExtTrigShutterType(handle,index);
    }

    void strobe_mode(int value) {
        CameraSetStrobeMode(handle,value);
    }

    void strobe_polarity(int value) {
        CameraSetStrobePolarity(handle,value);
    }

    void strobe_delay(unsigned int value) {
        CameraSetStrobeDelayTime(handle,value);
    }

    void strobe_pulse(unsigned int value) {
        CameraSetStrobePulseWidth(handle,value);
    }

    auto firmware() {
        char firmware[256],iface[256],sdk[256];
        CameraGetFirmwareVersion(handle,firmware);
        CameraGetInerfaceVersion(handle,iface);
        CameraSdkGetVersionString(sdk);
        int updatable=0;
        CameraCheckFwUpdate(handle,&updatable);
        char nickname[256];
        CameraGetFriendlyName(handle,nickname);

        stringstream res;
        res << "True" << endl
             << firmware << ','
             << iface << ','
             << sdk << ','
             << info.acDriverVersion << ','
             << updatable << ','
             << nickname << ','
             << info.acSn << endl;
        return res.str();
    }

    string name() {
        char nickname[256];
        CameraGetFriendlyName(handle,nickname);
        return nickname;
    }

    void rename(string name) {
        CameraSetFriendlyName(handle,const_cast<char*>(name.c_str()));
    }

    void params_reset() {
        CameraLoadParameter(handle,PARAMETER_TEAM_DEFAULT);
    }

    void params_save(int value) {
        CameraSaveParameter(handle,value);
    }

    void params_load(int value) {
        CameraLoadParameter(handle,value);
    }

    void params_save_to_file(string filename) {
        CameraSaveParameterToFile(handle,const_cast<char*>(filename.c_str()));
    }

    void params_load_from_file(string filename) {
        CameraReadParameterFromFile(handle,const_cast<char*>(filename.c_str()));
    }

    auto brightness() {
        UINT acc_y = 0;

        int mode=1;
        CameraGetTriggerMode(handle,&mode);
        if(playing && mode == 0) {
            YAcc(frame_buffer.data(),&frame_head,&acc_y);
        }

        return acc_y;
    }

    void fpn_save(string filepath) {
        constexpr int IO_CONTROL_SAVE_FPN_TO_FILE = 33;     // 把相机里的FPN数据存到文件
        CameraSpecialControl(handle, IO_CONTROL_SAVE_FPN_TO_FILE, 0, (void*)filepath.c_str());
    }

    void fpn_load(string filepath) {
        constexpr int IO_CONTROL_LOAD_FPN_TO_DEVICE = 34;   // 从文件加载FPN数据到相机里
        CameraSpecialControl(handle, IO_CONTROL_LOAD_FPN_TO_DEVICE, 0, (void*)filepath.c_str());
    }

    void fpn_clear() {
        constexpr int IO_CONTROL_DELETE_FPN = 35;           // 删除相机里的FPN数据
        CameraSpecialControl(handle, IO_CONTROL_DELETE_FPN, 0, NULL);
    }

    void fpn(int e) {
        constexpr int IO_CONTROL_ENABLE_FPN = 37; // 启用、禁用FPN
        CameraSpecialControl(handle, IO_CONTROL_ENABLE_FPN, e, NULL);
    }

};

#endif // DEVICE_H
