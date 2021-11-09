#include "mainmenu.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_devicetreewidgetitem.h"

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

DeviceTreeWidgetItem::DeviceTreeWidgetItem(QWidget *parent,QTreeWidgetItem *item,QString series) :
    QWidget(parent),
    ui(new Ui::DeviceTreeWidgetItem),
    item(item),
    series(series)
{
    ui->setupUi(this);

}

DeviceTreeWidgetItem::~DeviceTreeWidgetItem()
{
    delete ui;
}

void DeviceTreeWidgetItem::on_toolButton_refresh_clicked()
{
    QProcess cmd;
    cmd.start("/home/sl.truman/Desktop/build-mind-vision-Desktop-Debug/mind-vision",{"list"});
    cmd.waitForFinished();

    while(item->childCount()) item->removeChild(item->child(0));
    auto s = cmd.readAllStandardOutput().split('\n'); s.removeLast();

    for(QString line : s) {
        auto info = line.split(' '); //产品系列 产品名称 产品昵称 内核符号连接名 内部使用 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
        if(info[0] != series) continue;

        auto cameraName = info[0] == "GIGE" ? QString("%1(%2)").arg(info[1]).arg(info[10]) : info[2];
        auto child = new QTreeWidgetItem(item,{cameraName});
        child->setData(0,Qt::UserRole,info);
        child->setData(0,Qt::ToolTipRole,cameraName);
        child->setIcon(0,QIcon(":/切图-首页/录像.png"));
    }
}

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

    ui->treeWidget_devices->setItemWidget(gige,0,new DeviceTreeWidgetItem(this,gige,"GIGE"));
    ui->treeWidget_devices->setItemWidget(usb,0,new DeviceTreeWidgetItem(this,usb,"U3V"));
    ui->tabWidget->tabBar()->hide();

    connect(&cameraStatusUpdate,SIGNAL(timeout()),SLOT(at_cameraStatusUpdate_timeout()));
    cameraStatusUpdate.setInterval(1000);
    cameraStatusUpdate.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event){
    ui->tabWidget->setCurrentIndex(0);
    for(auto view : cameraViews) {
        view->stop();
        view->camera->write("exit\n");
        delete view;
    }
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

void MainWindow::on_treeWidget_devices_customContextMenuRequested(const QPoint &pos)
{
    if(!selectedCameraItem || selectedCameraItem->data(0,Qt::UserRole).isNull()) return;

    auto qMenu = new QMenu(ui->treeWidget_devices);
    qMenu->setObjectName("menu_devices");
    qMenu->addAction(ui->action_preview);
    qMenu->addAction(ui->action_modifyIp);
    qMenu->addAction(ui->action_topOrNot);
    qMenu->addAction(ui->action_rename);
    qMenu->exec(QCursor::pos()); //在鼠标点击的位置显示鼠标右键菜单
}

void MainWindow::on_pushButton_magnify_clicked()
{
    if(!selectedCameraItem || selectedCameraItem->data(0,Qt::UserRole).isNull()) return;

    auto cameraName = selectedCameraItem->data(0,Qt::UserRole).toStringList()[2];
    if(!cameraViews.contains(cameraName)) return;
    cameraViews[cameraName]->currentScale += 0.1;
}

void MainWindow::on_pushButton_shrink_clicked()
{
if(!selectedCameraItem || selectedCameraItem->data(0,Qt::UserRole).isNull()) return;

    auto cameraName = selectedCameraItem->data(0,Qt::UserRole).toStringList()[2];
    if(!cameraViews.contains(cameraName)) return;
    cameraViews[cameraName]->currentScale -= 0.1;
}

void MainWindow::on_pushButton_perfect_clicked()
{
    if(!selectedCameraItem || selectedCameraItem->data(0,Qt::UserRole).isNull()) return;

    auto cameraName = selectedCameraItem->data(0,Qt::UserRole).toStringList()[2];
    if(!cameraViews.contains(cameraName)) return;
    cameraViews[cameraName]->currentScale = 0.99;
}

void MainWindow::on_pushButton_take_clicked()
{
    if(!selectedCameraItem || selectedCameraItem->data(0,Qt::UserRole).isNull()) return;
    auto cameraName = selectedCameraItem->data(0,Qt::UserRole).toStringList()[2];
    if(!cameraViews.contains(cameraName)) return;

    auto imageDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    cout << imageDir.toStdString() << endl;
    auto time = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz");
    auto filePath = imageDir + "/mind-vision " + time + ".bmp";
    cameraViews[cameraName]->background->pixmap().save(filePath);
    QMessageBox::information(NULL, "抓拍", "图片已保存：" + filePath, QMessageBox::Ok);
}

void MainWindow::on_pushButton_record_clicked()
{

}

void MainWindow::on_pushButton_exposure_clicked()
{

}

void MainWindow::on_pushButton_whiteBalance_clicked()
{

}

void MainWindow::on_pushButton_layout_clicked()
{
    auto i = ui->tabWidget->currentIndex() + 1;
    auto c = ui->tabWidget->count();

    ui->tabWidget->setCurrentIndex(i == c ? 1 : i);
}

void MainWindow::on_pushButton_customStatus_clicked()
{

}

void MainWindow::on_action_preview_triggered()
{
    emit ui->pushButton_playOrStop->click();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    for(auto o : ui->tab_main->findChildren<CameraView*>())
        o->setParent(nullptr);

    for(auto o : ui->tab_all->findChildren<CameraView*>())
        o->setParent(nullptr);

    switch(index) {
    case 0:
        break;
    case 1:{
        auto item = ui->treeWidget_devices->currentItem();
        if (!item) break;

        auto cameraName = item->data(0,Qt::UserRole).toStringList()[2];

        auto it = cameraViews.find(cameraName);
        if(it != cameraViews.end()) {
            ui->tab_main_contents->addWidget(*it,0,0);
        }
        break;
    }
    case 2:
        for(auto view : cameraViews) {
            auto cameraName = view->camera->arguments()[1];

            auto i=0,j=ui->tab_all_contents->columnCount();
            if(ui->tab_all_contents->count() < j*j)
                for(i=0;i< ui->tab_all_contents->columnCount();i++) {
                    ui->tab_all_contents->setRowStretch(i,1);
                    for (j=0;j < ui->tab_all_contents->columnCount();j++) {
                        ui->tab_all_contents->setColumnStretch(j,1);
                        auto item = ui->tab_all_contents->itemAtPosition(i,j);
                        if(!item) goto NEW_ITEM;
                    }
                }
NEW_ITEM:
            ui->tab_all_contents->addWidget(view,i,j);

        }
        break;
    default:
        break;
    }
}

void MainWindow::at_cameraStatusUpdate_timeout()
{
    for(auto i=0;i < 2;i++) {
        auto topItem = ui->treeWidget_devices->topLevelItem(i);
        for(auto j=0;j < topItem->childCount();j++){
            auto cameraName = topItem->child(j)->data(0,Qt::UserRole).toStringList()[2];
            auto it = cameraViews.find(cameraName);
            if (it == cameraViews.end()) {
                topItem->child(j)->setIcon(0,QIcon(":/切图-首页/录像.png"));
                continue;
            }

            if(it.value()->playing()) topItem->child(j)->setIcon(0,QIcon(":/切图-首页/绿.png"));
            else topItem->child(j)->setIcon(0,QIcon(":/切图-首页/红.png"));
        }
    }

    if(!selectedCameraItem) return;
    auto cameraName = selectedCameraItem->data(0,Qt::UserRole).toStringList()[2];
    if(!cameraViews.contains(cameraName)) return;
    auto resolution = cameraViews[cameraName]->background->pixmap().size();
    ui->label_resolution->setText(QString("%1x%2").arg(resolution.width()).arg(resolution.height()));

    if(!cameraViews.contains(cameraName)) return;
    ui->label_scale->setText(QString::number(cameraViews[cameraName]->currentScale * 100) + '%');
    ui->label_displayFPS->setText(QString::number(cameraViews[cameraName]->displayFPS));
    ui->label_frames->setText(QString::number(cameraViews[cameraName]->frames));

}


void MainWindow::on_pushButton_playOrStop_clicked(bool checked)
{
    if(!selectedCameraItem || selectedCameraItem->data(0,Qt::UserRole).isNull()) return;

    auto cameraName = selectedCameraItem->data(0,Qt::UserRole).toStringList()[2];

    if(!cameraViews.contains(cameraName)) {
        auto cmd = new QProcess(this);
        cmd->start("/home/sl.truman/Desktop/build-mind-vision-Desktop-Debug/mind-vision",{"open",cameraName});
        cmd->waitForReadyRead();
        auto res = cmd->readLine().split(' ');
        QString ret = res[0];
        if (ret == "failed") {
            QMessageBox::critical(NULL, "错误", "连接相机失败！", QMessageBox::Ok);
            return;
        }

        cameraViews.insert(cameraName,new CameraView(cmd));
    }

    if(checked) {
        cout << cameraName.toStdString() << " play" << endl;
        cameraViews[cameraName]->play(cameraName);
    } else {
        cout << cameraName.toStdString() << " stop" << endl;
        cameraViews[cameraName]->stop();
    }

    ui->tabWidget->setCurrentIndex(cameraViews.size() == 1 ? 1 : 2);
    emit ui->tabWidget->currentChanged(cameraViews.size() == 1 ? 1 : 2);
}


void MainWindow::on_pushButton_allCameras_clicked()
{

}



void MainWindow::on_treeWidget_devices_itemSelectionChanged()
{

}


void MainWindow::on_treeWidget_devices_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    selectedCameraItem = nullptr;
    if(current->data(0,Qt::UserRole).isNull()) return;
    selectedCameraItem = current;

    auto info = selectedCameraItem->data(0,Qt::UserRole).toStringList();
    ui->label_series_2->setText(info[0]);
    ui->label_deviceName_2->setText(info[1]);
    auto cameraName = info[2];
    ui->label_physicalAddress_2->setText(info[3]);
    ui->label_sensor_2->setText(info[6]);
    ui->label_ip_2->setText(info.size() > 10 ? info[10] : "");
    ui->label_mask_2->setText(info.size() > 11 ? info[11] : "");
    ui->label_gateway_2->setText(info.size() > 12 ? info[12] : "");
    ui->label_manufacturer_2->setText("");

    auto it = cameraViews.find(cameraName);
    if(cameraViews.end() != it && it.value()->playing())
        ui->pushButton_playOrStop->setChecked(true);
    else
        ui->pushButton_playOrStop->setChecked(false);

    ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex());
    emit ui->tabWidget->currentChanged(ui->tabWidget->currentIndex());
}

