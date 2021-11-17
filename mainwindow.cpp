﻿#include "mainmenu.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "toplevelitemwidget.h"

#include <QProcess>
#include <QSpacerItem>
#include <QToolButton>
#include <QStringList>
#include <QRegExp>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDateTime>

#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , selectedCameraItem(nullptr)
{
    ui->setupUi(this);

    //setWindowFlag(Qt::FramelessWindowHint);
    this->setMenuWidget(new MainMenu(this));

    auto gige = ui->treeWidget_devices->topLevelItem(0);
    auto usb = ui->treeWidget_devices->topLevelItem(1);

    ui->treeWidget_devices->setItemWidget(gige,0,new TopLevelItemWidget(gige,"GIGE",this));
    ui->treeWidget_devices->setItemWidget(usb,0,new TopLevelItemWidget(usb,"U3V,Usb3Camera0",this));
    ui->widget_params->hide();
    ui->widget_status->hide();

    connect(&cameraStatusUpdate,SIGNAL(timeout()),SLOT(at_cameraStatusUpdate_timeout()),Qt::QueuedConnection);
    cameraStatusUpdate.setInterval(1000);
    cameraStatusUpdate.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event){
    ui->tabWidget_preview->setCurrentIndex(0);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        mMousePressed = true;
        mRelativeSrcPos = event->globalPos() - pos();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    mMousePressed = false;
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(mMousePressed) {
       move(event->globalPos() - mRelativeSrcPos);  //当前位置减去相对的原点位置得到的是移动距离
    }
}

void MainWindow::at_cameraStatusUpdate_timeout()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    if(deviceItem) {
        auto resolution = deviceItem->cameraView->background->pixmap().size();
        ui->label_resolution->setText(QString("%1x%2").arg(resolution.width()).arg(resolution.height()));
        ui->label_scale->setText(QString::number(deviceItem->cameraView->currentScale * 100) + '%');
        ui->label_displayFPS->setText(QString::number(deviceItem->cameraView->displayFPS));
        ui->label_frames->setText(QString::number(deviceItem->cameraView->frames));
        ui->pushButton_playOrStop->setChecked(deviceItem->cameraView->playing());
    }

    switch(ui->tabWidget_preview->currentIndex()) {
    case 0:
        break;
    case 1:
        if(deviceItem && -1 == ui->tab_main_contents->indexOf(deviceItem->cameraView)) {
            if(ui->tab_main_contents->count())
                ui->tab_main_contents->itemAt(0)->widget()->setParent(nullptr);

            ui->tab_main_contents->addWidget(deviceItem->cameraView);
        }
        break;
    case 2:
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
        break;
    default:
        break;
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
    qMenu->addAction(ui->action_modifyIp);
    qMenu->addAction(ui->action_topOrNot);
    qMenu->addAction(ui->action_rename);
    qMenu->exec(QCursor::pos()); //在鼠标点击的位置显示鼠标右键菜单
}

void MainWindow::on_action_open_triggered()
{
    emit ui->pushButton_playOrStop->clicked();
}

void MainWindow::on_pushButton_magnify_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->currentScale += 0.1;
}

void MainWindow::on_pushButton_shrink_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->currentScale -= 0.1;
}

void MainWindow::on_pushButton_perfect_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;
    deviceItem->cameraView->currentScale = 0.99;
}

void MainWindow::on_pushButton_take_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    auto imageDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    cout << imageDir.toStdString() << endl;
    auto time = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz");
    auto filePath = imageDir + "/mind-vision " + time + ".bmp";
    deviceItem->cameraView->background->pixmap().save(filePath);
    QMessageBox::information(this, "抓拍", "图片已保存：" + filePath, QMessageBox::Ok);
}

//void MainWindow::on_pushButton_record_clicked()
//{

//}

//void MainWindow::on_pushButton_customStatus_clicked()
//{

//}

void MainWindow::on_pushButton_playOrStop_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    if(QProcess::NotRunning == deviceItem->camera.state()) {
        if(!deviceItem->open()){
            QMessageBox::critical(this, "错误", "连接相机失败！", QMessageBox::Ok);
            return;
        }
    }
    else {
        deviceItem->close();
        deviceItem->cameraView->setParent(nullptr);
    }

    emit ui->treeWidget_devices->itemSelectionChanged();
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
    }
    else {
//        ui->tabWidget_preview->tabBar()->setTabVisible(0,true);
        if(ui->tabWidget_preview->currentIndex() != 0)
            ui->tabWidget_preview->setCurrentIndex(0);
    }

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    auto info = deviceItem->data(0,Qt::UserRole).toStringList();
    ui->label_series_2->setText(info[0]);
    ui->label_deviceName_2->setText(info[1]);
    auto cameraName = info[2];
    ui->label_physicalAddress_2->setText(info[3]);
    ui->label_sensor_2->setText(info[5]);
    ui->label_ip_2->setText(info.size() > 9 ? info[9] : "");
    ui->label_mask_2->setText(info.size() > 10 ? info[10] : "");
    ui->label_gateway_2->setText(info.size() > 11 ? info[11] : "");
    ui->label_manufacturer_2->setText("Mind Vision");

    if(QProcess::NotRunning == deviceItem->camera.state()) {
        ui->widget_params->hide();
        return;
    }

    ui->widget_params->show();

    emit this->on_MainWindow_cameraParamsUpdate();
}

void MainWindow::on_comboBox_exposureMode_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    ui->slider_brightness->setEnabled(index);
    ui->checkBox_flicker->setEnabled(index);
    ui->comboBox_frequency->setEnabled(index);
    ui->slider_gain->setDisabled(index);
    ui->slider_exposureTime->setDisabled(index);

    deviceItem->exposureMode(index);
}

