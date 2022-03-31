#include "mainmenu.h"
#include "mainwindow.h"
#include "mainwindow_frameless.hpp"
#include "ui_mainwindow.h"
#include "toplevelitemwidget.h"
#include "rightsidetitlebar.h"
#include "loadingdialog.h"
#include "offlinefpndialog.h"

#include <QProcess>
#include <QSpacerItem>
#include <QToolButton>
#include <QStringList>
#include <QRegExp>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDateTime>
#include <QColorDialog>

#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow_FrameLess(parent);
    this->setMenuWidget(new MainMenu(this));

    auto rightSide = new RightSideTitleBar(this,ui->treeWidget_devices);
    ui->dockWidget_rightSide->setTitleBarWidget(rightSide);
    ui->dockWidget_rightSide->hide();
    ui->widget_control->hide();
    ui->widget_status->hide();
    ui->tabWidget_preview->tabBar()->hide();

    auto at_tabWidget_params_currentChanged = [=]() {
        emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
    };

    auto gige = ui->treeWidget_devices->topLevelItem(0);
    auto usb = ui->treeWidget_devices->topLevelItem(1);

    ui->treeWidget_devices->setItemWidget(gige,0,new TopLevelItemWidget(gige,"GIGE,GiGeCamera",ui->treeWidget_devices));
    ui->treeWidget_devices->setItemWidget(usb,0,new TopLevelItemWidget(usb,"U3V,Usb3Camera0,Usb2Camera1",ui->treeWidget_devices));
    ui->treeWidget_devices->expandAll();

    connect(rightSide,&RightSideTitleBar::save_clicked,at_tabWidget_params_currentChanged);
    connect(rightSide,&RightSideTitleBar::default_clicked,at_tabWidget_params_currentChanged);
    connect(rightSide,&RightSideTitleBar::params_activated,at_tabWidget_params_currentChanged);

    connect(&cameraStatusUpdate,SIGNAL(timeout()),SLOT(at_cameraStatusUpdate_timeout()));
    cameraStatusUpdate.setInterval(1000);
    cameraStatusUpdate.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    ui->tabWidget_preview->setCurrentIndex(0);

    for(auto topItemIndex=0;topItemIndex<ui->treeWidget_devices->topLevelItemCount();topItemIndex++) {
        auto topLevelItem = ui->treeWidget_devices->topLevelItem(topItemIndex);

        for(auto subItemIndex=0;subItemIndex< topLevelItem->childCount();subItemIndex++) {
            auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(subItemIndex));

            if(!deviceItem->camera->opened()) continue;
            deviceItem->close();
        }
    }
}

void MainWindow::at_cameraStatusUpdate_timeout()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    if(deviceItem == nullptr || !deviceItem->camera->opened()) return;

    if(deviceItem->cameraView->avgBrightness) {
        auto brightness = QString::number(deviceItem->cameraView->brightness);
        ui->label_brightness->setText(brightness);
        ui->widget_bright->show();
    } else {
        ui->widget_bright->hide();
    }

    auto resolution = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->background->pixmap().size();
    ui->label_resolution->setText(QString("%1x%2").arg(resolution.width()).arg(resolution.height()));
    ui->label_zoom->setText(QString::number(deviceItem->cameraView->currentScale * 100) + '%');
    ui->label_displayFPS->setText(QString::number(deviceItem->cameraView->displayFPS));
    ui->label_coordinate->setText(deviceItem->cameraView->coordinate);

    auto rgb = deviceItem->cameraView->rgb;
    if(deviceItem->cameraView->img.format() != QImage::Format::Format_RGB888) {
        ui->label_rgb_column->setText(tr("GrayLevel"));
        ui->label_rgb->setText(QString("%4").arg(deviceItem->cameraView->brightness));
    } else {
        ui->label_rgb_column->setText("RGB");
        ui->label_rgb->setText(QString("%1,%2,%3(%4)").arg(rgb.red()).arg(rgb.green()).arg(rgb.blue()).arg(deviceItem->cameraView->brightness));
    }

    ui->pushButton_playOrStop->setChecked(deviceItem->cameraView->playing);

    auto s = deviceItem->camera->snapshoting;
    ui->pushButton_snapshot->setText(s ? tr("Stop") : tr("Snapshot"));
    ui->pushButton_snapshot->setCheckable(s);

    s = deviceItem->camera->recording;
    ui->pushButton_record->setText(s ? tr("Stop") : tr("Record"));
    ui->pushButton_record->setCheckable(s);

    auto status = QString::fromStdString(deviceItem->camera->status_string.str()).split(',');

    ui->label_frames->setText(status[0]);
    ui->label_recordFPS->setText(status[1]);

    if(std::dynamic_pointer_cast<GIGEDevice>(deviceItem->camera)) {
        ui->label_temperature->setText(status[3]);
        ui->label_lost->setText(status[4]);
        ui->label_resend->setText(status[5]);
        ui->label_packSize->setText(status[6]);
    } else if(std::dynamic_pointer_cast<USB3Device>(deviceItem->camera)) {
        ui->label_sensorFps->setText(status[2]);
        ui->label_temperature->setText(status[3]);
        ui->label_lost->setText(status[4]);
        ui->label_resend->setText(status[5]);
        ui->label_recover->setText(status[6]);
        ui->label_linkSpeed->setText(status[7] == "3" ? tr("SuperSpeed") : status[7] == "2" ? tr("HighSpeed") : tr("FullSpeed"));
    } else {
        ui->label_linkSpeed->setText(status[7] == "3" ? tr("SuperSpeed") : status[7] == "2" ? tr("HighSpeed") : tr("FullSpeed"));
    }

    if(ui->radioButton_automationExposure->isChecked() && ui->tabWidget_params->currentIndex() == 0) {
        try {
            auto exposure = get<0>(deviceItem->exposure(false));
            ui->spinBox_gain->setValue(exposure[9].toInt() / 100.);
            ui->slider_gain->setValue(exposure[9].toInt());
            ui->spinBox_exposureTime->setValue(exposure[12].toDouble() / 1000.);
            ui->slider_exposureTime->setValue(exposure[12].toDouble());
        } catch(...) {}
    }

    auto camera_gf = std::dynamic_pointer_cast<GF120>(deviceItem->camera);

    if(camera_gf) {
        auto infrared_status = camera_gf->status();
        ui->lineEdit_infrared_ad_value->setText(QString::number(infrared_status.ad_value));
        ui->lineEdit_infrared_ad_diff->setText(QString::number(infrared_status.diff_ad_value));
        ui->lineEdit_infrared_gst417m_temp->setText(QString::number(infrared_status.outside_temp / 100.) + QString::fromLocal8Bit("°C"));
        ui->lineEdit_infrared_temp_diff->setText(QString::number(infrared_status.diff_temp_value / 100.) + QString::fromLocal8Bit("°C"));
        ui->spinBox_infrared_exposure->setValue(infrared_status.gst417m_ctia);

        ui->tabWidget_params->setTabVisible(10,true);
        for(int i=0;i<10;i++) ui->tabWidget_params->setTabVisible(i,false);
    } else {
        ui->tabWidget_params->setTabVisible(10,false);
        for(int i=0;i<10;i++) ui->tabWidget_params->setTabVisible(i,true);
    }
}

void MainWindow::on_treeWidget_devices_customContextMenuRequested(const QPoint &pos)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;
    cout << "menu " << deviceItem->text(0).toStdString() << std::endl;

    auto qMenu = new QMenu(ui->treeWidget_devices);
    qMenu->setObjectName("menu_devices");
    qMenu->addAction(ui->action_open);
    qMenu->addAction(ui->actionTop);
//    qMenu->addAction(ui->action_modifyIp);
//    qMenu->addAction(ui->action_rename);
    qMenu->exec(QCursor::pos()); //在鼠标点击的位置显示鼠标右键菜单
}

void MainWindow::on_action_open_triggered()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    if(!deviceItem->camera->opened()) {
        connect(deviceItem->cameraView,&CameraView::focused,this,[=](){
            ui->treeWidget_devices->setCurrentItem(deviceItem);
        },Qt::UniqueConnection);

        connect(deviceItem->cameraView,&CameraView::doubleClick,this,[=](){
            ui->treeWidget_devices->setCurrentItem(deviceItem);
            ui->tabWidget_preview->setCurrentIndex(1);
        },Qt::UniqueConnection);

        cout << "open " << std::endl;
        if(!deviceItem->open()) {
            QMessageBox::critical(nullptr, tr("Device"), tr("Failed Connecting the camera!"), QMessageBox::Ok);
            return;
        }
    } else {
        cout << "close " << std::endl;
        deviceItem->close();
        cout << "closed " << std::endl;
        deviceItem->cameraView->setParent(nullptr);
    }

    emit ui->treeWidget_devices->itemSelectionChanged();
}

void MainWindow::on_pushButton_zoomIn_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->zoom(1.2);
}

void MainWindow::on_pushButton_zoomOut_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->zoom(0.8);
}

void MainWindow::on_pushButton_zoomFull_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->setWindowFlags(Qt::Window);
    deviceItem->cameraView->showFullScreen();
}

void MainWindow::on_pushButton_snapshot_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(ui->pushButton_snapshot->isChecked()) {
        cout << "snapshot-state" << std::endl;
        deviceItem->camera->snapshot_stop();
        return;
    }

    cout << "snapshot-start" << std::endl;
    auto mainMenu = dynamic_cast<MainMenu*>(this->menuWidget());
    deviceItem->camera->snapshot_start(mainMenu->snapshotDialog.dir().toStdString(),mainMenu->snapshotDialog.format(),mainMenu->snapshotDialog.period());
    emit at_cameraStatusUpdate_timeout();
}

void MainWindow::on_pushButton_record_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(ui->pushButton_record->isChecked()) {
        cout << "record-state" << std::endl;
        deviceItem->camera->record_stop();
        return;
    }

    auto mainMenu = dynamic_cast<MainMenu*>(this->menuWidget());
    deviceItem->camera->record_start(mainMenu->recordDialog.dir().toStdString(),mainMenu->recordDialog.format(),mainMenu->recordDialog.quality());
    emit at_cameraStatusUpdate_timeout();
}

void MainWindow::on_pushButton_playOrStop_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    if(deviceItem->cameraView->playing) {
        deviceItem->cameraView->pause();
    } else {
        deviceItem->cameraView->play();
    }
}

void MainWindow::on_pushButton_whiteBalance_clicked()
{
    emit ui->pushButton_onceWhiteBalance->clicked();
}

void MainWindow::on_pushButton_softwareTrigger_clicked()
{
    emit ui->pushButton_softTrigger->clicked();
}

void MainWindow::on_treeWidget_devices_itemSelectionChanged()
{
    int availableDevices = 0;

    for(auto topItemIndex=0;topItemIndex<ui->treeWidget_devices->topLevelItemCount();topItemIndex++) {
        auto topLevelItem = ui->treeWidget_devices->topLevelItem(topItemIndex);

        for(auto subItemIndex=0;subItemIndex< topLevelItem->childCount();subItemIndex++) {
            auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(subItemIndex));
            if(deviceItem->camera->opened()) availableDevices++;
        }
    }

    if(availableDevices) {
//        ui->tabWidget_preview->tabBar()->setTabVisible(0,false);
        if(ui->tabWidget_preview->currentIndex() == 0)
            ui->tabWidget_preview->setCurrentIndex(1);
    } else {
//        ui->tabWidget_preview->tabBar()->setTabVisible(0,true);
        if(ui->tabWidget_preview->currentIndex() != 0)
            ui->tabWidget_preview->setCurrentIndex(0);
    }

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) {
        ui->tabWidget_preview->setCurrentIndex(0);
        ui->dockWidget_rightSide->hide();
        ui->widget_control->hide();
        ui->widget_status->hide();
    } else if(!deviceItem->camera->opened()) {
        ui->dockWidget_rightSide->hide();
        ui->widget_control->hide();
        ui->widget_status->hide();
    } else {
        ui->dockWidget_rightSide->show();
        ui->widget_control->show();
        ui->widget_status->show();

        emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
    }

    if(!deviceItem) return;

    auto info = deviceItem->camera->info;
    ui->label_series_2->setText(QString(info.acProductSeries).left(4));
    ui->label_deviceName_2->setText(info.acProductName);
    ui->label_sensor_2->setText(info.acSensorType);
    ui->label_manufacturer_2->setText("Mind Vision");

    auto gige_camera = std::dynamic_pointer_cast<GIGEDevice>(deviceItem->camera);
    if(gige_camera) {
        ui->label_ip_2->show();
        ui->label_ip->show();
        ui->label_mask_2->show();
        ui->label_mask->show();
        ui->label_gateway_2->show();
        ui->label_gateway->show();

        ui->label_ip_2->setText(gige_camera->camIp.c_str());
        ui->label_mask_2->setText(gige_camera->camMask.c_str());
        ui->label_gateway_2->setText(gige_camera->camGateWay.c_str());
    } else {
        ui->label_ip_2->hide();
        ui->label_ip->hide();
        ui->label_mask_2->hide();
        ui->label_mask->hide();
        ui->label_gateway_2->hide();
        ui->label_gateway->hide();
    }

    ui->widget_packSize->hide();
    ui->widget_sfps->hide();

    if(std::dynamic_pointer_cast<GIGEDevice>(deviceItem->camera)) {
        ui->widget_lost->show();
        ui->widget_resend->show();
        ui->widget_packSize->show();
        ui->widget_temperature->show();
        auto portType = QString(info.acPortType);
        if(-1 != portType.indexOf("10000M")) ui->label_linkSpeed->setText("10000M");
        if(-1 != portType.indexOf("1000M")) ui->label_linkSpeed->setText("1000M");
        if(-1 != portType.indexOf("100M")) ui->label_linkSpeed->setText("100M");
    } else if(std::dynamic_pointer_cast<USB3Device>(deviceItem->camera)) {
        ui->widget_lost->show();
        ui->widget_resend->show();
        ui->widget_recover->show();
        ui->widget_sfps->show();
        ui->widget_temperature->show();
    } else {
        ui->widget_lost->hide();
        ui->widget_resend->hide();
        ui->widget_recover->hide();
        ui->widget_temperature->hide();
    }

    ui->checkBox_showBright->setChecked(deviceItem->cameraView->avgBrightness);

    if(ui->tabWidget_preview->currentIndex() == 1 && -1 == ui->tab_main_contents->indexOf(deviceItem->cameraView)) {
        if(ui->tab_main_contents->count())
            ui->tab_main_contents->itemAt(0)->widget()->setParent(nullptr);
        ui->tab_main_contents->addWidget(deviceItem->cameraView);
    }
}

