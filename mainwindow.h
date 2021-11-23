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
    void on_pushButton_zoomIn_clicked();
    void on_pushButton_zoomOut_clicked();
    void on_pushButton_zoomFull_clicked();
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
    void on_comboBox_frequency_currentIndexChanged(int index);
    void on_slider_r_valueChanged(int value);
    void on_slider_g_valueChanged(int value);
    void on_slider_b_valueChanged(int value);
    void on_slider_saturation_valueChanged(int value);
    void on_slider_gamma_valueChanged(int value);
    void on_slider_contrastRatio_valueChanged(int value);
    void on_checkBox_horizontalMirror_stateChanged(int arg1);
    void on_checkBox_verticalMirror_stateChanged(int arg1);
    void on_slider_acutance_valueChanged(int value);
    void on_pushButton_softTrigger_clicked();
    void on_comboBox_triggerMode_currentIndexChanged(int index);
    void on_comboBox_flashMode_currentIndexChanged(int index);
    void on_comboBox_flashPolarity_currentIndexChanged(int index);
    void on_comboBox_resolution_currentIndexChanged(int index);
    void on_MainWindow_cameraParamsUpdate();


    void on_pushButton_resetParams_clicked();

    void on_pushButton_loadParamsFromFile_clicked();

    void on_pushButton_saveParams_clicked();

    void on_comboBox_params_activated(int index);

private:
    Ui::MainWindow *ui;
    QTreeWidgetItem* selectedCameraItem;
    QTimer cameraStatusUpdate;
};
#endif // MAINWINDOW_H
