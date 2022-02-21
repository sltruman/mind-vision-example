#include "mainmenu.h"
#include "mainwindow.h"
#include "mainwindow_frameless.hpp"
#include "ui_mainwindow.h"
#include "toplevelitemwidget.h"
#include "rightsidetitlebar.h"
#include "loadingdialog.h"

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

    auto at_tabWidget_params_currentChanged = [=](){
        emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
    };

    connect(rightSide,&RightSideTitleBar::save_clicked,at_tabWidget_params_currentChanged);
    connect(rightSide,&RightSideTitleBar::default_clicked,at_tabWidget_params_currentChanged);
    connect(rightSide,&RightSideTitleBar::params_activated,at_tabWidget_params_currentChanged);

    auto gige = ui->treeWidget_devices->topLevelItem(0);
    auto usb = ui->treeWidget_devices->topLevelItem(1);

    ui->treeWidget_devices->setItemWidget(gige,0,new TopLevelItemWidget(gige,"GIGE,GiGeCamera",ui->treeWidget_devices));
    ui->treeWidget_devices->setItemWidget(usb,0,new TopLevelItemWidget(usb,"U3V,Usb3Camera0,Usb2Camera1",ui->treeWidget_devices));
    ui->treeWidget_devices->expandAll();
    ui->dockWidget_rightSide->hide();
    ui->widget_control->hide();
    ui->widget_status->hide();
    ui->tabWidget_preview->tabBar()->hide();

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

            if(QProcess::NotRunning == deviceItem->camera.state()) continue;
            deviceItem->close();
        }
    }
}

void MainWindow::at_cameraStatusUpdate_timeout()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    if(deviceItem && deviceItem->camera.state() == QProcess::Running) {
        if(deviceItem->cameraView->avgBrightness) {
            auto brightness = deviceItem->brightness();
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

        auto s = deviceItem->snapshotState();
        ui->pushButton_snapshot->setText(s ? tr("Stop") : tr("Snapshot"));
        ui->pushButton_snapshot->setCheckable(s);

        s = deviceItem->recordState();
        ui->pushButton_record->setText(s ? tr("Stop") : tr("Record"));
        ui->pushButton_record->setCheckable(s);

        auto portType = deviceItem->data(0,Qt::UserRole).toStringList()[6];

        try {
            auto status = deviceItem->status(portType);

            ui->label_frames->setText(status[0]);
            ui->label_recordFPS->setText(status[1]);

            if(-1 != portType.indexOf("NET")) {
                ui->label_temperature->setText(status[3]);
                ui->label_lost->setText(status[4]);
                ui->label_resend->setText(status[5]);
                ui->label_packSize->setText(status[6]);
            } else if(-1 != portType.indexOf("USB3")) {
                ui->label_sensorFps->setText(status[2]);
                ui->label_temperature->setText(status[3]);
                ui->label_lost->setText(status[4]);
                ui->label_resend->setText(status[5]);
                ui->label_recover->setText(status[6]);
                ui->label_linkSpeed->setText(status[7] == "3" ? tr("SuperSpeed") : status[7] == "2" ? tr("HighSpeed") : tr("FullSpeed"));
            } else {
                ui->label_linkSpeed->setText(status[7] == "3" ? tr("SuperSpeed") : status[7] == "2" ? tr("HighSpeed") : tr("FullSpeed"));
            }
        } catch(...) {}

        if(ui->radioButton_automationExposure->isChecked() && ui->tabWidget_params->currentIndex() == 0) {
            try {
                auto exposure = get<0>(deviceItem->exposure(false));
                ui->spinBox_gain->setValue(exposure[9].toInt() / 100.);
                ui->slider_gain->setValue(exposure[9].toInt());
                ui->spinBox_exposureTime->setValue(exposure[12].toDouble() / 1000.);
                ui->slider_exposureTime->setValue(exposure[12].toDouble());
            }catch(...) {

            }
        }
    }
}