void MainWindow::on_checkBox_flicker_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

     deviceItem->camera->flicker(arg1);
}

void MainWindow::on_comboBox_frequency_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->frequency(index);
}

void MainWindow::on_checkBox_horizontalMirrorSoft_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->horizontal_mirror(0,arg1);
}

void MainWindow::on_checkBox_verticalMirrorSoft_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->vertical_mirror(0,arg1);
}

void MainWindow::on_checkBox_horizontalMirrorHard_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->horizontal_mirror(1,arg1);
}

void MainWindow::on_checkBox_verticalMirrorHard_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->vertical_mirror(2,arg1);
}

void MainWindow::on_pushButton_onceWhiteBalance_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->once_white_balance();

    emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
}

void MainWindow::on_comboBox_resolution_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->resolution(index);
}

void MainWindow::on_spinBox_acutance_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_acutance->value();
    deviceItem->camera->acutance(v);
    ui->slider_acutance->setValue(v);
}

void MainWindow::on_slider_acutance_sliderMoved(int position)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    ui->spinBox_acutance->setValue(position);
    emit ui->spinBox_acutance->editingFinished();
}

void MainWindow::on_pushButton_loadParamsFromFile_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto filename = QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("config Files(*.config )"));
    if(filename.isEmpty()) return;
    deviceItem->camera->params_load_from_file(filename.toStdString());
    emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
}

void MainWindow::on_pushButton_saveParamsToFile_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto filename = QFileDialog::getSaveFileName(this,tr("File name"),"",tr("config Files(*.config )"));
    if(filename.isEmpty()) return;
    deviceItem->camera->params_save_to_file(filename.toStdString());
}

void MainWindow::on_actionTop_triggered()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    auto topTreeItem = deviceItem->parent();
    topTreeItem->removeChild(deviceItem);
    topTreeItem->insertChild(0,deviceItem);
}

void MainWindow::on_tabWidget_params_currentChanged(int index)
{
    ui->checkBox_whiteBalanceWindow->setChecked(false);
    ui->checkBox_deadPixelsWindow->setChecked(false);
    ui->checkBox_exposureWIndow->setChecked(false);

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->resolutionWindow = false;

    try {
        if(index == 0) {
            auto exposure = deviceItem->exposure(true);
            auto values = get<0>(exposure);

            if(values[0].toUInt()) ui->radioButton_automationExposure->setChecked(true);
            else ui->radioButton_manualExposure->setChecked(true);

            ui->slider_brightness->setMinimum(values[1].toUInt());
            ui->slider_brightness->setMaximum(values[2].toUInt());
            ui->slider_brightness->setValue(values[3].toUInt());

            ui->spinBox_brightness->setMinimum(values[1].toUInt());
            ui->spinBox_brightness->setMaximum(values[2].toUInt());
            ui->spinBox_brightness->setValue(values[3].toUInt());

            ui->checkBox_flicker->setChecked(values[4].toUInt());
            ui->comboBox_frequency->setCurrentIndex(values[5].toUInt());

            ui->spinBox_threshold->setValue(values[6].toUInt());

            ui->slider_gain->setMinimum(values[7].toInt());
            ui->slider_gain->setMaximum(values[8].toInt());
            ui->slider_gain->setValue(values[9].toInt());

            ui->spinBox_gain->setMinimum(values[7].toInt() / 100.);
            ui->spinBox_gain->setMaximum(values[8].toInt());
            ui->spinBox_gain->setValue(values[9].toInt() / 100.);

            ui->spinBox_gainMinimum->setMinimum(values[7].toInt() / 100.);
            ui->spinBox_gainMinimum->setMaximum(values[16].toInt() / 100.);
            ui->spinBox_gainMaximum->setMinimum(values[7].toInt() / 100.);
//            ui->spinBox_gainMaximum->setMaximum(values[8].toInt() / 100.);
            ui->spinBox_gainMinimum->setValue(values[7].toInt() / 100.);
            ui->spinBox_gainMaximum->setValue(values[16].toInt() / 100.);

            ui->slider_exposureTime->setMinimum(values[10].toInt());
            ui->slider_exposureTime->setMaximum(values[11].toInt());
            ui->spinBox_exposureTime->setMinimum(values[10].toInt() / 1000.);
            ui->spinBox_exposureTime->setMaximum(values[11].toInt());

            ui->spinBox_exposureTime->setValue(values[12].toInt() / 1000.);
//            emit ui->spinBox_exposureTime->editingFinished();

            ui->spinBox_exposureTimeMinimum->setMinimum(values[13].toInt() / 1000.);
            ui->spinBox_exposureTimeMinimum->setMaximum(values[18].toInt() / 1000.);
            ui->spinBox_exposureTimeMaximum->setMinimum(values[13].toInt() / 1000.);
//            ui->spinBox_exposureTimeMaximum->setMaximum(values[14].toInt() / 1000.);
            ui->spinBox_exposureTimeMinimum->setValue(values[13].toInt() / 1000.);
            ui->spinBox_exposureTimeMaximum->setValue(values[18].toInt() / 1000.);

            auto window = get<1>(exposure);
            auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
            cs->exposureWindowPos.setX(window[0].toInt());
            cs->exposureWindowPos.setY(window[1].toInt());
            cs->exposureWindowPos.setWidth(window[2].toInt());
            cs->exposureWindowPos.setHeight(window[3].toInt());
        } else if(index == 1) {
            auto whiteBalance = deviceItem->whiteBalance();
            auto values = get<0>(whiteBalance);

            if(values[0].toUInt()) ui->radioButton_wbAutomation->setChecked(true);
            else ui->radioButton_wbManual->setChecked(true);

            ui->slider_r->setMinimum(values[1].toUInt());
            ui->slider_r->setMaximum(values[2].toUInt());
            ui->slider_r->setValue(values[3].toUInt());
            ui->spinBox_r->setMinimum(values[1].toUInt() / 100.f);
            ui->spinBox_r->setMaximum(values[2].toUInt() / 100.f);
            ui->spinBox_r->setValue(values[3].toUInt() / 100.f);

            ui->slider_g->setMinimum(values[4].toUInt());
            ui->slider_g->setMaximum(values[5].toUInt());
            ui->slider_g->setValue(values[6].toUInt());
            ui->spinBox_g->setMinimum(values[4].toUInt() / 100.f);
            ui->spinBox_g->setMaximum(values[5].toUInt() / 100.f);
            ui->spinBox_g->setValue(values[6].toUInt() / 100.f);

            ui->slider_b->setMinimum(values[7].toUInt());
            ui->slider_b->setMaximum(values[8].toUInt());
            ui->slider_b->setValue(values[9].toUInt());
            ui->spinBox_b->setMinimum(values[7].toUInt() / 100.f);
            ui->spinBox_b->setMaximum(values[8].toUInt() / 100.f);
            ui->spinBox_b->setValue(values[9].toUInt() / 100.f);

            ui->slider_saturation->setMinimum(values[10].toUInt());
            ui->slider_saturation->setMaximum(values[11].toUInt());
            ui->slider_saturation->setValue(values[12].toUInt());
            ui->spinBox_saturation->setMinimum(values[10].toUInt());
            ui->spinBox_saturation->setMaximum(values[11].toUInt());
            ui->spinBox_saturation->setValue(values[12].toUInt());

            ui->checkBox_monochrome->setChecked(values[13].toUInt());
            ui->checkBox_inverse->setChecked(values[14].toUInt());
            ui->comboBox_algorithm->setCurrentIndex(values[15].toInt());

            ui->comboBox_colorTemrature->clear();
            for(auto colorTemrature : get<1>(whiteBalance))
                ui->comboBox_colorTemrature->addItem(tr(colorTemrature.replace(QString::fromLocal8Bit("、"),",").toLocal8Bit()));

            ui->comboBox_colorTemrature->setCurrentIndex(values[16].toInt());
            auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
            cs->whiteBalanceWindowPos.setX(values[17].toInt());
            cs->whiteBalanceWindowPos.setY(values[18].toInt());
            cs->whiteBalanceWindowPos.setWidth(values[19].toInt());
            cs->whiteBalanceWindowPos.setHeight(values[20].toInt());
            ui->checkBox_fpn->setChecked(values[21].toInt());
        } else if(index == 2) {
            auto lookupTableMode = deviceItem->camera->lookup_table_mode();
            ui->comboBox_lutMode->setCurrentIndex(lookupTableMode);
        } else if(index == 3) {
            auto isp = deviceItem->isp();
            ui->checkBox_horizontalMirrorSoft->setChecked(isp[0].toUInt());
            ui->checkBox_verticalMirrorSoft->setChecked(isp[1].toUInt());

            ui->slider_acutance->setMinimum(isp[2].toUInt());
            ui->slider_acutance->setMaximum(isp[3].toUInt());
            ui->slider_acutance->setValue(isp[4].toUInt());
            ui->spinBox_acutance->setMinimum(isp[2].toUInt());
            ui->spinBox_acutance->setMaximum(isp[3].toUInt());
            ui->spinBox_acutance->setValue(isp[4].toUInt());

            ui->checkBox_noise->setChecked(isp[5].toUInt());
            if(isp[6].toUInt()) ui->comboBox_noise3D->setCurrentIndex(isp[7].toUInt() - 1);
            else ui->comboBox_noise3D->setCurrentIndex(0);
            ui->comboBox_rotate->setCurrentIndex(isp[8].toUInt());
            ui->checkBox_flatFiledCorrect->setChecked(isp[9].toUInt());
            ui->checkBox_deadPixelsCorrect->setChecked(isp[10].toUInt());
            auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
            cs->existedPixels.clear();

            for(auto xy : deviceItem->camera->dead_pixels())
                cs->existedPixels.append(QPoint(xy[0],xy[1]));

            ui->checkBox_anamorphose->setChecked(isp[12].toUInt());
            if(isp[13] != "-1") ui->checkBox_horizontalMirrorSoft->setChecked(isp[13].toUInt());
            if(isp[14] != "-1") ui->checkBox_verticalMirrorSoft->setChecked(isp[14].toUInt());
        } else if(index == 4) {
            auto video_tuple = deviceItem->video();
            auto video = std::get<0>(video_tuple);
            auto output_formats = std::get<1>(video_tuple);

            ui->comboBox_frameRateSpeed->clear();
            for(auto frameSpeed : get<2>(video_tuple))
                ui->comboBox_frameRateSpeed->addItem(tr(frameSpeed.toLocal8Bit()));
            ui->comboBox_frameRateSpeed->setCurrentIndex(video[0].toUInt());

            ui->spinBox_frameRateLimit->setValue(video[1].toInt());
            ui->slider_outputRange->setMaximum(video[4].toInt()-8);
            ui->slider_outputRange->setValue(video[3].toInt());

            ui->comboBox_outputFormats->clear();
            for(auto format : output_formats) {
                ui->comboBox_outputFormats->addItem(format);
            }
            ui->comboBox_outputFormats->setCurrentIndex(video[2].toUInt());
        } else if(index == 5) {
            auto resolutionMode = deviceItem->resolutionMode();
            if(resolutionMode.toUInt()) ui->radioButton_customResolution->click();
            else ui->radioButton_presetResolution->click();

        } else if(index == 6) {
            auto i=0,j=0;
            auto io = deviceItem->io();

            ui->group_in0->hide();
            ui->group_in1->hide();
            ui->group_in2->hide();
            ui->group_out0->hide();
            ui->group_out1->hide();
            ui->group_out2->hide();
            ui->group_out3->hide();
            ui->group_out4->hide();

            for(auto line : io) {
                auto columns = line.split(',');
                auto type = columns[0];
                auto mode = columns[1].toUInt();
                auto state = columns[2].toUInt();

                if(type == "Input") {
                    auto group_in = ui->tabWidget_params->findChild<QWidget*>(QString("group_in%1").arg(i));
                    group_in->show();

                    auto comboBox_ioMode = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_ioMode%1").arg(i));

                    if(i)
                        comboBox_ioMode->setCurrentIndex(mode == 2 ? 0 : -1);
                    else
                        comboBox_ioMode->setCurrentIndex(mode == 0 ? 0 : 1);

                    auto comboBox_ioState = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_ioState%1").arg(i));
                    comboBox_ioState->setCurrentIndex(state);
                    i++;
                } else {
                    auto group_out = ui->tabWidget_params->findChild<QWidget*>(QString("group_out%1").arg(j));
                    group_out->show();


                    auto comboBox_outputIoMode = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_outputIoMode%1").arg(j));
                    if(j)
                        comboBox_outputIoMode->setCurrentIndex(mode == 3 ? 0 : -1);
                    else
                        comboBox_outputIoMode->setCurrentIndex(mode == 1 ? 0 : 1);

                    auto comboBox_outputIoState = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_outputIoState%1").arg(j));
                    comboBox_outputIoState->setCurrentIndex(state);

                    j++;
                }
            }
        } else if(index == 7) {
            auto controls= deviceItem->controls();
            auto values = get<0>(controls);
            ui->comboBox_triggerMode->setCurrentIndex(values[0].toUInt());
            ui->spinBox_frameCount->setValue(values[1].toUInt());
            ui->spinBox_delay->setValue(values[2].toUInt());
            ui->spinBox_interval->setValue(values[3].toUInt());

            ui->comboBox_outsideTriggerMode->clear();
            for(auto triggerMode : get<1>(controls))
                ui->comboBox_outsideTriggerMode->addItem(tr(triggerMode.toLocal8Bit()));
            ui->comboBox_outsideTriggerMode->setCurrentIndex(values[4].toUInt());
            ui->spinBox_debounce->setValue(values[5].toUInt());

            ui->comboBox_flashMode->setCurrentIndex(values[6].toUInt());
            ui->comboBox_flashPolarity->setCurrentIndex(values[7].toUInt());
            ui->spinBox_strobeDelay->setValue(values[8].toUInt());
            ui->spinBox_strobePulse->setValue(values[9].toUInt());

            ui->comboBox_outsideShutter->clear();
            for(auto shutter : get<2>(controls))
                ui->comboBox_outsideShutter->addItem(tr(shutter.toLocal8Bit()));
            ui->comboBox_outsideShutter->setCurrentIndex(values[10].toUInt());

        } else if(index == 8) {
        } else if(index == 9) {
            auto firmware= deviceItem->firmware();
            ui->label_firmware->setText(firmware[0]);
            ui->label_interface->setText(firmware[1]);
            ui->label_platform->setText(firmware[2]);
            ui->label_driver->setText(firmware[3]);
            ui->label_updatable->setText(firmware[4].toUInt() ? tr("older") : tr("newest"));
            ui->lineEdit_nickname->setText(firmware[5]);
            ui->lineEdit_SN->setText(firmware[6]);
        } else if(index == 10) {
            auto firmware= deviceItem->firmware();
            auto pwd = QDir::current();
            auto configDir = QString("Camera/Data/corr_%1").arg(deviceItem->camera->info.acSn);
            pwd.mkpath(configDir);

            ui->lineEdit_sample_path->setText(configDir);
            ui->lineEdit_infrared_path_to_low_response_rate_sample->setText(configDir + "/response_temp40.bin");
            ui->lineEdit_infrared_path_to_high_response_rate_sample->setText(configDir + "/response_temp140.bin");

            auto gf120 = std::dynamic_pointer_cast<GF120>(deviceItem->camera);
            gui_init(gf120);
        } else {

        }
    } catch(...) {
        cout << "Failed to sync camera's params!" << std::endl;
        emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
    }
}

