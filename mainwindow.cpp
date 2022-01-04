#include "mainmenu.h"
#include "mainwindow.h"
#include "mainwindow_frameless.hpp"
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
#include <QColorDialog>

#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , selectedCameraItem(nullptr)
{
    ui->setupUi(this);
    MainWindow_FrameLess(parent);
    this->setMenuWidget(new MainMenu(this));

//    QLabel *label = new QLabel(tr("Devices"), ui->dockWidget_leftSide);
//    ui->dockWidget_leftSide->setTitleBarWidget(label);
    //ui->dockWidget_leftSide->titleBarWidget()->setStyleSheet("color: orange; font-size: 14pt; font-weight: bold;");

    auto gige = ui->treeWidget_devices->topLevelItem(0);
    auto usb = ui->treeWidget_devices->topLevelItem(1);

    ui->treeWidget_devices->setItemWidget(gige,0,new TopLevelItemWidget(gige,"GIGE,GiGeCamera",this));
    ui->treeWidget_devices->setItemWidget(usb,0,new TopLevelItemWidget(usb,"U3V,Usb3Camera0,Usb2Camera1",this));

    ui->treeWidget_devices->expandAll();

    ui->dockWidget_rightSide->hide();
    ui->widget_control->hide();
    ui->widget_status->hide();
    ui->tabWidget_preview->tabBar()->hide();

    connect(&cameraStatusUpdate,SIGNAL(timeout()),SLOT(at_cameraStatusUpdate_timeout()),Qt::QueuedConnection);
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

    if(deviceItem) {
        auto resolution = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->background->pixmap().size();
        ui->label_resolution->setText(QString("%1x%2").arg(resolution.width()).arg(resolution.height()));
        ui->label_zoom->setText(QString::number(deviceItem->cameraView->currentScale * 100) + '%');
        ui->label_displayFPS->setText(QString::number(deviceItem->cameraView->displayFPS));
        ui->label_frames->setText(QString::number(deviceItem->cameraView->frames));
        ui->label_coordinate->setText(deviceItem->cameraView->coordinate);
        ui->pushButton_playOrStop->setChecked(deviceItem->cameraView->playing);


        ui->pushButton_snapshot->setText(deviceItem->snapshotState() ? tr("Stop") : tr("Snapshot"));
        ui->pushButton_snapshot->setCheckable(deviceItem->snapshotState());
        ui->pushButton_record->setText(deviceItem->recordState() ? tr("Stop") : tr("Record"));
        ui->pushButton_record->setCheckable(deviceItem->recordState());
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
        if(!deviceItem->open()) {
            QMessageBox::critical(nullptr, tr("Device"), tr("Failed Connecting the camera!"), QMessageBox::Ok);
            return;
        }
    } else {
        deviceItem->close();
        deviceItem->cameraView->setParent(nullptr);
    }

    emit ui->treeWidget_devices->itemSelectionChanged();
}

void MainWindow::on_pushButton_zoomIn_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->currentScale *= 1.2;
}

void MainWindow::on_pushButton_zoomOut_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem) return;

    deviceItem->cameraView->currentScale *= 0.8;
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

    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) {
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
    ui->label_series_2->setText(info[0]);
    ui->label_deviceName_2->setText(info[1]);
    auto cameraName = info[2];
    ui->label_physicalAddress_2->setText(info[3]);
    ui->label_sensor_2->setText(info[5]);
    ui->label_ip_2->setText(info.size() > 9 ? info[9] : "");
    ui->label_mask_2->setText(info.size() > 10 ? info[10] : "");
    ui->label_gateway_2->setText(info.size() > 11 ? info[11] : "");
    ui->label_manufacturer_2->setText("Mind Vision");
}

void MainWindow::on_comboBox_exposureMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureMode(index);
}

void MainWindow::on_comboBox_exposureMode_currentIndexChanged(int index)
{
    ui->groupBox_automationExposure->setVisible(index);
    ui->groupBox_manualExposure->setHidden(index);
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

void MainWindow::on_comboBox_whiteBalanceMode_currentIndexChanged(int index)
{
    ui->slider_r->setDisabled(index);
    ui->slider_g->setDisabled(index);
    ui->slider_b->setDisabled(index);
    ui->slider_saturation->setDisabled(index);

    ui->spinBox_r->setReadOnly(index);
    ui->spinBox_g->setReadOnly(index);
    ui->spinBox_b->setReadOnly(index);
    ui->spinBox_saturation->setReadOnly(index);
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

void MainWindow::on_spinBox_acutance_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_acutance->value();
    deviceItem->acutance(v);
}

void MainWindow::on_slider_acutance_sliderMoved(int position)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    ui->spinBox_acutance->setValue(position);
    emit ui->spinBox_acutance->editingFinished();
}