void MainWindow::on_treeWidget_devices_customContextMenuRequested(const QPoint &pos)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;
    cout << "menu " << deviceItem->text(0).toStdString() << endl;

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

    if(QProcess::NotRunning == deviceItem->camera.state()) {
        connect(deviceItem->cameraView,&CameraView::focused,this,[=](){
            ui->treeWidget_devices->setCurrentItem(deviceItem);
        },Qt::UniqueConnection);

        connect(deviceItem->cameraView,&CameraView::doubleClick,this,[=](){
            ui->treeWidget_devices->setCurrentItem(deviceItem);
            ui->tabWidget_preview->setCurrentIndex(1);
        },Qt::UniqueConnection);

        cout << "open " << endl;
        if(!deviceItem->open()) {
            QMessageBox::critical(nullptr, tr("Device"), tr("Failed Connecting the camera!"), QMessageBox::Ok);
            return;
        }
    } else {
        cout << "close " << endl;
        deviceItem->close();
        cout << "closed " << endl;
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    if(ui->pushButton_snapshot->isChecked()) {
        cout << "snapshot-state" << endl;
        deviceItem->snapshotStop();
        return;
    }

    cout << "snapshot-start" << endl;
    auto mainMenu = dynamic_cast<MainMenu*>(this->menuWidget());
    deviceItem->snapshotStart(mainMenu->snapshotDialog.dir(),mainMenu->snapshotDialog.resolution(),mainMenu->snapshotDialog.format(),mainMenu->snapshotDialog.period());
    emit at_cameraStatusUpdate_timeout();
}

void MainWindow::on_pushButton_record_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    if(ui->pushButton_record->isChecked()) {
        cout << "record-state" << endl;
        deviceItem->recordStop();
        return;
    }

    auto mainMenu = dynamic_cast<MainMenu*>(this->menuWidget());
    deviceItem->recordStart(mainMenu->recordDialog.dir(),mainMenu->recordDialog.format(),mainMenu->recordDialog.quality(),mainMenu->recordDialog.frames());
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

            if(QProcess::Running == deviceItem->camera.state()) {
                availableDevices++;
            }
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
    } else if(QProcess::NotRunning == deviceItem->camera.state()) {
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
    auto info = deviceItem->data(0,Qt::UserRole).toStringList();
    ui->label_series_2->setText(info[0].left(4));
    ui->label_deviceName_2->setText(info[1]);
    auto cameraName = info[2];
    ui->label_sensor_2->setText(info[5]);
    ui->label_manufacturer_2->setText("Mind Vision");

    if(info.size() > 9) {
        ui->label_ip_2->show();
        ui->label_ip->show();
    } else {
        ui->label_ip_2->hide();
        ui->label_ip->hide();
    }

    if(info.size() > 10) {
        ui->label_mask_2->show();
        ui->label_mask->show();
    } else {
        ui->label_mask_2->hide();
        ui->label_mask->hide();
    }

    if(info.size() > 11) {
        ui->label_gateway_2->show();
        ui->label_gateway->show();
    } else {
        ui->label_gateway_2->hide();
        ui->label_gateway->hide();
    }

    ui->label_ip_2->setText(info.size() > 9 ? info[9] : "");
    ui->label_mask_2->setText(info.size() > 10 ? info[10] : "");
    ui->label_gateway_2->setText(info.size() > 11 ? info[11] : "");

    auto portType = info[6];
    ui->widget_lost->hide();
    ui->widget_resend->hide();
    ui->widget_recover->hide();
    ui->widget_packSize->hide();
    ui->widget_sfps->hide();
    ui->widget_temperature->hide();

    if(-1 != portType.indexOf("NET")) {
        ui->widget_lost->show();
        ui->widget_resend->show();
        ui->widget_packSize->show();
        ui->widget_temperature->show();
        if(-1 != portType.indexOf("10000M")) ui->label_linkSpeed->setText("10000M");
        if(-1 != portType.indexOf("1000M")) ui->label_linkSpeed->setText("1000M");
        if(-1 != portType.indexOf("100M")) ui->label_linkSpeed->setText("100M");
    } else if(-1 != portType.indexOf("USB3")) {
        ui->widget_lost->show();
        ui->widget_resend->show();
        ui->widget_recover->show();
        ui->widget_sfps->show();
        ui->widget_temperature->show();
    } else {    }

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

     deviceItem->flicker(arg1);
}

void MainWindow::on_comboBox_frequency_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->frequency(index);
}

void MainWindow::on_checkBox_horizontalMirrorSoft_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->horizontalMirror(0,arg1);
}

void MainWindow::on_checkBox_verticalMirrorSoft_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->verticalMirror(0,arg1);
}