void MainWindow::on_checkBox_monochrome_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->monochrome(checked);
}

void MainWindow::on_checkBox_inverse_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->inverse(checked);
}

void MainWindow::on_comboBox_algorithm_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->algorithm(index);
}

void MainWindow::on_comboBox_colorTemrature_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->color_temrature(index);
}

void MainWindow::on_comboBox_lutPreset_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->lookup_table_preset(index);
    auto lookupTables = deviceItem->lookupTablesForPreset();

    auto line = new QSplineSeries();
    auto values = lookupTables[1].split(',');
    for(int i=0;i < values.size();i++) {
        line->append(i, values[i].toInt());
    }

    ui->chartView_lut->chart()->removeAllSeries();
    ui->chartView_lut->chart()->addSeries(line);
    ui->chartView_lut->chart()->createDefaultAxes();
}

void MainWindow::on_comboBox_lutColorChannel_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto lookupTables = deviceItem->lookupTablesForCustom(index);

    auto line = new QSplineSeries();
    auto values = lookupTables[0].split(',');
    for(int i=0;i < values.size();i++) {
        line->append(i, values[i].toInt());
    }

    ui->chartView_lut->chart()->removeAllSeries();
    ui->chartView_lut->chart()->addSeries(line);
    ui->chartView_lut->chart()->createDefaultAxes();
}

void MainWindow::on_checkBox_noise_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->noise(checked);
}

void MainWindow::on_comboBox_noise3D_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(index) deviceItem->camera->noise3d(1,index + 1);
    else deviceItem->camera->noise3d(0,0);
}

void MainWindow::on_comboBox_rotate_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->rotate(index);
}

void MainWindow::on_comboBox_frameRateSpeed_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->frame_rate_speed(index);
}

void MainWindow::on_spinBox_frameRateLimit_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->frame_rate_limit(ui->spinBox_frameRateLimit->value());
}


void MainWindow::on_spinBox_resolutionX_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindowPos.setX(arg1);
    cs->resolutionWindowPos.setY(ui->spinBox_resolutionY->value());
    cs->resolutionWindowPos.setWidth(ui->spinBox_resolutionW->value());
    cs->resolutionWindowPos.setHeight(ui->spinBox_resolutionH->value());
}

void MainWindow::on_spinBox_resolutionY_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindowPos.setX(ui->spinBox_resolutionX->value());
    cs->resolutionWindowPos.setY(arg1);
    cs->resolutionWindowPos.setWidth(ui->spinBox_resolutionW->value());
    cs->resolutionWindowPos.setHeight(ui->spinBox_resolutionH->value());
}

void MainWindow::on_spinBox_resolutionW_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindowPos.setX(ui->spinBox_resolutionX->value());
    cs->resolutionWindowPos.setY(ui->spinBox_resolutionY->value());
    cs->resolutionWindowPos.setWidth(arg1);
    cs->resolutionWindowPos.setHeight(ui->spinBox_resolutionH->value());
}

void MainWindow::on_spinBox_resolutionH_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindowPos.setX(ui->spinBox_resolutionX->value());
    cs->resolutionWindowPos.setY(ui->spinBox_resolutionY->value());
    cs->resolutionWindowPos.setWidth(ui->spinBox_resolutionW->value());
    cs->resolutionWindowPos.setHeight(arg1);
}

void MainWindow::on_pushButton_resetResolutionRect_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->resolution(0);

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindow = true;
}

void MainWindow::on_pushButton_resolution_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindow = false;

    auto rect = cs->resolutionWindowPos;
    deviceItem->camera->resolution(rect.x(),rect.y(),rect.width(),rect.height());
}

void MainWindow::on_comboBox_ioMode0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Input",0,index == 0 ? 0 : 2);
}

void MainWindow::on_comboBox_ioState0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->io_state("Input",0,index);
}

void MainWindow::on_comboBox_ioMode1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Input",1,2);
}

void MainWindow::on_comboBox_ioState1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->io_state("Input",1,index);
}

void MainWindow::on_comboBox_ioMode2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Input",2,2);
}

void MainWindow::on_comboBox_ioState2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_state("Input",2,index);
}

void MainWindow::on_comboBox_outputIoMode0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Output",0,index == 0 ? 1 : 3);
}

void MainWindow::on_comboBox_outputIoState0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_state("Output",0,index);
}

void MainWindow::on_comboBox_outputIoMode1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Output",1,3);
}

void MainWindow::on_comboBox_outputIoState1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_state("Output",1,index);
}

void MainWindow::on_comboBox_outputIoMode2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Output",2,3);
}

void MainWindow::on_comboBox_outputIoState2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_state("Output",2,index);
}

void MainWindow::on_comboBox_outputIoMode3_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Output",3,3);
}

void MainWindow::on_comboBox_outputIoState3_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_state("Output",3,index);
}

void MainWindow::on_comboBox_outputIoMode4_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_mode("Output",3,3);
}

void MainWindow::on_comboBox_outputIoState4_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->io_state("Output",3,index);
}

void MainWindow::on_comboBox_triggerMode_currentIndexChanged(int index)
{
    ui->groupBox_trigger->hide();
    ui->groupBox_outsideTrigger->hide();
    ui->groupBox_strobe->hide();

    if(index > 0) {
        ui->groupBox_trigger->show();
        ui->groupBox_strobe->show();
    }

    if(index > 1) {
        ui->groupBox_outsideTrigger->show();
    }
}

void MainWindow::on_comboBox_triggerMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->trigger_mode(index);
}

void MainWindow::on_pushButton_softTrigger_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->once_soft_trigger();
}

void MainWindow::on_spinBox_frameCount_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->trigger_frames(ui->spinBox_frameCount->value());
}

void MainWindow::on_spinBox_delay_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->trigger_delay(ui->spinBox_delay->value());
}

void MainWindow::on_spinBox_interval_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->trigger_interval(ui->spinBox_interval->value());
}

void MainWindow::on_comboBox_outsideTriggerMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->outside_trigger_mode(index);
}

void MainWindow::on_spinBox_debounce_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->outside_trigger_debounce(ui->spinBox_debounce->value());
}

void MainWindow::on_comboBox_flashMode_currentIndexChanged(int index)
{
    if(index) {
        ui->comboBox_flashPolarity->setEnabled(true);
        ui->spinBox_strobeDelay->setEnabled(true);
        ui->spinBox_strobePulse->setEnabled(true);
    } else {
        ui->comboBox_flashPolarity->setDisabled(true);
        ui->spinBox_strobeDelay->setDisabled(true);
        ui->spinBox_strobePulse->setDisabled(true);
    }
}

void MainWindow::on_comboBox_flashMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;
    deviceItem->camera->strobe_mode(index);
}

void MainWindow::on_comboBox_flashPolarity_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->strobe_polarity(index);
}

void MainWindow::on_spinBox_strobeDelay_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->strobe_delay(ui->spinBox_strobeDelay->value());
}

void MainWindow::on_spinBox_strobePulse_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->strobe_pulse(ui->spinBox_strobePulse->value());
}

void MainWindow::on_checkBox_line1_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x1->value();
        auto lineY = ui->spinBox_y1->value();
        auto pen = QPen(QColor(255,0,0));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(0,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(0);
    }
}

void MainWindow::on_checkBox_line2_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x2->value();
        auto lineY = ui->spinBox_y2->value();
        auto pen = QPen(QColor(0,255,0));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(1,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(1);
    }
}

void MainWindow::on_checkBox_line3_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x3->value();
        auto lineY = ui->spinBox_y3->value();
        auto pen = QPen(QColor(0,0,255));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(2,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(2);
    }
}

void MainWindow::on_checkBox_line4_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x4->value();
        auto lineY = ui->spinBox_y4->value();
        auto pen = QPen(QColor(255,255,0));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(3,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(3);
    }
}

void MainWindow::on_checkBox_line5_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x5->value();
        auto lineY = ui->spinBox_y5->value();
        auto pen = QPen(QColor(255,0,255));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(4,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(4);
    }
}

void MainWindow::on_checkBox_line6_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x6->value();
        auto lineY = ui->spinBox_y6->value();
        auto pen = QPen(QColor(85,255,255));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(5,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(5);
    }
}

void MainWindow::on_checkBox_line7_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x7->value();
        auto lineY = ui->spinBox_y7->value();
        auto pen = QPen(QColor(170, 85, 0));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(6,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(6);
    }
}

void MainWindow::on_checkBox_line8_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x8->value();
        auto lineY = ui->spinBox_y8->value();
        auto pen = QPen(QColor(85, 0, 127));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(7,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(7);
    }
}

void MainWindow::on_checkBox_line9_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(arg1) {
        auto lineX = ui->spinBox_x9->value();
        auto lineY = ui->spinBox_y9->value();
        auto pen = QPen(QColor(85, 85, 127));
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.insert(8,std::make_tuple(lineX,lineY,pen));
    } else {
        dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->lines.remove(8);
    }
}

void MainWindow::on_pushButton_modifyNickname_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    if(-1 != ui->lineEdit_nickname->text().indexOf("V")
            || -1 != ui->lineEdit_nickname->text().indexOf(":")
            || -1 != ui->lineEdit_nickname->text().indexOf("*")
            || -1 != ui->lineEdit_nickname->text().indexOf("?")
            || -1 != ui->lineEdit_nickname->text().indexOf("\"")
            || -1 != ui->lineEdit_nickname->text().indexOf("<")
            || -1 != ui->lineEdit_nickname->text().indexOf(">")) {
        QMessageBox::information(this,tr("Nickname"),tr("Nickname cannot include:") + "V:*?\"<>|" );
    } else {
        deviceItem->camera->rename(ui->lineEdit_nickname->text().trimmed().toStdString());
        QMessageBox::information(this,tr("Nickname"),tr("Nickname has been modifyed and to restart the application!"));
    }
}

void MainWindow::on_checkBox_whiteBalanceWindow_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->whiteBalanceWindow = arg1;
}

void MainWindow::on_pushButton_setWhiteBalanceWindow_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto rect = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->whiteBalanceWindowPos;
    deviceItem->camera->white_balance_window(rect.x(),rect.y(),rect.width(),rect.height());
}

void MainWindow::on_pushButton_defaultWhiteBalanceWindow_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->whiteBalanceWindowPos = cs->background->pixmap().rect();
}

void MainWindow::on_checkBox_deadPixelsWindow_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->deadPixelWindow = arg1;
}

void MainWindow::on_pushButton_saveDeadPixels_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());

    string x,y;
    for(auto pos : cs->existedPixels + cs->manualPixels + cs->deadPixels + cs->brightPixels) {
        x += pos.x() + ",";
        y += pos.y() + ",";
    }

    if(!x.empty()) x.erase(x.length()-1,1);
    deviceItem->camera->dead_pixels(x,y);

    QMessageBox::information(this, tr("Dead Pixels"), tr("Saved Dead Pixels!"), QMessageBox::Ok);
}

void MainWindow::on_checkBox_flatFiledCorrect_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->flat_field_corrent(arg1);
}

void MainWindow::on_pushButton_darkField_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    try {
        deviceItem->camera->flat_field_init(0);
        QMessageBox::information(this, tr("Dark Field"), tr("Ok!"), QMessageBox::Ok);
    } catch(...) {
        QMessageBox::warning(nullptr, tr("Light Field"), tr("Light is not enough!"), QMessageBox::Ok);
    }
}

void MainWindow::on_pushButton_lightField_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    try {
        deviceItem->camera->flat_field_init(1);
        QMessageBox::information(this, tr("Light Field"), tr("Ok!"), QMessageBox::Ok);
    } catch(...) {
        QMessageBox::warning(nullptr, tr("Light Field"), tr("Light is not enough!"), QMessageBox::Ok);
    }
}

void MainWindow::on_pushButton_saveFlatFieldParams_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto filename = QFileDialog::getSaveFileName(this,tr("File name"),"",tr("config Files(*.ffc )"));
    if(filename.isEmpty()) return;
    deviceItem->camera->flat_field_params_save(filename.replace('/','\\').toStdString());
}

void MainWindow::on_pushButton_loadFlatFieldParams_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto filename = QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("config Files(*.ffc )"));
    if(filename.isEmpty()) return;
    deviceItem->camera->flat_field_params_load(filename.replace('/','\\').toStdString());
}

void MainWindow::on_pushButton_calibration_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem->calibrationDialog.exec())
        return;

    deviceItem->camera->undistory_params(deviceItem->calibrationDialog.width(),deviceItem->calibrationDialog.height(),deviceItem->calibrationDialog.cameraMatraix().toStdString(),deviceItem->calibrationDialog.distortCoeffs().toStdString());
    QMessageBox::information(this,tr("Calibration"),tr("Apply ok!"));
}

void MainWindow::on_slider_brightness_sliderMoved(int position)
{
    ui->spinBox_brightness->setValue(position);
    emit ui->spinBox_brightness->editingFinished();
}

void MainWindow::on_spinBox_brightness_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_brightness->value();
    ui->slider_brightness->setValue(v);
    deviceItem->camera->brightness(v);
}

void MainWindow::on_slider_gain_sliderMoved(int position)
{
    ui->spinBox_gain->setValue(position / 100.);
    emit ui->spinBox_gain->editingFinished();
}