void MainWindow::on_pushButton_resetParams_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->paramsReset();
    emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
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

void MainWindow::on_pushButton_saveParams_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;
    auto index = ui->comboBox_params->currentIndex();

    deviceItem->paramsSave(index);

//    auto filename = QFileDialog::getSaveFileName(this,tr("File name"),"",tr("config Files(*.config )"));
//    deviceItem->paramsSaveToFile(filename);
}

void MainWindow::on_comboBox_params_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->paramsLoad(index);
    emit ui->tabWidget_params->currentChanged(ui->tabWidget_params->currentIndex());
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

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    try {
        if(index == 0) {
            auto exposure = deviceItem->exposure();
            ui->comboBox_exposureMode->setCurrentIndex(exposure[0].toUInt());
            ui->slider_brightness->setMinimum(exposure[1].toUInt());
            ui->slider_brightness->setMaximum(exposure[2].toUInt());
            ui->slider_brightness->setValue(exposure[3].toUInt());

            ui->spinBox_brightness->setMinimum(exposure[1].toUInt());
            ui->spinBox_brightness->setMaximum(exposure[2].toUInt());
            ui->spinBox_brightness->setValue(exposure[3].toUInt());

            ui->checkBox_flicker->setChecked(exposure[4].toUInt());
            ui->comboBox_frequency->setCurrentIndex(exposure[5].toUInt());

            ui->slider_gain->setMinimum(exposure[6].toUInt());
            ui->slider_gain->setMaximum(exposure[7].toUInt());
            ui->slider_gain->setValue(exposure[8].toUInt());

            ui->spinBox_gain->setMinimum(exposure[6].toUInt());
            ui->spinBox_gain->setMaximum(exposure[7].toUInt());
            ui->spinBox_gain->setValue(exposure[8].toUInt());
            ui->spinBox_gainMinimum->setValue(exposure[6].toUInt());
            ui->spinBox_gainMaximum->setValue(exposure[7].toUInt());

            ui->slider_exposureTime->setMinimum(exposure[9].toUInt());
            ui->slider_exposureTime->setMaximum(exposure[10].toUInt());
            ui->slider_exposureTime->setValue(exposure[11].toUInt());

            ui->spinBox_exposureTime->setMinimum(exposure[9].toUInt());
            ui->spinBox_exposureTime->setMaximum(exposure[10].toUInt());
            ui->spinBox_exposureTime->setValue(exposure[11].toUInt());
            ui->spinBox_exposureTimeMinimum->setValue(exposure[9].toUInt());
            ui->spinBox_exposureTimeMaximum->setValue(exposure[10].toUInt());
        } else if(index == 1) {
            auto whiteBalance = deviceItem->whiteBalance();
            ui->comboBox_whiteBalanceMode->setCurrentIndex(whiteBalance[0].toUInt());
            ui->slider_r->setMinimum(whiteBalance[1].toUInt());
            ui->slider_r->setMaximum(whiteBalance[2].toUInt());
            ui->slider_r->setValue(whiteBalance[3].toUInt());
            ui->spinBox_r->setMinimum(whiteBalance[1].toUInt());
            ui->spinBox_r->setMaximum(whiteBalance[2].toUInt());
            ui->spinBox_r->setValue(whiteBalance[3].toUInt());

            ui->slider_g->setMinimum(whiteBalance[4].toUInt());
            ui->slider_g->setMaximum(whiteBalance[5].toUInt());
            ui->slider_g->setValue(whiteBalance[6].toUInt());
            ui->spinBox_g->setMinimum(whiteBalance[4].toUInt());
            ui->spinBox_g->setMaximum(whiteBalance[5].toUInt());
            ui->spinBox_g->setValue(whiteBalance[6].toUInt());

            ui->slider_b->setMinimum(whiteBalance[7].toUInt());
            ui->slider_b->setMaximum(whiteBalance[8].toUInt());
            ui->slider_b->setValue(whiteBalance[9].toUInt());
            ui->spinBox_b->setMinimum(whiteBalance[7].toUInt());
            ui->spinBox_b->setMaximum(whiteBalance[8].toUInt());
            ui->spinBox_b->setValue(whiteBalance[9].toUInt());

            ui->slider_saturation->setMinimum(whiteBalance[10].toUInt());
            ui->slider_saturation->setMaximum(whiteBalance[11].toUInt());
            ui->slider_saturation->setValue(whiteBalance[12].toUInt());
            ui->spinBox_saturation->setMinimum(whiteBalance[10].toUInt());
            ui->spinBox_saturation->setMaximum(whiteBalance[11].toUInt());
            ui->spinBox_saturation->setValue(whiteBalance[12].toUInt());

            ui->checkBox_monochrome->setChecked(whiteBalance[13].toUInt());
            ui->checkBox_inverse->setChecked(whiteBalance[14].toUInt());
            ui->comboBox_algorithm->setCurrentIndex(whiteBalance[15].toInt());
            ui->comboBox_colorTemrature->setCurrentIndex(whiteBalance[16].toInt());
            auto cs = dynamic_cast<CameraScene*>(deviceItem->cameraView->scene());
            cs->whiteBalanceWindowPos.setX(whiteBalance[17].toInt());
            cs->whiteBalanceWindowPos.setY(whiteBalance[18].toInt());
            cs->whiteBalanceWindowPos.setWidth(whiteBalance[19].toInt());
            cs->whiteBalanceWindowPos.setHeight(whiteBalance[20].toInt());
        } else if(index == 2) {
            auto lookupTableMode = deviceItem->lookupTableMode();
            ui->comboBox_lutMode->setCurrentIndex(lookupTableMode.toInt());
        } else if(index == 3) {
            auto isp = deviceItem->isp();
            ui->checkBox_horizontalMirror->setChecked(isp[0].toUInt());
            ui->checkBox_verticalMirror->setChecked(isp[1].toUInt());

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
            cs->deadPixelPos.clear();
            if(isp[11] != "None") {
                auto xArray = isp[11].split(',');
                auto yArray = isp[12].split(',');
                for(auto i=0;i < xArray.size();i++) cs->deadPixelPos.append(QPoint(xArray[i].toUInt(),yArray[i].toUInt()));
            }
            ui->checkBox_anamorphose->setChecked(isp[11].toUInt());
        } else if(index == 4) {
            auto video = deviceItem->video();
            ui->comboBox_frameRateSpeed->setCurrentIndex(video[0].toUInt());
            ui->spinBox_frameRateLimit->setValue(video[1].toInt());
        } else if(index == 5) {
            auto resolutionMode = deviceItem->resolutionMode();
            ui->comboBox_resolutionMode->setCurrentIndex(resolutionMode.toUInt());
        } else if(index == 6) {
            auto i=0,j=0;
            auto io = deviceItem->io();
            for(auto line : io) {
                auto columns = line.split(',');
                auto type = columns[0];
                auto mode = columns[1].toUInt();
                auto state = columns[2].toUInt();

                if(type == "Input"){
                    auto comboBox_ioMode = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_ioMode%1").arg(i));
                    comboBox_ioMode->setCurrentIndex(mode ? 1 : 0);
                    auto comboBox_ioState = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_ioState%1").arg(i));
                    comboBox_ioState->setCurrentIndex(state);
                    i++;
                } else {
                    auto comboBox_outputIoMode = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_outputIoMode%1").arg(j));
                    comboBox_outputIoMode->setCurrentIndex(mode == 1 ? 0 : 1);
                    auto comboBox_outputIoState = ui->tabWidget_params->findChild<QComboBox*>(QString("comboBox_outputIoState%1").arg(j));
                    comboBox_outputIoState->setCurrentIndex(state);
                    j++;
                }
            }
        } else if(index == 7) {
            auto controls= deviceItem->controls();
            ui->comboBox_triggerMode->setCurrentIndex(controls[0].toUInt());
            ui->spinBox_frameCount->setValue(controls[1].toUInt());
            ui->spinBox_delay->setValue(controls[2].toUInt());
            ui->spinBox_interval->setValue(controls[3].toUInt());
            ui->comboBox_outsideTriggerMode->setCurrentIndex(controls[4].toUInt());
            ui->spinBox_debounce->setValue(controls[5].toUInt());

            ui->comboBox_flashMode->setCurrentIndex(controls[6].toUInt());
            ui->comboBox_flashPolarity->setCurrentIndex(controls[7].toUInt());
            ui->spinBox_strobeDelay->setValue(controls[8].toUInt());
            ui->spinBox_strobePulse->setValue(controls[9].toUInt());
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

void MainWindow::on_comboBox_resolutionMode_currentIndexChanged(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());

    ui->groupBox_resolutionPreset->setHidden(index);
    ui->groupBox_resolutionCustom->setVisible(index);

    if(index == 0) {
        auto resolutions = deviceItem->resolutions();
        ui->comboBox_resolution->clear();
        ui->comboBox_resolution->addItems(resolutions);
        auto resolution = deviceItem->resolutionIndex();
        ui->comboBox_resolution->setCurrentIndex(resolution.toInt());
    } else {
        deviceItem->resolution(0);
        auto resolution = deviceItem->resolution();
        ui->spinBox_resolutionX->setValue(resolution[0].toInt());
        ui->spinBox_resolutionY->setValue(resolution[1].toInt());
        ui->spinBox_resolutionW->setValue(resolution[2].toInt());
        ui->spinBox_resolutionH->setValue(resolution[3].toInt());
    }

    dynamic_cast<CameraScene*>(deviceItem->cameraView->scene())->resolutionWindow = index;
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
    deviceItem->ioMode("Input",0,index ? 0 : 2);
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
    deviceItem->ioMode("Input",1,index ? 0 : 2);
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
    deviceItem->ioMode("Input",2,index ? 0 : 2);
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
    deviceItem->ioMode("Output",0,index ? 0 : 2);
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
    deviceItem->ioMode("Output",1,index ? 0 : 2);
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
    deviceItem->ioMode("Output",2,index ? 0 : 2);
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
    deviceItem->ioMode("Output",3,index ? 0 : 2);
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
    deviceItem->ioMode("Output",3,index ? 0 : 2);
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
        ui->label_flashPolarity->hide();
        ui->comboBox_flashPolarity->hide();
    } else {
        ui->label_flashPolarity->show();
        ui->comboBox_flashPolarity->show();
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

    deviceItem->rename(ui->lineEdit_nickname->text());
    QMessageBox::information(this,tr("Nickname"),tr("Apply ok!"));
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
    for(auto pos : cs->deadPixelPos) {
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
    ui->spinBox_gain->setValue(position);
    emit ui->spinBox_gain->editingFinished();
}

void MainWindow::on_spinBox_gain_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_gain->value();
    ui->slider_gain->setValue(v);
    deviceItem->gain(v);
}

void MainWindow::on_slider_exposureTime_sliderMoved(int position)
{
    ui->spinBox_exposureTime->setValue(position);
    emit ui->spinBox_exposureTime->editingFinished();
}

void MainWindow::on_spinBox_exposureTime_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_exposureTime->value();
    ui->slider_exposureTime->setValue(v);
    deviceItem->exposureTime(v);
}