void MainWindow::on_checkBox_horizontalMirrorHard_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->horizontalMirror(1,arg1);
}

void MainWindow::on_checkBox_verticalMirrorHard_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->verticalMirror(2,arg1);
}

void MainWindow::on_pushButton_onceWhiteBalance_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->onceWhiteBalance();

    emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
}

void MainWindow::on_comboBox_resolution_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->resolution(index);
}

void MainWindow::on_spinBox_acutance_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_acutance->value();
    deviceItem->acutance(v);
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto filename = QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("config Files(*.config )"));
    if(filename.isEmpty()) return;
    deviceItem->paramsLoadFromFile(filename);
    emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
}

void MainWindow::on_pushButton_saveParamsToFile_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto filename = QFileDialog::getSaveFileName(this,tr("File name"),"",tr("config Files(*.config )"));
    if(filename.isEmpty()) return;
    deviceItem->paramsSaveToFile(filename);
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->resolutionWindow = false;

    try {
        if(index == 0) {
            auto exposure = deviceItem->exposure();
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
        } else if(index == 2) {
            auto lookupTableMode = deviceItem->lookupTableMode();
            ui->comboBox_lutMode->setCurrentIndex(lookupTableMode.toInt());
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

            QFile f(isp[11]);
            f.open(QIODevice::ReadOnly | QIODevice::Text);
            auto xylist = QString(f.readAll()).split('\n');
            if(xylist.size()) xylist.removeLast();
            f.close();

            for(auto xystr : xylist) {
                auto xy = xystr.split(',');
                cs->existedPixels.append(QPoint(xy[0].toUInt(),xy[1].toUInt()));
            }

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
        }
    } catch(...) {
        cout << "Failed to sync camera's params!" << endl;
        emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
    }
}

void MainWindow::on_checkBox_monochrome_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->monochrome(checked);
}

void MainWindow::on_checkBox_inverse_clicked(bool checked)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->inverse(checked);
}

void MainWindow::on_comboBox_algorithm_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->algorithm(index);
}

void MainWindow::on_comboBox_colorTemrature_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->colorTemrature(index);
}

void MainWindow::on_comboBox_lutPreset_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->lookupTablePreset(index);
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->noise(checked);
}

void MainWindow::on_comboBox_noise3D_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    if(index) deviceItem->noise3d(1,index + 1);
    else deviceItem->noise3d(0,0);
}

void MainWindow::on_comboBox_rotate_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->rotate(index);
}

void MainWindow::on_comboBox_frameRateSpeed_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->frameRateSpeed(index);
}

void MainWindow::on_spinBox_frameRateLimit_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->frameRateLimit(ui->spinBox_frameRateLimit->value());
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
    deviceItem->resolution(0);

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindow = true;
}

void MainWindow::on_pushButton_resolution_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->resolutionWindow = false;

    auto rect = cs->resolutionWindowPos;
    deviceItem->resolution(rect.x(),rect.y(),rect.width(),rect.height());
}

void MainWindow::on_comboBox_ioMode0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Input",0,index == 0 ? 0 : 2);
}

void MainWindow::on_comboBox_ioState0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->ioState("Input",0,index);
}

void MainWindow::on_comboBox_ioMode1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Input",1,2);
}

void MainWindow::on_comboBox_ioState1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->ioState("Input",1,index);
}

void MainWindow::on_comboBox_ioMode2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Input",2,2);
}

void MainWindow::on_comboBox_ioState2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioState("Input",2,index);
}

void MainWindow::on_comboBox_outputIoMode0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Output",0,index == 0 ? 1 : 3);
}

void MainWindow::on_comboBox_outputIoState0_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioState("Output",0,index);
}

void MainWindow::on_comboBox_outputIoMode1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Output",1,3);
}

void MainWindow::on_comboBox_outputIoState1_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioState("Output",1,index);
}

void MainWindow::on_comboBox_outputIoMode2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Output",2,3);
}

void MainWindow::on_comboBox_outputIoState2_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioState("Output",2,index);
}

void MainWindow::on_comboBox_outputIoMode3_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Output",3,3);
}

void MainWindow::on_comboBox_outputIoState3_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioState("Output",3,index);
}

void MainWindow::on_comboBox_outputIoMode4_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioMode("Output",3,3);
}