void MainWindow::on_spinBox_gain_valueChanged(double value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto maximum = ui->spinBox_gain->maximum() / 100.;

    if(maximum < value) {
        ui->spinBox_gain->setValue(maximum);
    }

    auto v = ui->spinBox_gain->value()  * 100.;
    ui->slider_gain->setValue(v);
    deviceItem->camera->gain(v);
}

void MainWindow::on_slider_exposureTime_sliderMoved(int position)
{
    ui->spinBox_exposureTime->setValue(position / 1000.);
    emit ui->spinBox_exposureTime->editingFinished();
}

void MainWindow::on_spinBox_exposureTime_valueChanged(double value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto maximum = ui->spinBox_exposureTime->maximum() / 1000.;

    if(maximum < value) {
        ui->spinBox_exposureTime->setValue(maximum);
    }

    auto v = ui->spinBox_exposureTime->value() * 1000.;
    ui->slider_exposureTime->setValue(static_cast<int>(v));
    deviceItem->camera->exposure_time(v);
}

void MainWindow::on_spinBox_gainMinimum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->gain_range(ui->spinBox_gainMinimum->value() * 100.f,ui->spinBox_gainMaximum->value() * 100.f);
}

void MainWindow::on_spinBox_gainMaximum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->gain_range(ui->spinBox_gainMinimum->value() * 100.f,ui->spinBox_gainMaximum->value() * 100.f);
}

void MainWindow::on_spinBox_exposureTimeMaximum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->exposure_time_range(ui->spinBox_exposureTimeMinimum->value() * 1000.,ui->spinBox_exposureTimeMaximum->value() * 1000.);
}

void MainWindow::on_spinBox_exposureTimeMinimum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->exposure_time_range(ui->spinBox_exposureTimeMinimum->value() * 1000.,ui->spinBox_exposureTimeMaximum->value() * 1000.);
}

void MainWindow::on_slider_r_sliderMoved(int position)
{
    ui->spinBox_r->setValue(position / 100.f);
    emit ui->spinBox_r->editingFinished();
}

void MainWindow::on_spinBox_r_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_r->value() * 100.f;
    deviceItem->camera->rgb(v,ui->slider_g->value(),ui->slider_b->value());
    ui->slider_r->setValue(v);
}

void MainWindow::on_slider_g_sliderMoved(int position)
{
    ui->spinBox_g->setValue(position / 100.f);
    emit ui->spinBox_g->editingFinished();
}

void MainWindow::on_spinBox_g_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_g->value() * 100.f;
    deviceItem->camera->rgb(ui->slider_r->value(),v,ui->slider_b->value());
    ui->slider_g->setValue(v);
}

void MainWindow::on_slider_b_sliderMoved(int position)
{
    ui->spinBox_b->setValue(position / 100.f);
    emit ui->spinBox_b->editingFinished();
}

void MainWindow::on_spinBox_b_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_b->value() *  100.f;
    deviceItem->camera->rgb(ui->slider_r->value(),ui->slider_g->value(),v);
    ui->slider_b->setValue(v);
}

void MainWindow::on_slider_saturation_sliderMoved(int position)
{
    ui->spinBox_saturation->setValue(position);
    emit ui->spinBox_saturation->editingFinished();
}

void MainWindow::on_spinBox_saturation_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_saturation->value();
    deviceItem->camera->saturation(v);
    ui->slider_saturation->setValue(v);
}

void MainWindow::on_comboBox_lutMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->lookup_table_mode(index);
}

void MainWindow::on_comboBox_lutMode_currentIndexChanged(int index)
{
    ui->groupBox_lutDynamic->hide();
    ui->groupBox_lutPreset->hide();
    ui->groupBox_lutCustom->hide();

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    if(index == 0) {
        auto lookupTables = deviceItem->lookupTablesForDynamic();
        ui->slider_gamma->setMinimum(lookupTables[0].toUInt());
        ui->slider_gamma->setMaximum(lookupTables[1].toUInt());
        emit ui->slider_gamma->sliderMoved(lookupTables[2].toUInt());
        ui->spinBox_gamma->setMinimum(lookupTables[0].toUInt() / 100.f);
        ui->spinBox_gamma->setMaximum(lookupTables[1].toUInt() / 100.f);
        ui->spinBox_gamma->setValue(lookupTables[2].toUInt() / 100.f);

        ui->slider_contrastRatio->setMinimum(lookupTables[3].toUInt());
        ui->slider_contrastRatio->setMaximum(lookupTables[4].toUInt());
        ui->slider_contrastRatio->setValue(lookupTables[5].toUInt());
        ui->spinBox_contrastRatio->setMinimum(lookupTables[3].toUInt());
        ui->spinBox_contrastRatio->setMaximum(lookupTables[4].toUInt());
        ui->spinBox_contrastRatio->setValue(lookupTables[5].toUInt());
        ui->groupBox_lutDynamic->show();
    } else if(index == 1) {
        auto lookupTables = deviceItem->lookupTablesForPreset();
        ui->comboBox_lutPreset->setCurrentIndex(lookupTables[0].toUInt());
        ui->groupBox_lutPreset->show();
    } else if(index == 2) {
        ui->comboBox_lutColorChannel->setCurrentIndex(0);
        ui->groupBox_lutCustom->show();
    } else {}
}

void MainWindow::on_spinBox_gamma_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_gamma->value() * 100;
    deviceItem->camera->gamma(v);
    ui->slider_gamma->setValue(v);

    auto lookupTables = deviceItem->lookupTablesForDynamic();

    auto line = new QSplineSeries();
    auto values = lookupTables[6].split(',');
    for(int i=0;i < values.size();i++) {
        line->append(i, values[i].toInt());
    }

    ui->chartView_lut->chart()->removeAllSeries();
    ui->chartView_lut->chart()->addSeries(line);
    ui->chartView_lut->chart()->createDefaultAxes();
}

void MainWindow::on_slider_gamma_sliderMoved(int position)
{
    ui->spinBox_gamma->setValue(position / 100.f);
    emit ui->spinBox_gamma->editingFinished();
}

void MainWindow::on_spinBox_contrastRatio_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v= ui->spinBox_contrastRatio->value();
    deviceItem->camera->contrast_ratio(v);
    ui->slider_contrastRatio->setValue(v);

    auto lookupTables = deviceItem->lookupTablesForDynamic();

    auto line = new QSplineSeries();
    auto values = lookupTables[6].split(',');
    for(int i=0;i < values.size();i++) {
        line->append(i, values[i].toInt());
    }

    ui->chartView_lut->chart()->removeAllSeries();
    ui->chartView_lut->chart()->addSeries(line);
    ui->chartView_lut->chart()->createDefaultAxes();
}

void MainWindow::on_slider_contrastRatio_sliderMoved(int position)
{
    ui->spinBox_contrastRatio->setValue(position);
    emit ui->spinBox_contrastRatio->editingFinished();
}

void MainWindow::on_checkBox_anamorphose_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->undistort(checked);
}

void MainWindow::on_pushButton_layout_clicked()
{
    ui->tabWidget_preview->setCurrentIndex(ui->tabWidget_preview->currentIndex() % 2 + 1);
}

void MainWindow::on_spinBox_threshold_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->threshold(ui->spinBox_threshold->value());
}

void MainWindow::on_pushButton_analyzeDeadPixels_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto xylist = deviceItem->dead_pixels_analyze_for_dead(ui->spinBox_deadPixelThreshold->value());

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->deadPixels.clear();

    for(auto xystr : xylist) {
        auto xy = xystr.split(',');
        cs->deadPixels.append(QPoint(xy[0].toUInt(),xy[1].toUInt()));
    }

    QMessageBox::information(nullptr, tr("Dead Pixels"), tr(QString("Found %1 points!").arg(xylist.size()).toLocal8Bit()), QMessageBox::Ok);
}

void MainWindow::on_pushButton_analyzeBrightPixels_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto xylist = deviceItem->dead_pixels_analyze_for_bright(ui->spinBox_deadPixelThreshold->value());

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->brightPixels.clear();

    for(auto xystr : xylist) {
        auto xy = xystr.split(',');
        cs->brightPixels.append(QPoint(xy[0].toUInt(),xy[1].toUInt()));
    }

    QMessageBox::information(nullptr, tr("Bright Pixels"), tr(QString("Found %1 points!").arg(xylist.size()).toLocal8Bit()), QMessageBox::Ok);
}

void MainWindow::on_pushButton_clearOldPixels_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->existedPixels.clear();
}

void MainWindow::on_pushButton_clearNewPixels_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->manualPixels.clear();
    cs->deadPixels.clear();
    cs->brightPixels.clear();
}

void MainWindow::on_slider_outputRange_sliderMoved(int position)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->raw_output_range(position);

    if(position < 0) {
        ui->label_outputRange->setText(tr("Full Output"));
    } else {
        ui->label_outputRange->setText(QString("%1~%2").arg(position+1).arg(position+8) + tr("Output"));
    }
}

void MainWindow::on_comboBox_outputFormats_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->raw_output_format(index);
}

void MainWindow::on_pushButton_low8Bit_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    ui->slider_outputRange->setValue(0);
    emit ui->slider_outputRange->sliderMoved(0);
}

void MainWindow::on_pushButton_high8Bit_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    ui->slider_outputRange->setValue(ui->slider_outputRange->maximum());
    emit ui->slider_outputRange->sliderMoved(ui->slider_outputRange->maximum());
}

void MainWindow::on_comboBox_outputFormats_currentIndexChanged(int index)
{
    if(index) {
        ui->groupBox_outputRange->setEnabled(true);
    } else {
        ui->groupBox_outputRange->setDisabled(true);
    }
}

void MainWindow::on_treeWidget_devices_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    emit ui->action_open->triggered();
}

void MainWindow::on_checkBox_exposureWIndow_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->exposureWindow = arg1;
}

void MainWindow::on_pushButton_setExposureWindow_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto rect = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->exposureWindowPos;
    deviceItem->camera->exposure_window(rect.x(),rect.y(),rect.width(),rect.height());
}

void MainWindow::on_radioButton_manualExposure_toggled(bool checked)
{
    ui->groupBox_manualExposure->setEnabled(checked);
    ui->checkBox_exposureWIndow->setChecked(false);
}

void MainWindow::on_radioButton_manualExposure_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->exposure_mode(0);
}

void MainWindow::on_radioButton_automationExposure_toggled(bool checked)
{
    ui->groupBox_automationExposure->setEnabled(checked);
    ui->checkBox_exposureWIndow->setChecked(false);
}

void MainWindow::on_radioButton_automationExposure_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->exposure_mode(1);
}

void MainWindow::on_radioButton_presetResolution_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    ui->groupBox_resolutionPreset->setHidden(0);
    ui->groupBox_resolutionCustom->setVisible(0);

    ui->comboBox_resolution->clear();
    for(auto s : deviceItem->resolutions()) {
        auto r = s.split(' ');

        QString rs = r[0] + ' ';

        if(r.size() > 1) {
            r[1] = tr(r[1].toLocal8Bit());
            rs += r[1] + ' ';
        }

        if(r.size() > 2) {
            r[2] = tr(r[2].toLocal8Bit());
            rs += r[2] + ' ';
        }
        ui->comboBox_resolution->addItem(rs);
    }

    auto resolution = deviceItem->resolutionIndex();
    ui->comboBox_resolution->setCurrentIndex(resolution.toInt());

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->resolutionWindow = 0;
}

void MainWindow::on_radioButton_customResolution_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    ui->groupBox_resolutionPreset->setHidden(1);
    ui->groupBox_resolutionCustom->setVisible(1);

    deviceItem->camera->resolution(0);
    auto resolution = deviceItem->resolution();
    ui->spinBox_resolutionX->setValue(resolution[0].toInt());
    ui->spinBox_resolutionY->setValue(resolution[1].toInt());
    ui->spinBox_resolutionW->setValue(resolution[2].toInt());
    ui->spinBox_resolutionH->setValue(resolution[3].toInt());

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->resolutionWindow = 1;
}

void MainWindow::on_checkBox_showBright_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->cameraView->avgBrightness = arg1;
}

void MainWindow::on_radioButton_wbManual_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->white_balance_mode(0);
}

void MainWindow::on_radioButton_wbAutomation_toggled(bool checked)
{
    ui->slider_r->setDisabled(checked);
    ui->slider_g->setDisabled(checked);
    ui->slider_b->setDisabled(checked);
    ui->slider_saturation->setDisabled(checked);
    ui->spinBox_r->setReadOnly(checked);
    ui->spinBox_g->setReadOnly(checked);
    ui->spinBox_b->setReadOnly(checked);
    ui->spinBox_saturation->setReadOnly(checked);
}

void MainWindow::on_radioButton_wbAutomation_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->white_balance_mode(1);
}

void MainWindow::on_tabWidget_preview_currentChanged(int index)
{
    auto f = [=]() {
        for(auto topItemIndex=0;topItemIndex<ui->treeWidget_devices->topLevelItemCount();topItemIndex++) {
            auto topLevelItem = ui->treeWidget_devices->topLevelItem(topItemIndex);

            for(auto subItemIndex=0;subItemIndex< topLevelItem->childCount();subItemIndex++) {
                auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(subItemIndex));

                if(!deviceItem->camera->opened()) continue;

                auto i=0,j=ui->tab_all_contents->columnCount();
                if(ui->tab_all_contents->count() < j*j) {
                    for(i=0;i< ui->tab_all_contents->columnCount();i++) {
                        ui->tab_all_contents->setRowStretch(i,1);
                        for (j=0;j < ui->tab_all_contents->columnCount();j++) {
                            ui->tab_all_contents->setColumnStretch(j,1);
                            auto item = ui->tab_all_contents->itemAtPosition(i,j);
                            if(!item) goto NEW_ITEM;
                            if(item->widget() == deviceItem->cameraView) goto OLD_ITEM;
                        }
                    }
                }
NEW_ITEM:
                ui->tab_all_contents->addWidget(deviceItem->cameraView,i,j);
OLD_ITEM:
                ;
            }
        }
    };

    viewStatusUpdate.stop();

    if(index == 1) {
        auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
        if(deviceItem && -1 == ui->tab_main_contents->indexOf(deviceItem->cameraView)) {
            if(ui->tab_main_contents->count())
                ui->tab_main_contents->itemAt(0)->widget()->setParent(nullptr);
            ui->tab_main_contents->addWidget(deviceItem->cameraView);
        }
    } else if(index == 2) {
        connect(&viewStatusUpdate,&QTimer::timeout,f);
        viewStatusUpdate.setInterval(2000);
        viewStatusUpdate.start();
    }
}

