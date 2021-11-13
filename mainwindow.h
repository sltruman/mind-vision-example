#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cameraview.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QProcess>
#include <QWidget>
#include <QTreeWidgetItem>

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
    void on_action_open_triggered();

    void on_pushButton_magnify_clicked();
    void on_pushButton_shrink_clicked();
    void on_pushButton_perfect_clicked();
    void on_pushButton_take_clicked();
//    void on_pushButton_record_clicked();
//    void on_pushButton_customStatus_clicked();
//    void on_tabWidget_currentChanged(int index);
    void on_pushButton_playOrStop_clicked();
//    void on_treeWidget_devices_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);


    void on_treeWidget_devices_itemSelectionChanged();
    void on_pushButton_onceWhiteBalance_clicked();


    void on_comboBox_exposureMode_currentIndexChanged(int index);

    void on_comboBox_whiteBalanceMode_currentIndexChanged(int index);

    void on_slider_brightness_valueChanged(int value);

    void on_checkBox_flicker_stateChanged(int arg1);

    void on_slider_gain_valueChanged(int value);

    void on_slider_exposureTime_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    QTreeWidgetItem* selectedCameraItem;
    QTimer cameraStatusUpdate;
};
#endif // MAINWINDOW_H