void MainWindow::on_comboBox_outputIoState4_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->ioState("Output",3,index);
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->triggerMode(index);
}

void MainWindow::on_pushButton_softTrigger_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->onceSoftTrigger();
}

void MainWindow::on_spinBox_frameCount_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->triggerFrames(ui->spinBox_frameCount->value());
}

void MainWindow::on_spinBox_delay_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->triggerDelay(ui->spinBox_delay->value());
}

void MainWindow::on_spinBox_interval_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->triggerInterval(ui->spinBox_interval->value());
}

void MainWindow::on_comboBox_outsideTriggerMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->outsideTriggerMode(index);
}

void MainWindow::on_spinBox_debounce_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->outsideTriggerDebounce(ui->spinBox_debounce->value());
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    deviceItem->flashMode(index);
}

void MainWindow::on_comboBox_flashPolarity_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->flashPolarity(index);
}

void MainWindow::on_spinBox_strobeDelay_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->strobeDelay(ui->spinBox_strobeDelay->value());
}

void MainWindow::on_spinBox_strobePulse_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->strobePulse(ui->spinBox_strobePulse->value());
}

void MainWindow::on_checkBox_line1_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    if(-1 != ui->lineEdit_nickname->text().indexOf("V")
            || -1 != ui->lineEdit_nickname->text().indexOf(":")
            || -1 != ui->lineEdit_nickname->text().indexOf("*")
            || -1 != ui->lineEdit_nickname->text().indexOf("?")
            || -1 != ui->lineEdit_nickname->text().indexOf("\"")
            || -1 != ui->lineEdit_nickname->text().indexOf("<")
            || -1 != ui->lineEdit_nickname->text().indexOf(">")) {
        QMessageBox::information(this,tr("Nickname"),tr("Nickname cannot include:") + "V:*?\"<>|" );
    } else {
        deviceItem->rename(ui->lineEdit_nickname->text().trimmed());
        QMessageBox::information(this,tr("Nickname"),tr("Nickname has been modifyed and to restart the application!"));
    }
}

void MainWindow::on_checkBox_whiteBalanceWindow_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->whiteBalanceWindow = arg1;
}

void MainWindow::on_pushButton_setWhiteBalanceWindow_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto rect = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->whiteBalanceWindowPos;
    deviceItem->whiteBalanceWindow(rect.x(),rect.y(),rect.width(),rect.height());
}

void MainWindow::on_pushButton_defaultWhiteBalanceWindow_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->whiteBalanceWindowPos = cs->background->pixmap().rect();
}

void MainWindow::on_checkBox_deadPixelsWindow_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
    cs->deadPixelWindow = arg1;
}

void MainWindow::on_pushButton_saveDeadPixels_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());

    QString x,y;
    for(auto pos : cs->existedPixels + cs->manualPixels + cs->deadPixels + cs->brightPixels) {
        x += QString("%1,").arg(pos.x());
        y += QString("%1,").arg(pos.y());
    }

    if(!x.isEmpty()) x.remove(x.length()-1,1);
    deviceItem->deadPixels(x,y);

    QMessageBox::information(this, tr("Dead Pixels"), tr("Saved Dead Pixels!"), QMessageBox::Ok);
}

void MainWindow::on_checkBox_flatFiledCorrect_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->flatFieldCorrent(arg1);
}

void MainWindow::on_pushButton_darkField_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    try {
        deviceItem->flatFieldInit(0);
        QMessageBox::information(this, tr("Dark Field"), tr("Ok!"), QMessageBox::Ok);
    } catch(...) {
        QMessageBox::warning(nullptr, tr("Light Field"), tr("Light is not enough!"), QMessageBox::Ok);
    }
}

void MainWindow::on_pushButton_lightField_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    try {
        deviceItem->flatFieldInit(1);
        QMessageBox::information(this, tr("Light Field"), tr("Ok!"), QMessageBox::Ok);
    } catch(...) {
        QMessageBox::warning(nullptr, tr("Light Field"), tr("Light is not enough!"), QMessageBox::Ok);
    }
}

void MainWindow::on_pushButton_saveFlatFieldParams_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto filename = QFileDialog::getSaveFileName(this,tr("File name"),"",tr("config Files(*.ffc )"));
    if(filename.isEmpty()) return;
    deviceItem->flatFieldParamsSave(filename.replace('/','\\'));
}