void MainWindow::on_pushButton_fpnEdit_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    OfflineFpnDialog fpn(nullptr,deviceItem->cameraView->snapshot());
    if(QDialog::Accepted == fpn.exec()) {
        deviceItem->camera->fpn_load(fpn.fpnfilepath.toStdString());
    }
}

void MainWindow::on_pushButton_fpnClear_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->fpn_clear();
}

void MainWindow::on_checkBox_fpn_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->fpn(checked);
}

void MainWindow::on_pushButton_fpnSave_clicked()
{
    auto filename = QFileDialog::getSaveFileName(this,tr("File name"),"",tr("config Files(*.fpn )"));
    if(filename.isEmpty()) return;

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->camera->fpn_save(filename.toStdString());
}


void MainWindow::on_comboBox_infrared_thermometry_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    auto camera_gf120 = std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_thermometry(index);

    if(index == 1)
    {
        ui->spinBox_left_backbody_furnace_temperature->setValue(150);
        ui->spinBox_right_backbody_furnace_temperature->setValue(30);
    }
    if(index == 2)
    {
        ui->spinBox_left_backbody_furnace_temperature->setValue(400);
        ui->spinBox_right_backbody_furnace_temperature->setValue(30);
    }
    if(index == 3)
    {
        ui->spinBox_left_backbody_furnace_temperature->setValue(1200);
        ui->spinBox_right_backbody_furnace_temperature->setValue(30);
    }
}

void MainWindow::on_comboBox_infrared_color_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_color(index);
}

void MainWindow::on_comboBox_display_mode_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_display(index);
}

void MainWindow::on_checkBox_infrared_shutter_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_shutter(checked);
}

void MainWindow::on_checkBox_infrared_cool_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_cool(checked);
}

void MainWindow::on_spinBox_infrared_emissivity_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_emissivity(arg1);
}

void MainWindow::on_spinBox_infrared_sharpen_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_sharpen(arg1);
}

void MainWindow::on_spinBox_infrared_exposure_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_exposure(arg1);
}

void MainWindow::on_spinBox_dde_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_dde(arg1);
}

void MainWindow::on_checkBox_infrared_manual_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto value = ui->spinBox_infrared_manual->value() * 100;
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_manual(checked,value);
}

void MainWindow::on_pushButton_infrared_temperature_check_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->temperature_check();
}

void MainWindow::on_checkBox_infrared_stop_temperature_check_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_temperature_check_stop(checked);
}

void MainWindow::on_checkBox_infrared_shutter_temperature_raise_sample_clicked(bool checked){
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_shutter_temperature_raise_sample(checked);

    if(checked) //采样模式下不允许触发校正
    {
        ui->checkBox_infrared_response_rate_sample->setEnabled(false);
        ui->checkBox_infrared_temperature_curve_sample->setEnabled(false);
    }
    else
    {
        ui->checkBox_infrared_response_rate_sample->setEnabled(true);
        ui->checkBox_infrared_temperature_curve_sample->setEnabled(true);
    }
}

void MainWindow::on_checkBox_infrared_factory_check_detect_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_factory_check_detect(checked);
}

void MainWindow::on_checkBox_infrared_response_rate_sample_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_response_rate_sample(checked);

    if(checked) //采样模式下不允许触发校正
    {
        ui->checkBox_infrared_temperature_curve_sample->setEnabled(false);
        ui->checkBox_infrared_shutter_temperature_raise_sample->setEnabled(false);
        ui->spinBox_infrared_exposure->setEnabled(true);
        ui->pushButton_sample_low_response_rate->setEnabled(true);
        ui->pushButton_sample_high_response_rate->setEnabled(true);
        ui->pushButton_load_response_rate_file->setEnabled(true);
    }
    else
    {
        ui->checkBox_infrared_temperature_curve_sample->setEnabled(true);
        ui->checkBox_infrared_shutter_temperature_raise_sample->setEnabled(true);
        ui->spinBox_infrared_exposure->setEnabled(false);
        ui->pushButton_sample_low_response_rate->setEnabled(false);
        ui->pushButton_sample_high_response_rate->setEnabled(false);
        ui->pushButton_load_response_rate_file->setEnabled(false);
    }
}

void MainWindow::on_checkBox_infrared_temperature_curve_sample_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_temperature_curve_sample(checked);

    if(checked) //采样模式下不允许触发校正
    {
        ui->checkBox_infrared_response_rate_sample->setEnabled(false);
        ui->checkBox_infrared_temperature_curve_sample->setEnabled(true);
        ui->checkBox_infrared_shutter_temperature_raise_sample->setEnabled(false);
        ui->spinBox_infrared_exposure->setEnabled(false);
    }
    else
    {
        ui->checkBox_infrared_response_rate_sample->setEnabled(true);
        ui->checkBox_infrared_temperature_curve_sample->setEnabled(true);
        ui->checkBox_infrared_shutter_temperature_raise_sample->setEnabled(true);
        ui->spinBox_infrared_exposure->setEnabled(false);
    }
}

//拷贝文件夹：
bool MainWindow::copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists()){    /**< 如果目标目录不存在，则进行创建 */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList){
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir()){    /**< 当为目录时，递归的进行copy */
            if(!copyDirectoryFiles(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()),
                coverFileIfExist))
                return false;
        }
        else{            /**< 当允许覆盖操作时，将旧文件进行删除操作 */
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName())){
                targetDir.remove(fileInfo.fileName());
            }

            /// 进行文件copy
            if(!QFile::copy(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()))){
                    return false;
            }
        }
    }
    return true;
}

int				temp_data_mode1[0x10000];			//温度模式1的温度曲线查找表
int				temp_data_mode2[0x10000];			//温度模式2的温度曲线查找表
//二分法查找
int MainWindow::Bin_Search(int temp)
{
    int first = 0,last = 0x10000 -1,mid;
    int counter = 0;

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto GF120_set_param = camera_gf120->GF120_set_param;

    if(GF120_set_param.temp_mode == 2)
    {
        while(first <= last)
        {
            counter ++;
            mid = (first + last) / 2;//确定中间元素
            if(temp_data_mode2[mid] > temp)
            {
                last = mid-1; //mid已经交换过了,last往前移一位
            }
            else if(temp_data_mode2[mid] < temp)
            {
                first = mid+1;//mid已经交换过了,first往后移一位
            }
            else //判断是否相等
            {
                //printf("查找次数:%d temp :%d value :%d \n",counter,temp,mid);
                return mid;
            }
        }
         //printf("查找次数:%d temp :%d value :%d \n",counter,temp,mid);
        return mid;
    }
    else
    {
        while(first <= last)
        {
            counter ++;
            mid = (first + last) / 2;//确定中间元素
            if(temp_data_mode1[mid] > temp)
            {
                last = mid-1; //mid已经交换过了,last往前移一位
            }
            else if(temp_data_mode1[mid] < temp)
            {
                first = mid+1;//mid已经交换过了,first往后移一位
            }
            else //判断是否相等
            {
                // printf("查找次数:%d temp :%d value :%d \n",counter,temp,mid);
                return mid;
            }
        }
         //printf("查找次数:%d temp :%d value :%d \n",counter,temp,mid);
        return mid;
    }
}


void sleep(unsigned int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::on_pushButton_factory_check_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto GF120_get_param = camera_gf120->status();
    auto& GF120_set_param = camera_gf120->GF120_set_param;

    int i= 0;
    int time = 0;
    //1.先计算锅盖现象引起的偏差
    int diff_cover_ad = 0;
    int diff_ad_value = 50;//(150-30)*70;

    QApplication::setOverrideCursor(Qt::WaitCursor);//设置鼠标为等待状态
    QProgressDialog progress;
    progress.setWindowTitle(QString::fromLocal8Bit("出厂校正"));
    progress.setLabelText(QString::fromLocal8Bit("出厂校正中。。。").arg(GF120_get_param.outside_temp/100));
    progress.setCancelButtonText(QString::fromLocal8Bit("取消"));
    progress.setRange(0, 128);//设置范围
    progress.setModal(true);//设置为模态对话框
    progress.show();

    //2.调节曝光 每摄氏度70个灰度值
    //QMessageBox::information(this, "提示", "出厂校正。。。");
    printf("on_pushButton_factory_check_released\r\n");
    QString src_fileName="./config/corre_default";
    QString dest_fileName = QString("./config/corre_%1").arg(deviceItem->camera->info.acSn);     //str =  "Joy was born in 1993.";
    printf("%s %s \n",src_fileName.toStdString().data(),dest_fileName.toStdString().data());
    copyDirectoryFiles(src_fileName,dest_fileName,0);
    progress.setValue(2);

    int calibration_high = ui->spinBox_left_backbody_furnace_temperature->value()*100;
    int calibration_low = ui->spinBox_right_backbody_furnace_temperature->value()*100;
    printf("calibration_high:%d calibration_low:%d \r\n",calibration_high,calibration_low);

    camera_gf120->infrared_frame_temp_cnt(10);
    int cita = 128;
    camera_gf120->exposure_time(cita);
    sleep(100);

    camera_gf120->sample_mode(FACTORY_SAMPLE_MODE);
    camera_gf120->temperature_check();
    sleep(3000);
    camera_gf120->shutter_correct_for_stop();
    progress.setValue(10);

    if(progress.wasCanceled())
    {
        QApplication::restoreOverrideCursor();
        progress.close();
        return;
    }
    FILE *corre_fp;
    char filename[256];
    sprintf(filename,"%s/sample_data_mode1.bin",dest_fileName.toStdString().data());
    printf("filename:%s \r\n",filename);

    if((corre_fp=fopen(filename,"rb"))==NULL) //读取1.txt的文件
    {
        printf("open filed %s\r\n ",filename);
        QApplication::restoreOverrideCursor();
        progress.close();
        return;

    }
    else
    {
        fread(&temp_data_mode1, 0x40000, 1, corre_fp);
        fclose(corre_fp);
    }
    sprintf(filename,"%s/sample_data_mode2.bin",dest_fileName.toStdString().data());
    if((corre_fp=fopen(filename,"rb"))==NULL) //读取1.txt的文件
    {
        printf("open filed %s\r\n ",filename);
        QApplication::restoreOverrideCursor();
        progress.close();
        return;
    }
    else
    {
        fread(&temp_data_mode2, 0x40000, 1, corre_fp);
        fclose(corre_fp);
    }

    printf("calibration_high_bin:%d calibration_low:%d \r\n",Bin_Search(calibration_high),Bin_Search(calibration_low));
    diff_ad_value = abs(Bin_Search(calibration_high) - Bin_Search(calibration_low))/4 -80;
    int last_ad_value = 0;

    printf("on_pushButton_factory_check_released diff_ad_value:%d %d %d \r\n",diff_ad_value,Bin_Search(calibration_high),Bin_Search(calibration_low));
    camera_gf120->exposure_time(cita);
    sleep(500);
    GF120_get_param = camera_gf120->status();
    int last_ad_diff = 0;
    printf("last_ad_diff :%d \r\n",last_ad_diff);
    for(i = 0 ; i < 128 ; i ++)
    {
         sleep(1000);
         progress.setValue(i + 10);
         GF120_get_param = camera_gf120->status();
         if(i == 0)
         {
             if(abs(GF120_get_param.diff_ad_value - diff_ad_value) <= 30)
                 break;
         }
         printf("i:%d last_ad_diff :%d %d \r\n",i,last_ad_diff,abs(GF120_get_param.diff_ad_value - diff_ad_value));
         printf("qt diff_ad_value :%d %d\r\n",GF120_get_param.diff_ad_value,diff_ad_value);
         //QMessageBox::information(this, "提示", QString("实际灰度差值:%1 计算灰度差值:%2").arg(GF120_get_param.diff_ad_value).arg(diff_ad_value));
         if(i > 0 && (abs(GF120_get_param.diff_ad_value - diff_ad_value) >= last_ad_diff) && (abs(GF120_get_param.diff_ad_value - diff_ad_value) <= 100)) //已经找到最优值的后一个值 取前一个值
         {
             //记录曝光值
             if(cita > 128)
                 cita =  cita - 1;
             else
                 cita =  cita + 1;
              printf("found best cita value :%d \r\n",cita);
              camera_gf120->exposure_time(cita);
              break;
         }
         else if((GF120_get_param.diff_ad_value - diff_ad_value) > 40)
         {
             cita ++;
         }
         else
         {
            cita --;
         }
         last_ad_diff = abs(GF120_get_param.diff_ad_value - diff_ad_value); //上一次的差值
         //cita 超过范围或者改变cita后ad值变化太小说明有问题

          printf("last_ad_value :%d GF120_get_param.diff_ad_value：%d\r\n",last_ad_value,GF120_get_param.diff_ad_value);
         if(cita < 16 || cita > 224 || (abs(last_ad_value - GF120_get_param.diff_ad_value) <= 4) || progress.wasCanceled())
         {
            QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("1.请检查高低温黑体炉的设置温度和界面设置温度是否一致\r\n2.高低温黑体炉是否对中左右十字星"));
            QApplication::restoreOverrideCursor();
            progress.close();
            return;
         }
         last_ad_value = GF120_get_param.diff_ad_value;
         if(cita%16 == 0)   //随着曝光变化 需要调节GSK NUC
         {
             camera_gf120->temperature_check();
             sleep(5000);
         }
          camera_gf120->exposure_time(cita);
    }
    //QMessageBox::information(this, "提示", QString("曝光值设置正确:%1").arg(cita));
    printf("check OK cita :%d \r\n",cita);
    printf("diff_ad_value :%d %d\r\n",GF120_get_param.diff_ad_value,diff_ad_value);
    progress.setValue(100);

    //3.计算快门偏移
    camera_gf120->temperature_check();
    sleep(5000);
    progress.setValue(110);
    GF120_get_param = camera_gf120->status();
    printf("calibration_high_temp:%d calibration_low_temp:%d\r\n",GF120_get_param.calibration_high_temp,GF120_get_param.calibration_low_temp);
    calibration_high = (calibration_high - GF120_get_param.calibration_high_temp);
    calibration_low = (calibration_low - GF120_get_param.calibration_low_temp);
    int shutter_offset = 0;
    shutter_offset = (calibration_high + calibration_low)/2;
    printf("/////////////shutter_offset:%d calibration_high:%d calibration_low:%d\r\n",shutter_offset,calibration_high,calibration_low);
    if(shutter_offset > 150)
    {
        shutter_offset = 150;
    }
    if(shutter_offset < -150)
    {
        shutter_offset = -150;
    }
    printf("cal shutter_offset:%d \r\n",shutter_offset);

    QString file;

    if(GF120_set_param.temp_mode == 1)
    {
        file = dest_fileName + "/correction_temp_mode_1.txt";
    }
    else  if(GF120_set_param.temp_mode == 2)
    {
        file = dest_fileName + "/correction_temp_mode_2.txt";
    }

    QFile fileSave(file);
    if( ! fileSave.open( QIODevice::WriteOnly ))
    {
        //无法打开要写入的文件
        QMessageBox::warning(this, QString::fromLocal8Bit("打开写入文件"),QString::fromLocal8Bit("打开要写入的文件失败，请检查文件名和是否具有写入权限！"));
        return;
    }
    QString file_data;
    if(GF120_set_param.temp_mode == 1)
    {
        file_data = QString("shutter_offset = %1    \r\ngst417m_ctia = %2   \r\ngst417m_gfid = 150  \r\ngst417m_gain = 2 \r\n").arg(shutter_offset).arg(cita);
    }
    else  if(GF120_set_param.temp_mode == 2)
    {
         file_data = QString("shutter_offset = %1    \r\ngst417m_ctia = %2   \r\ngst417m_gfid = 80  \r\ngst417m_gain = 6 \r\n").arg(shutter_offset).arg(cita);
    }
    fileSave.write(file_data.toStdString().data());
    //关闭文件
    fileSave.close();
    progress.setValue(128);
    QApplication::restoreOverrideCursor();
    progress.close();

}