void MainWindow::on_slider_brightness_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->brightness(value);
}

void MainWindow::on_checkBox_flicker_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

     deviceItem->flicker(arg1);
}

void MainWindow::on_slider_gain_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->gain(value);
}

void MainWindow::on_slider_exposureTime_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->exposureTime(value);
}

void MainWindow::on_comboBox_frequency_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->frequency(index);
}

void MainWindow::on_comboBox_whiteBalanceMode_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    ui->slider_r->setDisabled(index);
    ui->slider_g->setDisabled(index);
    ui->slider_b->setDisabled(index);
    ui->slider_saturation->setDisabled(index);

    deviceItem->whiteBalanceMode(index);
}

void MainWindow::on_pushButton_onceWhiteBalance_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->onceWhiteBalance();
    emit ui->treeWidget_devices->itemSelectionChanged();
}

void MainWindow::on_slider_r_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->r(value);
}


void MainWindow::on_slider_g_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->g(value);
}


void MainWindow::on_slider_b_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->b(value);
}


void MainWindow::on_slider_saturation_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->saturation(value);
}


void MainWindow::on_slider_gamma_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->gamma(value);
}

void MainWindow::on_slider_contrastRatio_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->contrastRatio(value);
}


void MainWindow::on_comboBox_resolution_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->resolution(index);
}

void MainWindow::on_checkBox_horizontalMirror_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->horizontalMirror(arg1);
}

void MainWindow::on_checkBox_verticalMirror_stateChanged(int arg1)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->verticalMirror(arg1);
}


void MainWindow::on_slider_acutance_valueChanged(int value)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->acutance(value);
}

void MainWindow::on_comboBox_triggerMode_currentIndexChanged(int index)
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

void MainWindow::on_comboBox_flashMode_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->flashMode(index);
}

void MainWindow::on_comboBox_flashPolarity_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->flashPolarity(index);
}

void MainWindow::on_MainWindow_cameraParamsUpdate()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    try {
        auto exposure = deviceItem->exposure();
        ui->comboBox_exposureMode->setCurrentIndex(exposure[0].toUInt());
        ui->slider_brightness->setMinimum(exposure[1].toUInt());
        ui->slider_brightness->setMaximum(exposure[2].toUInt());
        ui->slider_brightness->setValue(exposure[3].toUInt());
        ui->checkBox_flicker->setChecked(exposure[4].toUInt());
        ui->comboBox_frequency->setCurrentIndex(exposure[5].toUInt());
        ui->slider_gain->setMinimum(exposure[6].toUInt());
        ui->slider_gain->setMaximum(exposure[7].toUInt());
        ui->slider_gain->setValue(exposure[8].toUInt());
        ui->slider_exposureTime->setMinimum(exposure[9].toUInt());
        ui->slider_exposureTime->setMaximum(exposure[10].toUInt());
        ui->slider_exposureTime->setValue(exposure[11].toUInt());

        auto whiteBalance = deviceItem->whiteBalance();
        ui->comboBox_whiteBalanceMode->setCurrentIndex(whiteBalance[0].toUInt());
        ui->slider_r->setMinimum(whiteBalance[1].toUInt());
        ui->slider_r->setMaximum(whiteBalance[2].toUInt());
        ui->slider_r->setValue(whiteBalance[3].toUInt());
        ui->slider_g->setMinimum(whiteBalance[4].toUInt());
        ui->slider_g->setMaximum(whiteBalance[5].toUInt());
        ui->slider_g->setValue(whiteBalance[6].toUInt());
        ui->slider_b->setMinimum(whiteBalance[7].toUInt());
        ui->slider_b->setMaximum(whiteBalance[8].toUInt());
        ui->slider_b->setValue(whiteBalance[9].toUInt());
        ui->slider_saturation->setMinimum(whiteBalance[10].toUInt());
        ui->slider_saturation->setMaximum(whiteBalance[11].toUInt());
        ui->slider_saturation->setValue(whiteBalance[12].toUInt());

        auto lookupTables = deviceItem->lookupTables();
        ui->slider_gamma->setMinimum(lookupTables[0].toUInt());
        ui->slider_gamma->setMaximum(lookupTables[1].toUInt());
        ui->slider_gamma->setValue(lookupTables[2].toUInt());
        ui->slider_contrastRatio->setMinimum(lookupTables[3].toUInt());
        ui->slider_contrastRatio->setMaximum(lookupTables[4].toUInt());
        ui->slider_contrastRatio->setValue(lookupTables[5].toUInt());

        auto resolutions = deviceItem->resolutions();
        ui->comboBox_resolution->clear();
        ui->comboBox_resolution->addItems(resolutions);

        auto isp = deviceItem->isp();
        ui->checkBox_horizontalMirror->setChecked(isp[0].toUInt());
        ui->checkBox_verticalMirror->setChecked(isp[1].toUInt());
        ui->slider_acutance->setMinimum(isp[2].toUInt());
        ui->slider_acutance->setMaximum(isp[3].toUInt());
        ui->slider_acutance->setValue(isp[4].toUInt());

        auto controls= deviceItem->controls();
        ui->comboBox_triggerMode->setCurrentIndex(controls[0].toUInt());
        ui->comboBox_flashMode->setCurrentIndex(controls[1].toUInt());
        ui->comboBox_flashPolarity->setCurrentIndex(controls[2].toUInt());
    }catch(...) {
        cout << "Failed to sync camera's params!" << endl;
        emit on_MainWindow_cameraParamsUpdate();
    }
}

