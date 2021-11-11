#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cameraview.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QProcess>
#include <QWidget>
#include <QTreeWidgetItem>

namespace Ui {
class DeviceTreeWidgetItem;
}

class DeviceTreeWidgetItem : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceTreeWidgetItem(QWidget *parent,QTreeWidgetItem *topLevelItem,QString series);
    ~DeviceTreeWidgetItem();

private slots:
    void on_toolButton_refresh_clicked();

private:
    Ui::DeviceTreeWidgetItem *ui;
    QTreeWidgetItem* topLevelItem;
    QString series;
};


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent *event);

    bool    mMousePressed;
    QPoint  mRelativeSrcPos;

private slots:
    void at_cameraStatusUpdate_timeout();

    void on_treeWidget_devices_customContextMenuRequested(const QPoint &pos);
    void on_pushButton_magnify_clicked();
    void on_pushButton_shrink_clicked();
    void on_pushButton_take_clicked();
    void on_pushButton_record_clicked();
    void on_pushButton_customStatus_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_pushButton_playOrStop_clicked(bool checked);
    void on_pushButton_perfect_clicked();
    void on_treeWidget_devices_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_action_open_triggered();

private:
    Ui::MainWindow *ui;

    QMap<QString,CameraView*> cameraViews;
    QTimer cameraStatusUpdate;
    QTreeWidgetItem* selectedCameraItem;


    CameraView* mainView;
};
#endif // MAINWINDOW_H