void MainWindow::on_pushButton_sample_path_clicked()
{
    auto dirPath = QFileDialog::getExistingDirectory(this,tr("Save Path"));
    if(dirPath.isEmpty()) return;

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    memcpy(&gf120->GF120_set_param.file_path,dirPath.toStdString().data(),dirPath.size());
    auto low_sample_path = dirPath += "/response_temp40.bin";
    auto high_sample_path = dirPath += "/response_temp140.bin";
    ui->lineEdit_infrared_path_to_low_response_rate_sample->setText(low_sample_path);
    ui->lineEdit_infrared_path_to_high_response_rate_sample->setText(high_sample_path);
}

void MainWindow::on_pushButton_sample_low_response_rate_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);

    QString file_path = ui->lineEdit_sample_path->text();
    gf120->response_rate_sample_start(40,file_path.toStdString());

    QApplication::setOverrideCursor(Qt::WaitCursor);//设置鼠标为等待状态
    QProgressDialog progress;
    progress.setWindowTitle(QString::fromLocal8Bit("响应率采集"));
    progress.setLabelText(QString::fromLocal8Bit("请确保对中低温黑体,采集低温响应率样本中 。。。"));
    progress.setCancelButtonText(QString::fromLocal8Bit("取消"));
    progress.setRange(0, 100);//设置范围
    progress.setModal(true);//设置为模态对话框
    progress.show();


    int i = 0;
    for (; i < 100; i++)
    {
         sleep(100);
         progress.setValue(i);
         gf120->response_rate_sample_status();

         //用户取消的话则中止
         if (progress.wasCanceled())
         {
             QMessageBox::critical(this, QString::fromLocal8Bit("错误"),  QString::fromLocal8Bit("取消采集低温响应率样本！").arg(gf120->GF120_set_param.collect_response_temp));
             break;
         }

         QCoreApplication::processEvents();
     }
     QApplication::restoreOverrideCursor();
     progress.close();
     if(i == 100) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), tr("采集低温响应率样本超时!").arg(gf120->GF120_set_param.collect_response_temp));
     else QMessageBox::information(this, QString::fromLocal8Bit("提示"), tr("采集低温响应率样本完成！").arg(gf120->GF120_set_param.collect_response_temp));
     gf120->response_rate_sample_stop();
}

void MainWindow::on_pushButton_sample_high_response_rate_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    QString file_path = ui->lineEdit_sample_path->text();
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    gf120->response_rate_sample_start(140,file_path.toStdString());

    QApplication::setOverrideCursor(Qt::WaitCursor);//设置鼠标为等待状态
    QProgressDialog progress;
    progress.setWindowTitle(QString::fromLocal8Bit("响应率采集"));
    progress.setLabelText(QString::fromLocal8Bit("请确保对中低温黑体,采集低温响应率样本中 。。。"));
    progress.setCancelButtonText(QString::fromLocal8Bit("取消"));
    progress.setRange(0, 100);//设置范围
    progress.setModal(true);//设置为模态对话框
    progress.show();

    int i = 0;
    for (; i < 100; i++)
    {
         sleep(100);
         progress.setValue(i);
         gf120->response_rate_sample_status();

         //用户取消的话则中止
         if (progress.wasCanceled())
         {
             QMessageBox::critical(this, QString::fromLocal8Bit("错误"),  QString::fromLocal8Bit("取消采集低温响应率样本！").arg(gf120->GF120_set_param.collect_response_temp));
             break;
         }

         QCoreApplication::processEvents();
     }

     QApplication::restoreOverrideCursor();
     progress.close();

     if(i == 100) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), tr("采集低温响应率样本超时!").arg(gf120->GF120_set_param.collect_response_temp));
     else QMessageBox::information(this, QString::fromLocal8Bit("提示"), tr("采集低温响应率样本完成！").arg(gf120->GF120_set_param.collect_response_temp));

     gf120->response_rate_sample_stop();
}



void MainWindow::on_checkBox_infrared_osd_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_osd(checked);
}

void MainWindow::on_checkBox_infrared_temperature_display_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_temperature_display(checked);
    ui->checkBox_infrared_osd->setChecked(checked);
}

void MainWindow::on_checkBox_infrared_roi_clicked(bool checked)
{
    int index = ui->comboBox_infrared_temperture_roi->currentIndex();
    typedef unsigned short USHORT;
    USHORT  user_width_start;			//用户设置区域检测温度区宽开始点
    USHORT  user_width_number;			//用户设置区域检测温度区宽像素个数
    USHORT  user_high_start;			//用户设置区域检测温度区高开始点
    USHORT  user_high_number;			//用户设置区域检测温度区宽像素个数
    USHORT  user_roi_emissivity;

    user_width_start = ui->spinBox_infrared_width_start->value();
    user_width_number = ui->spinBox_infrared_width_end ->value();
    user_high_start = ui->spinBox_infrared_high_start->value();
    user_high_number = ui->spinBox_infrared_high_end->value();
    user_roi_emissivity = ui->spinBox_infrared_roi_emissivity->value();
    if((user_width_start + user_width_number ) > 400)
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"),  QString::fromLocal8Bit("总宽度不能大于400"));
        return;
    }
    if((user_high_start + user_high_number) > 300)
    {
        QMessageBox::critical(this,  QString::fromLocal8Bit("错误"),  QString::fromLocal8Bit("总高度不能大于300"));
        return;
    }

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_roi(checked,index,user_width_start,user_width_number,user_high_start,user_high_number,user_roi_emissivity);
}

void MainWindow::on_comboBox_infrared_temperture_roi_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto status = camera_gf120->infrared_temperature_roi_status()[index];

    ui->checkBox_infrared_roi->setChecked(status.user_roi_enable);
    ui->spinBox_infrared_width_start->setValue(status.user_width_start);
    ui->spinBox_infrared_width_end ->setValue(status.user_width_number);
    ui->spinBox_infrared_high_start->setValue(status.user_high_start);
    ui->spinBox_infrared_high_end->setValue(status.user_high_number);
    ui->spinBox_infrared_roi_emissivity->setValue(status.user_roi_emissivity);
}


void MainWindow::on_checkBox_infrared_blackbody_calibrate_clicked(bool checked)
{
    if(ui->spinBox_infrared_width_start->value() >= ui->spinBox_infrared_width_end ->value())
    {
        QMessageBox::critical(this, QStringLiteral("错误"), tr("宽开始不能大于宽结束"));
        return;
    }
    if(ui->spinBox_infrared_high_start->value() >= ui->spinBox_infrared_high_end->value())
    {
        QMessageBox::critical(this, QStringLiteral("错误"), tr("高开始不能大于高结束"));
        return;
    }

    auto user_calibration_temp = ui->doubleSpinBox_infrared_blackbody_temperature->value() *100;
    auto user_width_start = ui->spinBox_infrared_blackbody_width_start->value();
    auto user_width_end = ui->spinBox_infrared_blackbody_width_end->value();
    auto user_high_start = ui->spinBox_infrared_blackbody_high_start->value();
    auto user_high_end = ui->spinBox_infrared_blackbody_high_end->value();

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_blackbody_calibrate(checked,user_calibration_temp,user_width_start,user_width_end,user_high_start,user_high_end);
}

void MainWindow::on_checkBox_infrared_color_map_clicked(bool checked)
{
    auto low = ui->spinBox_infrared_fake_color_low->value()*100;
    auto high = ui->spinBox_infrared_fake_color_high->value()*100;

    if(low >= high)
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请检查输入 低温大于高温"));
        return;
    }

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_color_map(checked,low,high);
}

void MainWindow::on_doubleSpinBox_infrared_temperature_compensation_valueChanged(double arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_temperature_compensation(arg1*100);
}

void MainWindow::on_doubleSpinBox_infrared_distance_compensation_valueChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_distance_compensation(arg1);
}

void MainWindow::on_doubleSpinBox_infrared_humidity_compensation_valueChanged(double arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_humidity_compensation(arg1 * 100);
}

void MainWindow::on_checkBox_infrared_high_warm_clicked(bool checked)
{
    auto temperature = ui->doubleSpinBox_infrared_high_warm->value()*100;
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_high_warm(checked,temperature);
}


void MainWindow::on_checkBox_infrared_low_warm_clicked(bool checked)
{
    auto  temperature = ui->doubleSpinBox_infrared_low_warm->value()*100;
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto camera_gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    camera_gf120->infrared_low_warm(checked,temperature);
}


void MainWindow::on_pushButton_load_response_rate_file_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    auto path = ui->lineEdit_infrared_path_to_low_response_rate_sample->text();
    auto path2 = ui->lineEdit_infrared_path_to_high_response_rate_sample->text();
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto rt = gf120->response_rate_sample_file_load(path.toStdString(),path2.toStdString());

    if(rt < 0) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("加载响应率文件错误，请检查响应率配置文件！"));
    else QMessageBox::information(this, QString::fromLocal8Bit("正常"), QString::fromLocal8Bit("加载响应率文件完成！"));
}


void MainWindow::on_pushButton_cover_low_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());


    auto file_path = ui->lineEdit_sample_path->text();
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    gf120->cover_sample_start(30,file_path.toStdString());
    auto GF120_get_param = gf120->status();

    QApplication::setOverrideCursor(Qt::WaitCursor);//设置鼠标为等待状态
    QProgressDialog progress;
    progress.setWindowTitle(QString::fromLocal8Bit("锅盖采集"));
    progress.setLabelText(QString::fromLocal8Bit("采集%1℃锅盖样本中 。。。").arg(GF120_get_param.outside_temp/100));
    progress.setCancelButtonText(QString::fromLocal8Bit("取消"));
    progress.setRange(0, 100);//设置范围
    progress.setModal(true);//设置为模态对话框
    progress.show();
    int i;
    for (i = 0; i < 100; i++)
    {
        sleep(100);
        progress.setValue(i);
        auto rt = gf120->cover_sample_status();
        printf("cover status :%d \r\n",rt);
        if(rt == 0)
        {
            QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("采集%1℃锅盖样本完成！").arg(gf120->GF120_set_param.collect_cover_temp));
            break;
        }
        //用户取消的则中止
        if (progress.wasCanceled())
        {
            QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("取消采集%1℃锅盖样本！").arg(gf120->GF120_set_param.collect_cover_temp));
            break;
        }

        QCoreApplication::processEvents();
    }
    QApplication::restoreOverrideCursor();
    progress.close();
    if(i == 100)
    {
        QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("采集%1℃锅盖样本失败！").arg(gf120->GF120_set_param.collect_cover_temp));
    }

    gf120->cover_sample_stop();
}


void MainWindow::on_pushButton_cover_high_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    auto file_path = ui->lineEdit_sample_path->text();
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    gf120->cover_sample_start(40,file_path.toStdString());
    auto GF120_get_param = gf120->status();

    QApplication::setOverrideCursor(Qt::WaitCursor);//设置鼠标为等待状态
    QProgressDialog progress;
    progress.setWindowTitle("锅盖采集");
    progress.setLabelText(tr("采集%1℃锅盖样本中 。。。").arg(GF120_get_param.outside_temp/100));
    progress.setCancelButtonText("取消");
    progress.setRange(0, 100);//设置范围
    progress.setModal(true);//设置为模态对话框
    progress.show();

    int i;

    for (i = 0; i < 100; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        progress.setValue(i);

        auto rt = gf120->cover_sample_status();

        printf("cover status :%d \r\n",rt);
        if(rt == 0)
        {
            QMessageBox::information(this,  QString::fromLocal8Bit("提示"),  QString::fromLocal8Bit("采集%1℃锅盖样本完成！").arg(gf120->GF120_set_param.collect_cover_temp));
            break;
        }
        //用户取消的则中止
        if (progress.wasCanceled())
        {
            QMessageBox::critical(this,  QString::fromLocal8Bit("错误"),  QString::fromLocal8Bit("取消采集%1℃锅盖样本！").arg(gf120->GF120_set_param.collect_cover_temp));
            break;
        }

        QCoreApplication::processEvents();
    }
    QApplication::restoreOverrideCursor();
    progress.close();
    if(i == 100)
    {
        QMessageBox::critical(this,  QString::fromLocal8Bit("错误"),  QString::fromLocal8Bit("采集%1℃锅盖样本失败！").arg(gf120->GF120_set_param.collect_cover_temp));
    }

    gf120->cover_sample_stop();
}


void MainWindow::on_pushButton_cover_load_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    auto path = ui->lineEdit_path_cover_low->text();
    auto path2 = ui->lineEdit_path_cover_high->text();

    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto rt = gf120->response_rate_sample_file_load(path.toStdString(),path2.toStdString());

    if(!rt) QMessageBox::critical(this, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("加载响应率文件错误，请检查响应率配置文件！"));
    else QMessageBox::information(this, QString::fromLocal8Bit("正常"), QString::fromLocal8Bit("加载响应率文件完成！"));
}

