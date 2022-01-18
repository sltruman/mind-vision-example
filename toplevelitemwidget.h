#ifndef TOPLEVELITEMWIDGET_H
#define TOPLEVELITEMWIDGET_H

#include "cameraview.h"
#include "calibrationdialog.h"

#include <QWidget>
#include <QTreeWidgetItem>
#include <QProcess>
#include <QTimer>

namespace Ui {
class TopLevelItemWidget;
}

class TopLevelItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TopLevelItemWidget(QTreeWidgetItem *topLevelItem,QString series,QWidget *parent);
    virtual ~TopLevelItemWidget();

    QTreeWidgetItem* topLevelItem;
    QString series;
    QTimer t;

private slots:
    void on_toolButton_refresh_clicked();
    void statusUpdate();

private:
    Ui::TopLevelItemWidget *ui;
};

class DeviceItem : public QTreeWidgetItem
{
public:
    DeviceItem(QTreeWidgetItem *parent, QString name);
    virtual ~DeviceItem();

    bool open();
    void close();

    QStringList exposure();
    void exposureMode(int value);
    void brightness(int value);
    void threshold(int value);
    void flicker(int value);
    void gain(float value);
    void gainRange(float minimum,float maximum);
    void exposureTime(float value);
    void exposureTimeRange(float minimum,float maximum);
    void frequency(int value);

    QStringList whiteBalance();
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

    std::tuple<QStringList,QStringList> video();
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

    QStringList controls();
    void triggerMode(int);
    void onceSoftTrigger();
    void triggerFrames(int);
    void triggerDelay(int);
    void triggerInterval(int);
    void outsideTriggerMode(int);
    void outsideTriggerDebounce(int);
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

    QString cameraName;
    QProcess camera;
    CameraView* cameraView;
    CalibrationDialog calibrationDialog;
};



#endif // TOPLEVELITEMWIDGET_H
