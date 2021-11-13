#include "mainmenu.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_devicetreewidgetitem.h"
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
    ui->treeWidget_devices->setItemWidget(usb,0,new TopLevelItemWidget(usb,"U3V",this));

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

        emit ui->treeWidget_devices->itemSelectionChanged();
    }
    else {
        deviceItem->close();
        deviceItem->cameraView->setParent(nullptr);
    }
}

void MainWindow::on_treeWidget_devices_itemSelectionChanged()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(ui->treeWidget_devices->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto info = deviceItem->data(0,Qt::UserRole).toStringList();
    ui->label_series_2->setText(info[0]);
    ui->label_deviceName_2->setText(info[1]);
    auto cameraName = info[2];
    ui->label_physicalAddress_2->setText(info[3]);
    ui->label_sensor_2->setText(info[6]);
    ui->label_ip_2->setText(info.size() > 10 ? info[10] : "");
    ui->label_mask_2->setText(info.size() > 11 ? info[11] : "");
    ui->label_gateway_2->setText(info.size() > 12 ? info[12] : "");
    ui->label_manufacturer_2->setText("");

    auto exposureParams = deviceItem->exposureParams();
    ui->comboBox_exposureMode->setCurrentIndex(exposureParams[0].toUInt());
    ui->slider_brightness->setMinimum(exposureParams[1].toUInt());
    ui->slider_brightness->setMaximum(exposureParams[2].toUInt());
    ui->slider_brightness->setValue(exposureParams[3].toUInt());
    ui->checkBox_flicker->setChecked(exposureParams[4].toUInt());
    ui->comboBox_frequency->setCurrentIndex(exposureParams[5].toUInt());
    ui->slider_gain->setMinimum(exposureParams[6].toUInt());
    ui->slider_gain->setMaximum(exposureParams[7].toUInt());
    ui->slider_gain->setValue(exposureParams[8].toUInt());
    ui->slider_exposureTime->setMinimum(exposureParams[9].toUInt());
    ui->slider_exposureTime->setMaximum(exposureParams[10].toUInt());
    ui->slider_exposureTime->setValue(exposureParams[11].toUInt());

    auto whiteBalanceParams = deviceItem->whiteBalanceParams();
    ui->comboBox_whiteBalanceMode->setCurrentIndex(whiteBalanceParams[0].toUInt());
    ui->slider_r->setMinimum(whiteBalanceParams[1].toUInt());
    ui->slider_r->setMaximum(whiteBalanceParams[2].toUInt());
    ui->slider_r->setValue(whiteBalanceParams[3].toUInt());
    ui->slider_g->setMinimum(whiteBalanceParams[4].toUInt());
    ui->slider_g->setMaximum(whiteBalanceParams[5].toUInt());
    ui->slider_g->setValue(whiteBalanceParams[6].toUInt());
    ui->slider_b->setMinimum(whiteBalanceParams[7].toUInt());
    ui->slider_b->setMaximum(whiteBalanceParams[8].toUInt());
    ui->slider_b->setValue(whiteBalanceParams[9].toUInt());
    ui->slider_saturation->setMinimum(whiteBalanceParams[10].toUInt());
    ui->slider_saturation->setMaximum(whiteBalanceParams[11].toUInt());
    ui->slider_saturation->setValue(whiteBalanceParams[12].toUInt());
}

void MainWindow::on_comboBox_exposureMode_currentIndexChanged(int index)
{
    ui->slider_brightness->setEnabled(index);
    ui->checkBox_flicker->setEnabled(index);
    ui->comboBox_frequency->setEnabled(index);
    ui->slider_gain->setDisabled(index);
    ui->slider_exposureTime->setDisabled(index);
}

void MainWindow::on_slider_brightness_valueChanged(int value)
{
    cout << value << endl;
}

void MainWindow::on_checkBox_flicker_stateChanged(int arg1)
{

}

void MainWindow::on_slider_gain_valueChanged(int value)
{

}

void MainWindow::on_slider_exposureTime_valueChanged(int value)
{

}

void MainWindow::on_pushButton_onceWhiteBalance_clicked()
{

}

void MainWindow::on_comboBox_whiteBalanceMode_currentIndexChanged(int index)
{
    ui->slider_r->setDisabled(index);
    ui->slider_g->setDisabled(index);
    ui->slider_b->setDisabled(index);
    ui->slider_saturation->setDisabled(index);
}
