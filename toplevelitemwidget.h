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

    QString cameraName;
    QProcess camera;
    CameraView* cameraView;
};



#endif // TOPLEVELITEMWIDGET_H