#include <QtXml/QtXml>


void MainWindow::on_pushButton_save_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    QString file = QString("GF120_%1_%2.xml").arg(deviceItem->camera->info.acSn).arg(2020);
    QString strSaveName = QFileDialog::getSaveFileName(this,QString::fromLocal8Bit("保存配置文件"),file,"xml(*xml)");
    //判断文件名
    if( strSaveName.isEmpty() )
    {
      QMessageBox::warning(this, QString::fromLocal8Bit("打开写入文件"),QString::fromLocal8Bit("操作已经取消"));
      return;
    }
    QFile fileSave(strSaveName);
    printf("strSaveName：%s",strSaveName.data());
    if( ! fileSave.open( QIODevice::WriteOnly ))
    {
        //无法打开要写入的文件
        QMessageBox::warning(this, QString::fromLocal8Bit("打开写入文件"),QString::fromLocal8Bit("打开要写入的文件失败，请检查文件名和是否具有写入权限！"));
        return;
    }

    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto& GF120_set_param = gf120->GF120_set_param;

    //创建xml文档在内存中
   QDomDocument doc;
   //添加处理命令
   QDomProcessingInstruction instruction;
   instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
   //创建注释
   doc.appendChild(instruction); //文档开始声明

   //添加根节点
   QDomElement root=doc.createElement("GF120_config");
   root.setAttribute("Version","2.1");
   doc.appendChild(root);
   //添加第一个子节点及其子元素
   QDomElement element;
   element=doc.createElement("temp_mode");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.temp_mode)));
   root.appendChild(element);

   element=doc.createElement("color_mode");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.color_mode)));
   root.appendChild(element);


   element=doc.createElement("manual_mode");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.manual_mode)));
   root.appendChild(element);

   element=doc.createElement("manual_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.manual_temp)));
   root.appendChild(element);


   element=doc.createElement("user_emissivity");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.user_emissivity)));
   root.appendChild(element);

   element=doc.createElement("sharpen_grade");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.sharpen_grade)));
   root.appendChild(element);


   element=doc.createElement("out_rgb888_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.out_rgb888_enable)));
   root.appendChild(element);

   element=doc.createElement("osd_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.osd_enable)));
   root.appendChild(element);


   element=doc.createElement("qt_img");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.qt_img)));
   root.appendChild(element);

   element=doc.createElement("temp_display_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.temp_display_enable)));
   root.appendChild(element);

   element=doc.createElement("user_compensate_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.user_compensate_temp)));
   root.appendChild(element);

   element=doc.createElement("humidity_compensate_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.humidity_compensate_temp)));
   root.appendChild(element);

   element=doc.createElement("humidity_compensate_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.distance_compensate_temp)));
   root.appendChild(element);

   element=doc.createElement("compensate_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.compensate_temp)));
   root.appendChild(element);


   element=doc.createElement("high_temp_warm_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.high_temp_warm_enable)));
   root.appendChild(element);

   element=doc.createElement("high_temp_warm_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.high_temp_warm_temp)));
   root.appendChild(element);

   element=doc.createElement("low_temp_warm_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.low_temp_warm_enable)));
   root.appendChild(element);

   element=doc.createElement("low_temp_warm_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.low_temp_warm_temp)));
   root.appendChild(element);

   element=doc.createElement("file_path");
   element.appendChild(doc.createTextNode(ui->lineEdit_sample_path->text()));
   root.appendChild(element);

   element=doc.createElement("low_cover_file");
   element.appendChild(doc.createTextNode(ui->lineEdit_path_cover_low->text()));
   root.appendChild(element);

   element=doc.createElement("high_cover_file");
   element.appendChild(doc.createTextNode(ui->lineEdit_path_cover_high->text()));
   root.appendChild(element);

   element=doc.createElement("low_response_file");
   element.appendChild(doc.createTextNode(ui->lineEdit_infrared_path_to_low_response_rate_sample->text()));
   root.appendChild(element);


   element=doc.createElement("high_response_file");
   element.appendChild(doc.createTextNode(ui->lineEdit_infrared_path_to_high_response_rate_sample->text()));
   root.appendChild(element);


   QDomElement GF120_roi=doc.createElement("GF120_roi");
   root.appendChild(GF120_roi);

   element=doc.createElement("user_roi_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_roi.user_roi_enable)));
   GF120_roi.appendChild(element);

  /* element=doc.createElement("user_width_start");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_roi.user_width_start)));
   GF120_roi.appendChild(element);

   element=doc.createElement("user_width_end");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_roi.user_width_end)));
   GF120_roi.appendChild(element);

   element=doc.createElement("user_high_start");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_roi.user_high_start)));
   GF120_roi.appendChild(element);


   element=doc.createElement("user_high_end");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_roi.user_high_end)));
   GF120_roi.appendChild(element);*/

   QDomElement GF120_color=doc.createElement("GF120_color");
   root.appendChild(GF120_color);

   element=doc.createElement("user_color_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_color.user_color_enable)));
   GF120_color.appendChild(element);

   element=doc.createElement("user_color_high");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_color.user_color_high)));
   GF120_color.appendChild(element);

   element=doc.createElement("user_color_low");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_color.user_color_low)));
   GF120_color.appendChild(element);

   QDomElement GF120_calibration=doc.createElement("GF120_calibration");
   root.appendChild(GF120_calibration);

   element=doc.createElement("user_calibration_enable");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_calibration.user_calibration_enable)));
   GF120_calibration.appendChild(element);

   element=doc.createElement("user_calibration_temp");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_calibration.user_calibration_temp)));
   GF120_calibration.appendChild(element);

   element=doc.createElement("user_width_start");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_calibration.user_width_start)));
   GF120_calibration.appendChild(element);

   element=doc.createElement("user_width_end");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_calibration.user_width_end)));
   GF120_calibration.appendChild(element);

   element=doc.createElement("user_high_start");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_calibration.user_high_start)));
   GF120_calibration.appendChild(element);

   element=doc.createElement("user_high_end");
   element.appendChild(doc.createTextNode(tr("%1").arg(GF120_set_param.GF120_calibration.user_high_end)));
   GF120_calibration.appendChild(element);

   //输出到文件
   QTextStream out_stream(&fileSave);
   doc.save(out_stream,4); //缩进4格
   fileSave.close();
}


