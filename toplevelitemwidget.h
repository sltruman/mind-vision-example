#ifndef TOPLEVELITEMWIDGET_H
#define TOPLEVELITEMWIDGET_H

#include "cameraview.h"
#include "calibrationdialog.h"
#include "cameraprocess.h"
#include "GF120.h"

#include <QWidget>
#include <QTreeWidgetItem>
#include <QTimer>

namespace Ui {
class TopLevelItemWidget;
}

class TopLevelItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TopLevelItemWidget(QTreeWidgetItem *topLevelItem,QString series,QTreeWidget *parent);
    virtual ~TopLevelItemWidget();

    QTreeWidgetItem* topLevelItem;
    QString series;
    QTimer t;

public slots:
    void on_toolButton_refresh_clicked();
    void statusUpdate();

private:
    Ui::TopLevelItemWidget *ui;
    QTreeWidget* parent;
};

class DeviceItem : public QTreeWidgetItem
{
public:
    DeviceItem(TopLevelItemWidget *w,QTreeWidgetItem *parent, QString name);
    virtual ~DeviceItem();


public:
    bool open();
    void close();

    std::tuple<QStringList,QStringList> exposure(int full = true);
    void exposureMode(int value);
    void brightness(int value);
    void threshold(int value);
    void flicker(int value);
    void gain(int value);
    void gainRange(int minimum,int maximum);
    void exposureTime(int value);
    void exposureTimeRange(int minimum,int maximum);
    void frequency(int value);
    void exposureWindow(int x,int y,int w,int h);

    std::tuple<QStringList,QStringList> whiteBalance();
    void whiteBalanceMode(int index);
    void onceWhiteBalance();
    void whiteBalanceWindow(int x,int y,int w,int h);
    void rgb(int r,int g,int b);
    void saturation(int value);
    void monochrome(int enable);
    void inverse(int enable);
    void algorithm(int index);
    void colorTemrature(int index);

    QString lookupTableMode();
    void lookupTableMode(int index);
    QStringList lookupTablesForDynamic();
    void gamma(int value);
    void contrastRatio(int value);
    QStringList lookupTablesForPreset();
    QStringList lookupTablesForCustom(int index);
    void lookupTablePreset(int index);

    QStringList isp();
    void horizontalMirror(int hard,int value);
    void verticalMirror(int hard,int value);
    void acutance(int value);
    void noise(int enable);
    void noise3d(int enable,int value);
    void rotate(int value);
    void deadPixels(QString x,QString y);
    QStringList dead_pixels_analyze_for_bright(int threshold);
    QStringList dead_pixels_analyze_for_dead(int threshold);
    void flatFieldCorrent(int enable);
    void flatFieldInit(int light);
    void flatFieldParamsSave(QString filepath);
    void flatFieldParamsLoad(QString filepath);
    void undistort(int enable);
    void undistortParams(int w,int h,QString cameraMatrix,QString distortCoeffs);

    std::tuple<QStringList,QStringList,QStringList> video();
    void frameRateSpeed(int index);
    void frameRateLimit(int value);
    void rawOutputFormat(int index);
    void rawOutputRange(int value);

    QString resolutionMode();
    QString resolutionIndex();
    QStringList resolution();
    QStringList resolutions();
    void resolution(int value);
    void resolution(int x,int y,int w,int h);

    QStringList io();
    void ioMode(QString type,int index,int value);
    void ioState(QString type,int index,int value);

    std::tuple<QStringList,QStringList,QStringList> controls();
    void triggerMode(int);
    void onceSoftTrigger();
    void triggerFrames(int);
    void triggerDelay(int);
    void triggerInterval(int);
    void outsideTriggerMode(int);
    void outsideTriggerDebounce(int);
    void outsideShutter(int);
    void flashMode(int);
    void flashPolarity(int);
    void strobeDelay(int);
    void strobePulse(int);

    QStringList firmware();
    void rename(QString name);
    void paramsReset();
    void paramsSave(int value);
    void paramsLoad(int value);
    void paramsSaveToFile(QString filename);
    void paramsLoadFromFile(QString filename);

    void snapshotStart(QString dir,int resolution,int format,int period);
    bool snapshotState();
    void snapshotStop();
    void recordStart(QString dir,int format,int quality,int frames);
    bool recordState();
    void recordStop();

    QStringList status(QString type);
    QString brightness();

    void fpn(int enable);
    void fpnClear();
    void fpnLoad(QString filepath);
    void fpnSave(QString filepath);

    void infrared_thermometry(int index);
    void infrared_color(int index);
    void infrared_display(int index);
    void infrared_shutter(int checked);
    void infrared_cool(int checked);
    void infrared_emissivity(int value);
    void infrared_sharpen(int value);
    void infrared_dde(int value);
    void infrared_exposure(int value);
    InfraredStatus infrared_status();
    void infrared_manual(int checked,short value);
    void infrared_temperature_check();
    void infrared_stop_temperature_check(bool checked);
    void infrared_factory_check_temperature_check_stop();
    void infrared_shutter_temperature_raise_sample(bool checked);
    void infrared_factory_check_detect(bool checked);
    void infrared_response_rate_sample(bool checked);
    void infrared_temperature_curve_sample(bool checked);
    void infrared_factory_check();
    void infrared_frame_temp_cnt(int);
    void infrared_factory_check_exposure(int);

    void infrared_sample_path(QString path);

    InfraredParamsStatus infrared_params_status();
    void infrared_params_status(InfraredParamsStatus);

    void infrared_response_rate_start(int value,QString path);
    bool infrared_response_rate_status();
    void infrared_response_rate_stop();
    bool infrared_load_response_rate_file(QString path,QString path2);

    void infrared_cover_start(int value,QString path);
    bool infrared_cover_status();
    void infrared_cover_stop();
    bool infrared_load_cove_file(QString path,QString path2);

    bool infared_save_config(QStringList);
    bool infared_delete_config();
    bool infrared_cmd(QString cmd);

    void infrared_osd(bool checked);
    void infrared_temperature_display(bool checked);

    InfraredTemperatureROIStatus infrared_temperature_roi_status(int index);
    void infrared_roi(bool checked,int index,int user_width_start,int user_width_number,int user_high_start,int user_high_number,int user_roi_emissivity);
    void infrared_blackbody_calibrate(bool checked,int blackbody_temprature,int user_width_start,int user_width_end,int user_high_start,int user_high_end);
    void infrared_color_map(bool checked,int low,int high);
    void infrared_temperature_compensation(int value);
    void infrared_distance_compensation(int value);
    void infrared_humidity_compensation(int value);
    void infrared_high_warm(bool checked,int temperature);
    void infrared_low_warm(bool checked,int temperature);

    QString cameraName;
    QProcess camera;
    CameraView* cameraView;
    CalibrationDialog calibrationDialog;
};



#endif // TOPLEVELITEMWIDGET_H
