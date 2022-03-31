#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "cameraview.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QProcess>
#include <QWidget>
#include <QTreeWidgetItem>
#include <QtCharts/QtCharts>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void MainWindow_FrameLess(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

    int padding;                    //边距
    bool moveEnable;                //可移动
    bool resizeEnable;              //可拉伸
    QWidget *widget;                //无边框窗体

    bool pressed;                   //鼠标按下
    bool pressedLeft;               //鼠标按下左侧
    bool pressedRight;              //鼠标按下右侧
    bool pressedTop;                //鼠标按下上侧
    bool pressedBottom;             //鼠标按下下侧
    bool pressedLeftTop;            //鼠标按下左上侧
    bool pressedRightTop;           //鼠标按下右上侧
    bool pressedLeftBottom;         //鼠标按下左下侧
    bool pressedRightBottom;        //鼠标按下右下侧

    int rectX, rectY, rectW, rectH; //窗体坐标+宽高
    QPoint lastPos;                 //鼠标按下处坐标

    QRect rectLeft;                 //左侧区域
    QRect rectRight;                //右侧区域
    QRect rectTop;                  //上侧区域
    QRect rectBottom;               //下侧区域
    QRect rectLeftTop;              //左上侧区域
    QRect rectRightTop;             //右上侧区域
    QRect rectLeftBottom;           //左下侧区域
    QRect rectRightBottom;          //右下侧区域
public Q_SLOTS:
//    //设置边距
//    void setPadding(int padding);
//    //设置是否可拖动+拉伸
//    void setMoveEnable(bool moveEnable);
//    void setResizeEnable(bool resizeEnable);

    //设置目标无边框的窗体
    void setWidget(QWidget *widget);

private Q_SLOTS:
    void at_cameraStatusUpdate_timeout();
    void on_treeWidget_devices_customContextMenuRequested(const QPoint &pos);
    void on_action_open_triggered();
    void on_pushButton_zoomIn_clicked();
    void on_pushButton_zoomOut_clicked();
    void on_pushButton_zoomFull_clicked();
    void on_pushButton_snapshot_clicked();
    void on_pushButton_record_clicked();
//    void on_pushButton_customStatus_clicked();
//    void on_tabWidget_currentChanged(int index);
    void on_pushButton_playOrStop_clicked();
    void on_pushButton_whiteBalance_clicked();
    void on_pushButton_softwareTrigger_clicked();