void MainWindow::on_pushButton_load_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    QString file = QString("GF120_%1_%2.xml").arg(deviceItem->camera->info.acSn).arg(2020);
    QString strSaveName = QFileDialog::getOpenFileName(this,"载入配置文件",file,"xml(*xml)");
    //判断文件名
    if( strSaveName.isEmpty() )
    {
       QMessageBox::warning(this, tr("打开写入文件"),tr("操作已经取消"));
      return;
    }
    QFile fileSave(strSaveName);
    printf("strSaveName：%s",strSaveName.toStdString().data());
    if( ! fileSave.open( QIODevice::ReadOnly ))
    {
        //无法打开要写入的文件
        QMessageBox::warning(this, tr("打开写入文件"),tr("打开要写入的文件失败，请检查文件名和是否具有写入权限！"));
        return;
    }
    QDomDocument doc;
    QString error;
    int row = 0, column = 0;
    //setContent是将指定的内容指定给QDomDocument解析，***参数可以是QByteArray或者是文件名等
    if(!doc.setContent(&fileSave,true,&error,&row,&column))
    {
        //如果出错，则会进入这里。errorStr得到的是出错说明
        QMessageBox::information(NULL, QString("title"), QString("parse file failed at line row and column") + QString::number(row, 10) + QString(",") + QString::number(column, 10));

    }
    if(doc.isNull())
    {
        QMessageBox::information(NULL, QString("title"), QString("document is null!"));
        fileSave.close();
        return;
    }

    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    auto& GF120_set_param = gf120->GF120_set_param;

    QDomElement root = doc.documentElement();

   //root_tag_name为persons
    QString root_tag_name = root.tagName();
    printf("root_tag_name:%s",root_tag_name.toStdString().data());

    QDomNode node=root.firstChild();
    while(!node.isNull())  //如果节点不空
    {

        if(node.isElement()) //如果节点是元素
        {
            //转换为元素
            printf("nodeName:%s value:%d\r\n",node.toElement().tagName().toStdString().data(),node.toElement().text().toInt());
            //qDebug()<<node.nodeName()<<":"<<node.toElement().text();
            if(node.toElement().tagName() == "color_mode")
            {
                GF120_set_param.collect_cover_temp= node.toElement().text().toInt();
            }
            if(node.toElement().tagName() == "color_mode")
            {
                GF120_set_param.color_mode= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "manual_mode")
            {
                GF120_set_param.manual_mode= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "manual_temp")
            {
                GF120_set_param.manual_temp= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "user_emissivity")
            {
                GF120_set_param.user_emissivity= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "sharpen_grade")
            {
                GF120_set_param.sharpen_grade= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "exposure_time")
            {
                GF120_set_param.exposure_time= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "out_rgb888_enable")
            {
                GF120_set_param.out_rgb888_enable= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "osd_enable")
            {
                GF120_set_param.osd_enable= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "temp_display_enable")
            {
                GF120_set_param.temp_display_enable= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "user_compensate_temp")
            {
                GF120_set_param.user_compensate_temp= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "humidity_compensate_temp")
            {
                GF120_set_param.humidity_compensate_temp= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "distance_compensate_temp")
            {
                GF120_set_param.distance_compensate_temp= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "compensate_temp")
            {
                GF120_set_param.compensate_temp= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "high_temp_warm_enable")
            {
                GF120_set_param.high_temp_warm_enable= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "high_temp_warm_temp")
            {
                GF120_set_param.high_temp_warm_temp= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "low_temp_warm_enable")
            {
                GF120_set_param.low_temp_warm_enable= node.toElement().text().toInt();
            }

            if(node.toElement().tagName() == "low_temp_warm_temp")
            {
                GF120_set_param.low_temp_warm_temp= node.toElement().text().toInt();
            }



            if(node.toElement().tagName() == "low_cover_file")
            {
                memcpy(&GF120_set_param.low_cover_file,node.toElement().text().toStdString().data(),node.toElement().text().size());
                printf("file name :%s \r\n",GF120_set_param.low_cover_file);
            }


            if(node.toElement().tagName() == "high_cover_file")
            {
                memcpy(&GF120_set_param.high_cover_file,node.toElement().text().toStdString().data(),node.toElement().text().size());
                printf("file name :%s \r\n",GF120_set_param.high_cover_file);
            }


            if(node.toElement().tagName() == "low_response_file")
            {
                memcpy(&GF120_set_param.low_response_file,node.toElement().text().toStdString().data(),node.toElement().text().size());
                printf("file name :%s \r\n",GF120_set_param.low_response_file);
            }

            if(node.toElement().tagName() == "high_response_file")
            {
                memcpy(&GF120_set_param.high_response_file,node.toElement().text().toStdString().data(),node.toElement().text().size());
                printf("file name :%s \r\n",GF120_set_param.high_response_file);
            }
            QDomNodeList list;
            if(node.toElement().tagName() == "GF120_roi")
            {
                list=node.toElement().childNodes();
                for(int i=0;i<list.count();i++)
                {
                    QDomNode n=list.at(i);
                    if(n.isElement())
                    {
                      qDebug()<<n.nodeName()<<":"<<n.toElement().text();
                      if(n.toElement().tagName() == "user_roi_enable")
                      {
                          GF120_set_param.GF120_roi.user_roi_enable = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_width_start")
                      {
                          GF120_set_param.GF120_roi.user_width_start = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_width_end")
                      {
                          GF120_set_param.GF120_roi.user_width_number = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_high_start")
                      {
                          GF120_set_param.GF120_roi.user_high_start = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_high_end")
                      {
                          GF120_set_param.GF120_roi.user_high_number = n.toElement().text().toInt();
                      }
                    }
                }
            }
            if(node.toElement().tagName() == "GF120_calibration")
            {
                list=node.toElement().childNodes();
                for(int i=0;i<list.count();i++)
                {
                    QDomNode n=list.at(i);
                    if(n.isElement())
                    {
                      qDebug()<<n.nodeName()<<":"<<n.toElement().text();
                      if(n.toElement().tagName() == "user_roi_enable")
                      {
                          GF120_set_param.GF120_calibration.user_calibration_enable = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_calibration_temp")
                      {
                          GF120_set_param.GF120_calibration.user_calibration_temp = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_width_start")
                      {
                          GF120_set_param.GF120_calibration.user_width_start = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_width_end")
                      {
                          GF120_set_param.GF120_calibration.user_width_end = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_high_start")
                      {
                          GF120_set_param.GF120_calibration.user_high_start = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_high_end")
                      {
                          GF120_set_param.GF120_calibration.user_high_end = n.toElement().text().toInt();
                      }
                    }
                }
            }
            if(node.toElement().tagName() == "GF120_color")
            {
                list=node.toElement().childNodes();
                for(int i=0;i<list.count();i++)
                {
                    QDomNode n=list.at(i);
                    if(n.isElement())
                    {
                      qDebug()<<n.nodeName()<<":"<<n.toElement().text();
                      if(n.toElement().tagName() == "user_color_enable")
                      {
                          GF120_set_param.GF120_color.user_color_enable = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_color_low")
                      {
                          GF120_set_param.GF120_color.user_color_low = n.toElement().text().toInt();
                      }
                      if(n.toElement().tagName() == "user_color_high")
                      {
                          GF120_set_param.GF120_color.user_color_high = n.toElement().text().toInt();
                      }
                    }
                }
            }
        }
        //下一个兄弟节点
        node=node.nextSibling();
    }

    fileSave.close();
    gui_init(gf120);
}

void MainWindow::gui_init(std::shared_ptr<GF120> gf120)
{
    auto& GF120_set_param = gf120->GF120_set_param;
    auto& GF120_roi_temp = gf120->GF120_roi_temp;

    ui->comboBox_infrared_thermometry->setCurrentIndex(GF120_set_param.temp_mode);
    ui->comboBox_infrared_color->setCurrentIndex(GF120_set_param.color_mode);
    ui->comboBox_display_mode->setCurrentIndex(1);
    printf("gui_init \r\n");

    ui->checkBox_infrared_osd->setChecked(GF120_set_param.osd_enable);
    ui->checkBox_infrared_temperature_display->setChecked(GF120_set_param.temp_display_enable);

    ui->checkBox_infrared_roi->setChecked(GF120_roi_temp[0].user_roi_enable);

    ui->spinBox_infrared_width_start->setValue(GF120_roi_temp[0].user_width_start);
    ui->spinBox_infrared_width_end ->setValue(GF120_roi_temp[0].user_width_number);
    ui->spinBox_infrared_high_start->setValue(GF120_roi_temp[0].user_high_start);
    ui->spinBox_infrared_high_end->setValue(GF120_roi_temp[0].user_high_number);

    ui->lineEdit_sample_path->setText(GF120_set_param.file_path);
    ui->lineEdit_path_cover_low->setText(GF120_set_param.low_cover_file);
    ui->lineEdit_path_cover_high->setText(GF120_set_param.high_cover_file);
    ui->lineEdit_infrared_path_to_low_response_rate_sample->setText(GF120_set_param.low_response_file);
    ui->lineEdit_infrared_path_to_high_response_rate_sample->setText(GF120_set_param.high_response_file);

    ui->checkBox_infrared_blackbody_calibrate->setChecked(GF120_set_param.GF120_calibration.user_calibration_enable);

    ui->spinBox_infrared_blackbody_width_start->setValue(GF120_set_param.GF120_calibration.user_width_start);
    ui->spinBox_infrared_blackbody_width_end->setValue(GF120_set_param.GF120_calibration.user_width_end);
    ui->spinBox_infrared_blackbody_high_start->setValue(GF120_set_param.GF120_calibration.user_high_start);
    ui->spinBox_infrared_blackbody_high_end->setValue(GF120_set_param.GF120_calibration.user_high_end);

    ui->checkBox_infrared_color_map->setChecked(GF120_set_param.GF120_color.user_color_enable);
    ui->spinBox_infrared_fake_color_low->setValue(GF120_set_param.GF120_color.user_color_low/100);
    ui->spinBox_infrared_fake_color_high->setValue(GF120_set_param.GF120_color.user_color_high/100);

    ui->checkBox_infrared_high_warm->setChecked(GF120_set_param.high_temp_warm_enable);
    ui->doubleSpinBox_infrared_high_warm->setValue((double)GF120_set_param.high_temp_warm_temp/100);
    ui->checkBox_infrared_low_warm->setChecked(GF120_set_param.low_temp_warm_enable);
    ui->doubleSpinBox_infrared_low_warm->setValue((double)GF120_set_param.low_temp_warm_temp/100);
    ui->doubleSpinBox_infrared_temperature_compensation->setValue((double)GF120_set_param.user_compensate_temp/100);
    ui->doubleSpinBox_infrared_distance_compensation->setValue((double)GF120_set_param.distance_compensate_temp);
    ui->doubleSpinBox_infrared_humidity_compensation->setValue((double)GF120_set_param.humidity_compensate_temp/100);
}


void MainWindow::on_pushButton_change_clicked()
{
    static int temp_value[0x10000];
    static double a;
    static double b;
    static double c;
    static double d;
    static double e;
    static double f;
    static double g;

    char*  filename;
    int size,len;
    char file_char[512];
   QFileDialog *fileDialog = new QFileDialog(this);
   fileDialog->setWindowTitle(tr("Select file"));
   fileDialog->setNameFilter(tr("config Files(sample_coefficient_mode*.txt )"));
   if(fileDialog->exec() == QDialog::Accepted) {
       QString path = fileDialog->selectedFiles()[0];
       //QMessageBox::information(NULL, tr("Path"), tr("You selected ") + path);


       QByteArray tmp = path.toLatin1();
       filename=tmp.data();
       printf("input file name :%s \r\n",filename);

   }
   FILE *corre_fp;
   if((corre_fp=fopen(filename,"rb"))==NULL) //读取1.txt的文件
   {
       printf("open filed %s\r\n ",filename);
       return ;
   }
   else
   {
       printf("corre_fp :%d \n",corre_fp);
   }
   fseek(corre_fp, 0L, SEEK_END);
   size = ftell(corre_fp);//此处返回一个很大的值
   fseek(corre_fp, 0L, SEEK_SET);
   memset(file_char,0x0,sizeof(file_char));
   len = fread(&file_char, size, 1, corre_fp);
   if(len <= 0)
   {
       fclose(corre_fp);
       return;
   }
   printf("%s \r\n",file_char);
   sscanf(file_char,"a = %lf b = %lf c = %lf d = %lf e = %lf f = %lf g = %lf ",&a,&b,&c,&d,&e,&f,&g);
   fclose(corre_fp);
   printf("a:%.32lf,b:%.32lf,c:%.32lf,d:%.32lf,e:%.32lf,f:%.32lf,g:%.32lf",a,b,c,d,e,f,g);

   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   fileDialog->setWindowTitle(tr("Select file"));
   fileDialog->setNameFilter(tr("config Files(*sample_data_mode*.bin )"));
   if(fileDialog->exec() == QDialog::Accepted) {
       QString path = fileDialog->selectedFiles()[0];
       //QMessageBox::information(NULL, tr("Path"), tr("You selected ") + path);


       QByteArray tmp = path.toLatin1();
       filename=tmp.data();
       printf("output file name :%s \r\n",filename);

   }
   if((corre_fp=fopen(filename,"wb+"))==NULL) //读取1.txt的文件
   {
       printf("open filed %s\r\n ",filename);
       return ;
   }
   else
   {
       printf("corre_fp :%d \n",corre_fp);
   }
   int i;
   double tmp;
   double temp = 0;
   for(i = 0 ; i < 0x10000; i ++)
   {
       tmp = i;
       temp = 0;
       temp += (tmp*tmp*tmp*tmp*tmp*tmp)*g;
       temp += (tmp*tmp*tmp*tmp*tmp)*f;
       temp += (tmp*tmp*tmp*tmp)*e;
       temp += (tmp*tmp*tmp)*d;
       temp += (tmp*tmp)*c;
       temp += (tmp)*b;
       temp += a;
       temp_value[i] = (int)temp;

   }
   fwrite(&temp_value, sizeof(temp_value), 1, corre_fp); //将采集的温度写入数据开始
   fflush(corre_fp);
   fclose(corre_fp);
   printf("value: %d %d %d %d \r\n ",temp_value[19223],temp_value[24113],temp_value[38866],temp_value[56256]);
   printf("%d %d %d %d \r\n",temp_value[19223],temp_value[20693],temp_value[22360],temp_value[24113]);
   delete fileDialog;
}


void MainWindow::on_pushButton_saveToDevice_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    QString srcDirPath = QFileDialog::getExistingDirectory(
                   this, "choose src Directory",
                    "/");
    if (srcDirPath.isEmpty())
    {
        return;
    }

    QDir dir(srcDirPath);
    if (!dir.exists())
    {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请选择正确的目录"));
        return;
    }

    dir.setFilter(QDir::Files);
    QStringList filter;
    QList<QFileInfo> fileInfo(dir.entryInfoList(filter));
    if (fileInfo.count() < 1)
    {
        QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("目录不能为空"));
        return;
    }

    QString strText = QString::fromLocal8Bit("将写入如下文件 %1:\n").arg(srcDirPath);
    QVector<QByteArray> FileNames;

    FileNames.append(srcDirPath.toLatin1());
    for (int i = 0; i < fileInfo.count(); ++i)
    {
        QString fileName = fileInfo.at(i).fileName();
        strText += fileName + "\n";
        FileNames.append(fileName.toLatin1());
    }
    strText += QString::fromLocal8Bit("\n写入新的校正文件会覆盖相机内原有内容，确认要覆盖吗？");
     strText += QString::fromLocal8Bit("\n写入新的校正文件大约需要35s,请等待,60s后没有响应关闭程序！");
    QMessageBox::StandardButton selected = QMessageBox::warning(this, "warning", strText, QMessageBox::Yes | QMessageBox::No);
    if (selected == QMessageBox::Yes)
    {
        vector<string> FileNamePtrs;
        for (int i = 0; i < FileNames.count(); ++i)
        {
            FileNamePtrs.push_back(FileNames[i].data());
        }

        auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
        auto status = gf120->save_check_config(FileNamePtrs);

        if (status == 0)
        {
            QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("保存成功"));
        }
        else
        {
            QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("保存失败:%1").arg(status));
        }
    }
}


void MainWindow::on_pushButton_deleteFilesInDevice_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    QString strText = "确认要删除相机内的校正文件吗？删除后测试温度误差会增大";
    QMessageBox::StandardButton selected = QMessageBox::warning(this, "warning", strText, QMessageBox::Yes | QMessageBox::No);
    if (selected == QMessageBox::Yes)
    {
        auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
        auto status = gf120->delete_check_config();

        if (status == 0)
        {
            QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("保存成功"));
        }
        else
        {
            QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("保存失败:%1").arg(status));
        }
    }
}


void MainWindow::on_pushButton_up_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    char*  filename;
    int size,len;
    char file_char[512];
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle(tr("Select file"));
    fileDialog->setNameFilter(tr("*.* )"));
    if(fileDialog->exec() != QDialog::Accepted)
    {
        return;
    }

    QString path = fileDialog->selectedFiles()[0];
    //QMessageBox::information(NULL, tr("Path"), tr("You selected ") + path);
    QByteArray tmp = path.toLatin1();
    filename=tmp.data();
    printf("input file name :%s \r\n",filename);
    ui->lineEdit_updateFile->setText(path);

    QFile tmpfile(path);
    tmpfile.open(QIODevice::ReadOnly);
    QDataStream in(&tmpfile);

    int fileSize = tmpfile.size();
    int nn = 0;
    int nCount = 0;
    int xxx=0;
    char cmd_buff[512];
    char result[64];
    memset(cmd_buff,0x0,sizeof(cmd_buff));
    memset(result,0x0,sizeof(result));
    typedef unsigned char BYTE;
    BYTE temp = 0;
    BYTE tempT[100] = {0};
    QString file_path = tmpfile.fileName();
    QString file_name = file_path.section("/",-1,-1);
    printf("%s\n",file_name.toStdString().data());
    sprintf(cmd_buff,"updatefile_start(%s )",file_name.toStdString().data() );//bbbbb
    printf("cmd_buff :%s \r\n",cmd_buff);


    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    xxx = gf120->command(cmd_buff);

    //while (!in.atEnd())
    while ( (nCount = in.readRawData((char*)tempT, 100)) !=0 )
    {
        //in >> temp;
//        memset(tempT,0,100);
        //in.readBytes(rbytes, len);
//        in.readRawData((char*)tempT, 100);
        //nn++;
        nn = nn + nCount;

        //sprintf(cmd_buff,"updatefile %d",temp);
        sprintf(cmd_buff,"data(%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "

                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d %d %d %d %d %d "
                ")"
                ,tempT[0],tempT[1],tempT[2],tempT[3],tempT[4],tempT[5],tempT[6],tempT[7],tempT[8],tempT[9]
                ,tempT[10],tempT[11],tempT[12],tempT[13],tempT[14],tempT[15],tempT[16],tempT[17],tempT[18],tempT[19]
                ,tempT[20],tempT[21],tempT[22],tempT[23],tempT[24],tempT[25],tempT[26],tempT[27],tempT[28],tempT[29]
                ,tempT[30],tempT[31],tempT[32],tempT[33],tempT[34],tempT[35],tempT[36],tempT[37],tempT[38],tempT[39]
                ,tempT[40],tempT[41],tempT[42],tempT[43],tempT[44],tempT[45],tempT[46],tempT[47],tempT[48],tempT[49]
                ,tempT[50],tempT[51],tempT[52],tempT[53],tempT[54],tempT[55],tempT[56],tempT[57],tempT[58],tempT[59]
                ,tempT[60],tempT[61],tempT[62],tempT[63],tempT[64],tempT[65],tempT[66],tempT[67],tempT[68],tempT[69]
                ,tempT[70],tempT[71],tempT[72],tempT[73],tempT[74],tempT[75],tempT[76],tempT[77],tempT[78],tempT[79]
                ,tempT[80],tempT[81],tempT[82],tempT[83],tempT[84],tempT[85],tempT[86],tempT[87],tempT[88],tempT[89]
                ,tempT[90],tempT[91],tempT[92],tempT[93],tempT[94],tempT[95],tempT[96],tempT[97],tempT[98],tempT[99]
                );

        xxx = gf120->command(cmd_buff);
        //printf("xxx=%d cmd_buff=%s result:%s \r\n",xxx, cmd_buff, result);
        if(xxx != 0 )
        {
    //        break;
        }
        memset(tempT,0,100);
        if(nn%100 == 0 )
        {
            //printf("xxx=%d cmd_buff=%s result:%s \r\n",xxx, cmd_buff, result);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            //m_camera_updateStatus->setText(QString("process:%1 / %2 ").arg(QString::number(nn*100 )).arg(QString::number(fileSize )) );
            double bfb = nn*1.0 / fileSize;
        //    m_camera_updateStatus->setText(QString("UploadFile:%1 %").arg( QString::number( bfb*100.0, 'f', 2 ) ) );
        }

    }
    sprintf(cmd_buff,"updatefile_end(%s )",file_name.toStdString().data() );//写完成关闭文件
    printf("cmd_buff :%s \r\n",cmd_buff);
    xxx = gf120->command(cmd_buff);

    temp = 1;
    sprintf(cmd_buff,"updatefile data.rbf",temp);
    //m_camera_updateStatus->setText(QString("process:%1 %").arg( QString::number( 100.0, 'f', 2 ) ) );

//    double c = 0;
//    int d = 0;
//    tmpfile.read((char*)&c, sizeof(c));
//    tmpfile.read((char*)&d, sizeof(d));
//    //cout<<c<<' '<<d<<endl;
//    //QByteArray bytes = tmpfile.readAll();
//    //c = *((double*)bytes.data());
//    //d = *((int*)(bytes.data() + sizeof(c)));
//    //cout<<c<<' '<<d<<endl;


    printf("nn=%d", nn);
}


void MainWindow::on_pushButton_update_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    char cmd_buff[512];
    char result[64];
    memset(cmd_buff,0x0,sizeof(cmd_buff));
    memset(result,0x0,sizeof(result));
    sprintf(cmd_buff,"update_fpga(gev300_gf120_mipi.rbf )");//写完成关闭文件
    printf("cmd_buff :%s \r\n",cmd_buff);
    auto gf120 =std::dynamic_pointer_cast<GF120>(deviceItem->camera);
    gf120->command(cmd_buff);
}

