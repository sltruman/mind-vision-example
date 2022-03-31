#ifndef TOPLEVELITEMWIDGET_H
#define TOPLEVELITEMWIDGET_H

#include "cameraview.h"
#include "calibrationdialog.h"

#include "wrap/device_manager.hpp"


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
    DeviceManager dm;

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
    DeviceItem(TopLevelItemWidget *w,QTreeWidgetItem *parent, QString name,shared_ptr<Device> camera);
    virtual ~DeviceItem();

public:
    bool open();
    void close();
    std::tuple<QStringList,QStringList> exposure(int full);
    std::tuple<QStringList,QStringList> whiteBalance();
    QStringList lookupTablesForDynamic();
    QStringList lookupTablesForPreset();
    QStringList lookupTablesForCustom(int index);
    QStringList isp();
    QStringList dead_pixels_analyze_for_bright(int threshold);
    QStringList dead_pixels_analyze_for_dead(int threshold);
    std::tuple<QStringList,QStringList,QStringList> video();
    QString resolutionMode();
    QString resolutionIndex();
    QStringList resolution();
    QStringList resolutions();
    QStringList io();
    std::tuple<QStringList,QStringList,QStringList> controls();
    QStringList firmware();
    QStringList status();
//    QString brightness();

    shared_ptr<Device> camera;
    GF120* camera_gf120;

    CameraView* cameraView;
    CalibrationDialog calibrationDialog;
};



#endif // TOPLEVELITEMWIDGET_H
