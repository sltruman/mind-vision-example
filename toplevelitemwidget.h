#ifndef TOPLEVELITEMWIDGET_H
#define TOPLEVELITEMWIDGET_H

#include "cameraview.h"

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
    ~TopLevelItemWidget();

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
    ~DeviceItem();

    bool open();
    void close();

    QStringList exposure();
    void exposureMode(int value);
    void brightness(int value);
    void flicker(int value);
    void gain(int value);
    void exposureTime(int value);
    void frequency(int value);

    QStringList whiteBalance();
    void whiteBalanceMode(int index);
    void onceWhiteBalance();
    void r(int value);
    void g(int value);
    void b(int value);
    void saturation(int value);

    QStringList lookupTables();
    void gamma(int value);
    void contrastRatio(int value);

    QStringList resolutions();
    void resolution();
    void resolution(int value);

    QStringList isp();
    void horizontalMirror(int value);
    void verticalMirror(int value);
    void acutance(int value);

    QStringList controls();
    void triggerMode(int);
    void onceSoftTrigger();
    void flashMode(int);
    void flashPolarity(int);

    void paramsReset();
    void paramsSave(int value);
    void paramsLoad(int value);
    void paramsSaveToFile(QString filename);
    void paramsLoadFromFile(QString filename);

    QString cameraName;
    QProcess camera;
    CameraView* cameraView;
};



#endif // TOPLEVELITEMWIDGET_H