void MainWindow::on_spinBox_gainMinimum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->gainRange(ui->spinBox_gainMinimum->value(),ui->spinBox_gainMaximum->value());
}

void MainWindow::on_spinBox_gainMaximum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->gainRange(ui->spinBox_gainMinimum->value(),ui->spinBox_gainMaximum->value());
}

void MainWindow::on_spinBox_exposureTimeMaximum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureTimeRange(ui->spinBox_exposureTimeMinimum->value(),ui->spinBox_exposureTimeMaximum->value());
}

void MainWindow::on_spinBox_exposureTimeMinimum_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->exposureTimeRange(ui->spinBox_gainMinimum->value(),ui->spinBox_gainMaximum->value());
}

void MainWindow::on_slider_r_sliderMoved(int position)
{
    ui->spinBox_r->setValue(position);
    emit ui->spinBox_r->editingFinished();
}

void MainWindow::on_spinBox_r_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_r->value();
    deviceItem->rgb(v,ui->slider_g->value(),ui->slider_b->value());
}

void MainWindow::on_slider_g_sliderMoved(int position)
{
    ui->spinBox_g->setValue(position);
    emit ui->spinBox_g->editingFinished();
}

void MainWindow::on_spinBox_g_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_g->value();
    deviceItem->rgb(ui->slider_r->value(),v,ui->slider_b->value());
}