void MainWindow::on_pushButton_loadFlatFieldParams_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto filename = QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("config Files(*.ffc )"));
    if(filename.isEmpty()) return;
    deviceItem->flatFieldParamsLoad(filename.replace('/','\\'));
}

void MainWindow::on_pushButton_calibration_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem->calibrationDialog.exec())
        return;

    deviceItem->undistortParams(deviceItem->calibrationDialog.width(),deviceItem->calibrationDialog.height(),deviceItem->calibrationDialog.cameraMatraix(),deviceItem->calibrationDialog.distortCoeffs());
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
    deviceItem->brightness(v);
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
    deviceItem->gain(v);
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
    deviceItem->exposureTime(v);
}

void MainWindow::on_spinBox_gainMinimum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->gainRange(ui->spinBox_gainMinimum->value() * 100.f,ui->spinBox_gainMaximum->value() * 100.f);
}

void MainWindow::on_spinBox_gainMaximum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->gainRange(ui->spinBox_gainMinimum->value() * 100.f,ui->spinBox_gainMaximum->value() * 100.f);
}

void MainWindow::on_spinBox_exposureTimeMaximum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureTimeRange(ui->spinBox_exposureTimeMinimum->value() * 1000.,ui->spinBox_exposureTimeMaximum->value() * 1000.);
}

void MainWindow::on_spinBox_exposureTimeMinimum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureTimeRange(ui->spinBox_exposureTimeMinimum->value() * 1000.,ui->spinBox_exposureTimeMaximum->value() * 1000.);
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
    deviceItem->rgb(v,ui->slider_g->value(),ui->slider_b->value());
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
    deviceItem->rgb(ui->slider_r->value(),v,ui->slider_b->value());
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
    deviceItem->rgb(ui->slider_r->value(),ui->slider_g->value(),v);
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
    deviceItem->saturation(v);
    ui->slider_saturation->setValue(v);
}

void MainWindow::on_comboBox_lutMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->lookupTableMode(index);
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
    deviceItem->gamma(v);
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
    deviceItem->contrastRatio(v);
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
    deviceItem->undistort(checked);
}

void MainWindow::on_pushButton_layout_clicked()
{
    ui->tabWidget_preview->setCurrentIndex(ui->tabWidget_preview->currentIndex() % 2 + 1);
}

void MainWindow::on_spinBox_threshold_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->threshold(ui->spinBox_threshold->value());
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
    deviceItem->rawOutputRange(position);

    if(position < 0) {
        ui->label_outputRange->setText(tr("Full Output"));
    } else {
        ui->label_outputRange->setText(QString("%1~%2").arg(position+1).arg(position+8) + tr("Output"));
    }
}

void MainWindow::on_comboBox_outputFormats_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->rawOutputFormat(index);
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
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->exposureWindow = arg1;
}

void MainWindow::on_pushButton_setExposureWindow_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto rect = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->exposureWindowPos;
    deviceItem->exposureWindow(rect.x(),rect.y(),rect.width(),rect.height());
}

void MainWindow::on_radioButton_manualExposure_toggled(bool checked)
{
    ui->groupBox_manualExposure->setEnabled(checked);
    ui->checkBox_exposureWIndow->setChecked(false);
}

void MainWindow::on_radioButton_manualExposure_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureMode(0);
}

void MainWindow::on_radioButton_automationExposure_toggled(bool checked)
{
    ui->groupBox_automationExposure->setEnabled(checked);
    ui->checkBox_exposureWIndow->setChecked(false);
}

void MainWindow::on_radioButton_automationExposure_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureMode(1);
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

    deviceItem->resolution(0);
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
    deviceItem->whiteBalanceMode(0);
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
    deviceItem->whiteBalanceMode(1);
}

void MainWindow::on_tabWidget_preview_currentChanged(int index)
{
    auto f = [=]() {
        for(auto topItemIndex=0;topItemIndex<ui->treeWidget_devices->topLevelItemCount();topItemIndex++) {
            auto topLevelItem = ui->treeWidget_devices->topLevelItem(topItemIndex);

            for(auto subItemIndex=0;subItemIndex< topLevelItem->childCount();subItemIndex++) {
                auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(subItemIndex));

                if(QProcess::NotRunning == deviceItem->camera.state()) continue;

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