//    void on_treeWidget_devices_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_treeWidget_devices_itemSelectionChanged();
    void on_pushButton_onceWhiteBalance_clicked();

    void on_checkBox_flicker_stateChanged(int arg1);
    void on_comboBox_frequency_currentIndexChanged(int index);
    void on_checkBox_horizontalMirrorSoft_stateChanged(int arg1);
    void on_checkBox_verticalMirrorSoft_stateChanged(int arg1);
    void on_checkBox_horizontalMirrorHard_stateChanged(int arg1);
    void on_checkBox_verticalMirrorHard_stateChanged(int arg1);
    void on_spinBox_acutance_editingFinished();
    void on_slider_acutance_sliderMoved(int value);

    void on_comboBox_resolution_currentIndexChanged(int index);
    void on_pushButton_loadParamsFromFile_clicked();
    void on_pushButton_saveParamsToFile_clicked();
    void on_actionTop_triggered();
    void on_tabWidget_params_currentChanged(int index);
    void on_checkBox_monochrome_clicked(bool checked);
    void on_checkBox_inverse_clicked(bool checked);
    void on_comboBox_algorithm_activated(int index);
    void on_comboBox_colorTemrature_activated(int index);
    void on_comboBox_lutMode_currentIndexChanged(int index);
    void on_comboBox_lutPreset_currentIndexChanged(int index);
    void on_comboBox_lutColorChannel_currentIndexChanged(int index);
    void on_checkBox_noise_clicked(bool checked);
    void on_comboBox_noise3D_activated(int index);
    void on_comboBox_rotate_activated(int index);
    void on_comboBox_frameRateSpeed_activated(int index);
    void on_spinBox_frameRateLimit_editingFinished();
    void on_spinBox_resolutionX_valueChanged(int arg1);
    void on_spinBox_resolutionY_valueChanged(int arg1);
    void on_spinBox_resolutionW_valueChanged(int arg1);
    void on_spinBox_resolutionH_valueChanged(int arg1);
    void on_pushButton_resetResolutionRect_clicked();
    void on_pushButton_resolution_clicked();

    void on_comboBox_ioMode0_activated(int index);
    void on_comboBox_ioState0_activated(int index);
    void on_comboBox_ioMode1_activated(int index);
    void on_comboBox_ioState1_activated(int index);
    void on_comboBox_ioMode2_activated(int index);
    void on_comboBox_ioState2_activated(int index);
    void on_comboBox_outputIoMode0_activated(int index);
    void on_comboBox_outputIoState0_activated(int index);
    void on_comboBox_outputIoMode1_activated(int index);
    void on_comboBox_outputIoState1_activated(int index);
    void on_comboBox_outputIoMode2_activated(int index);
    void on_comboBox_outputIoState2_activated(int index);
    void on_comboBox_outputIoMode3_activated(int index);
    void on_comboBox_outputIoState3_activated(int index);
    void on_comboBox_outputIoMode4_activated(int index);
    void on_comboBox_outputIoState4_activated(int index);

    void on_comboBox_triggerMode_currentIndexChanged(int index);
    void on_comboBox_triggerMode_activated(int index);
    void on_pushButton_softTrigger_clicked();
    void on_spinBox_frameCount_editingFinished();
    void on_spinBox_delay_editingFinished();
    void on_spinBox_interval_editingFinished();
    void on_comboBox_outsideTriggerMode_activated(int index);
    void on_spinBox_debounce_editingFinished();
    void on_comboBox_flashMode_currentIndexChanged(int index);
    void on_comboBox_flashMode_activated(int index);
    void on_comboBox_flashPolarity_activated(int index);
    void on_spinBox_strobeDelay_editingFinished();
    void on_spinBox_strobePulse_editingFinished();

    void on_checkBox_line1_stateChanged(int arg1);
    void on_checkBox_line2_stateChanged(int arg1);
    void on_checkBox_line3_stateChanged(int arg1);
    void on_checkBox_line4_stateChanged(int arg1);
    void on_checkBox_line5_stateChanged(int arg1);
    void on_checkBox_line6_stateChanged(int arg1);
    void on_checkBox_line7_stateChanged(int arg1);
    void on_checkBox_line8_stateChanged(int arg1);
    void on_checkBox_line9_stateChanged(int arg1);

    void on_pushButton_modifyNickname_clicked();
    void on_checkBox_whiteBalanceWindow_stateChanged(int arg1);
    void on_pushButton_setWhiteBalanceWindow_clicked();
    void on_pushButton_defaultWhiteBalanceWindow_clicked();
    void on_checkBox_deadPixelsWindow_stateChanged(int arg1);
    void on_pushButton_saveDeadPixels_clicked();
    void on_checkBox_flatFiledCorrect_stateChanged(int arg1);
    void on_pushButton_darkField_clicked();
    void on_pushButton_lightField_clicked();
    void on_pushButton_loadFlatFieldParams_clicked();
    void on_pushButton_saveFlatFieldParams_clicked();
    void on_pushButton_calibration_clicked();

    void on_slider_brightness_sliderMoved(int position);
    void on_spinBox_brightness_editingFinished();
    void on_slider_gain_sliderMoved(int position);
    void on_spinBox_gain_valueChanged(double);
    void on_slider_exposureTime_sliderMoved(int position);
    void on_spinBox_exposureTime_valueChanged(double);
    void on_spinBox_gainMinimum_editingFinished();
    void on_spinBox_gainMaximum_editingFinished();
    void on_spinBox_exposureTimeMaximum_editingFinished();
    void on_spinBox_exposureTimeMinimum_editingFinished();

    void on_slider_r_sliderMoved(int position);
    void on_spinBox_r_editingFinished();
    void on_slider_g_sliderMoved(int position);
    void on_spinBox_g_editingFinished();
    void on_slider_b_sliderMoved(int position);
    void on_spinBox_b_editingFinished();
    void on_spinBox_saturation_editingFinished();
    void on_slider_saturation_sliderMoved(int position);

    void on_comboBox_lutMode_activated(int index);
    void on_spinBox_gamma_editingFinished();
    void on_spinBox_contrastRatio_editingFinished();
    void on_slider_gamma_sliderMoved(int position);
    void on_slider_contrastRatio_sliderMoved(int position);
    void on_checkBox_anamorphose_clicked(bool checked);
    void on_pushButton_layout_clicked();   
    void on_spinBox_threshold_editingFinished();
    void on_pushButton_analyzeDeadPixels_clicked();
    void on_pushButton_analyzeBrightPixels_clicked();

    void on_pushButton_clearOldPixels_clicked();
    void on_pushButton_clearNewPixels_clicked();
    void on_slider_outputRange_sliderMoved(int position);
    void on_comboBox_outputFormats_activated(int index);
    void on_pushButton_low8Bit_clicked();
    void on_pushButton_high8Bit_clicked();
    void on_comboBox_outputFormats_currentIndexChanged(int index);
    void on_treeWidget_devices_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_checkBox_exposureWIndow_stateChanged(int arg1);
    void on_pushButton_setExposureWindow_clicked();

    void on_radioButton_manualExposure_clicked();
    void on_radioButton_automationExposure_clicked();
    void on_radioButton_presetResolution_clicked();
    void on_radioButton_customResolution_clicked();

    void on_checkBox_showBright_stateChanged(int arg1);
    void on_radioButton_wbManual_clicked();
    void on_radioButton_wbAutomation_clicked();
    void on_radioButton_automationExposure_toggled(bool checked);
    void on_radioButton_manualExposure_toggled(bool checked);
    void on_radioButton_wbAutomation_toggled(bool checked);
    void on_tabWidget_preview_currentChanged(int index);
    void on_pushButton_fpnEdit_clicked();
    void on_pushButton_fpnClear_clicked();
    void on_checkBox_fpn_clicked(bool checked);
    void on_pushButton_fpnSave_clicked();

    void gui_init(std::shared_ptr<GF120> gf120);
    void on_comboBox_infrared_thermometry_activated(int index);
    void on_comboBox_infrared_color_activated(int index);
    void on_comboBox_display_mode_currentIndexChanged(int index);
    void on_checkBox_infrared_shutter_clicked(bool checked);
    void on_checkBox_infrared_cool_clicked(bool checked);
    void on_spinBox_infrared_emissivity_valueChanged(int arg1);
    void on_spinBox_infrared_sharpen_valueChanged(int arg1);
    void on_spinBox_infrared_exposure_valueChanged(int arg1);
    void on_spinBox_dde_valueChanged(int arg1);
    void on_checkBox_infrared_manual_clicked(bool checked);
    void on_pushButton_infrared_temperature_check_clicked();
    void on_checkBox_infrared_stop_temperature_check_clicked(bool checked);
    void on_checkBox_infrared_shutter_temperature_raise_sample_clicked(bool checked);
    void on_checkBox_infrared_factory_check_detect_clicked(bool checked);
    void on_checkBox_infrared_response_rate_sample_clicked(bool checked);
    void on_checkBox_infrared_temperature_curve_sample_clicked(bool checked);
    void on_pushButton_factory_check_clicked();
    void on_pushButton_sample_path_clicked();
    void on_pushButton_sample_low_response_rate_clicked();
    void on_pushButton_sample_high_response_rate_clicked();
    void on_checkBox_infrared_osd_clicked(bool checked);
    void on_checkBox_infrared_temperature_display_clicked(bool checked);
    void on_checkBox_infrared_roi_clicked(bool checked);
    void on_comboBox_infrared_temperture_roi_activated(int index);
    void on_checkBox_infrared_blackbody_calibrate_clicked(bool checked);
    void on_checkBox_infrared_color_map_clicked(bool checked);
    void on_checkBox_infrared_high_warm_clicked(bool checked);
    void on_checkBox_infrared_low_warm_clicked(bool checked);
    void on_doubleSpinBox_infrared_temperature_compensation_valueChanged(double arg1);
    void on_doubleSpinBox_infrared_distance_compensation_valueChanged(int arg1);
    void on_doubleSpinBox_infrared_humidity_compensation_valueChanged(double arg1);
    void on_pushButton_load_response_rate_file_clicked();
    void on_pushButton_cover_low_clicked();
    void on_pushButton_cover_high_clicked();
    void on_pushButton_cover_load_clicked();
    void on_pushButton_save_clicked();
    void on_pushButton_load_clicked();
    void on_pushButton_change_clicked();
    void on_pushButton_saveToDevice_clicked();
    void on_pushButton_deleteFilesInDevice_clicked();
    void on_pushButton_up_clicked();
    void on_pushButton_update_clicked();
    bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist);
    int Bin_Search(int temp);

private:
    Ui::MainWindow *ui;
    QTreeWidgetItem* selectedCameraItem;
    QTimer viewStatusUpdate;
    QTimer cameraStatusUpdate;
    QTimer cameraStatus2Update;
    QTimer exposureStatusUpdate;
};
#endif // MAINWINDOW_H