void MainWindow::on_slider_b_sliderMoved(int position)
{
    ui->spinBox_b->setValue(position);
    emit ui->spinBox_b->editingFinished();
}

void MainWindow::on_spinBox_b_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v = ui->spinBox_b->value();
    deviceItem->rgb(ui->slider_r->value(),ui->slider_g->value(),v);
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
}

void MainWindow::on_comboBox_whiteBalanceMode_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    deviceItem->whiteBalanceMode(index);
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
        ui->slider_gamma->setValue(lookupTables[2].toUInt());
        ui->spinBox_gamma->setMinimum(lookupTables[0].toUInt());
        ui->spinBox_gamma->setMaximum(lookupTables[1].toUInt());
        ui->spinBox_gamma->setValue(lookupTables[2].toUInt());

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
    auto v = ui->spinBox_gamma->value();
    deviceItem->gamma(v);
}

void MainWindow::on_slider_gamma_valueChanged(int position)
{
    ui->spinBox_gamma->setValue(position);
    emit ui->spinBox_gamma->editingFinished();

    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
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

void MainWindow::on_spinBox_contrastRatio_editingFinished()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    auto v= ui->spinBox_contrastRatio->value();
    deviceItem->contrastRatio(v);
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
    if(nullptr == dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem())) {
        ui->tabWidget_preview->setCurrentIndex(0);
    } else {
        ui->tabWidget_preview->setCurrentIndex(2);
    }
}
