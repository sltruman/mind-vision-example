﻿#include "toplevelitemwidget.h"
#include "ui_toplevelitemwidget.h"

TopLevelItemWidget::TopLevelItemWidget(QTreeWidgetItem *item,QString series,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TopLevelItemWidget),
    topLevelItem(item),
    series(series)
{
    ui->setupUi(this);
    emit ui->toolButton_refresh->clicked();

    connect(&t,SIGNAL(timeout()),SLOT(statusUpdate()));
    t.setInterval(1000);
    t.start();
}

TopLevelItemWidget::~TopLevelItemWidget()
{
    delete ui;
}

void TopLevelItemWidget::on_toolButton_refresh_clicked()
{
    QProcess cmd;
    cmd.start("mind-vision",QStringList("list"));
    cmd.waitForFinished();
    auto s = cmd.readAllStandardOutput().split('\n'); s.removeLast();

    auto topLevelItems = topLevelItem->takeChildren();
    for(auto it=topLevelItems.begin();it != topLevelItems.end();it++) {
        auto item = *it;
        auto deviceItem = dynamic_cast<DeviceItem*>(item);

        auto lines = QList<QByteArray>(s);
        for(auto it=lines.begin();it!=lines.end();it++) {
            QString line = *it;
            auto info = line.split(','); //产品系列 产品名称 产品昵称 内核符号连接名 内部使用 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
            if(-1 == series.indexOf(info[0])) continue;

            if(deviceItem->cameraName == info[7]) {
                topLevelItem->addChild(deviceItem);
                s.removeOne(*it);
                goto FOUND_DEVICE;
            }
        }

        delete item;

FOUND_DEVICE:
        ;
    }
    auto lines = QList<QByteArray>(s);
    for(auto it=lines.begin();it!=lines.end();it++) {
        QString line = *it;
        auto info = line.split(','); //产品系列 产品名称 产品昵称 内核符号连接名 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
        if(-1 == series.indexOf(info[0])) continue;

        auto deviceName = QString("%1#%2").arg(info[1]).arg(info.size() > 9 ? info[9] : info[8]);

        auto child = new DeviceItem(topLevelItem,deviceName);
        child->setData(0,Qt::UserRole,info);
        child->cameraName = info[7];
        child->setIcon(0,QIcon(":/theme/icon/camera.png"));
    }
}

void TopLevelItemWidget::statusUpdate() {
    for(auto i=0;i< topLevelItem->childCount();i++) {
        auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(i));

        if (QProcess::NotRunning == deviceItem->camera.state()) {
            deviceItem->setIcon(0,QIcon(":/theme/icon/camera.png"));
            continue;
        }

        if(deviceItem->cameraView->playing()) deviceItem->setIcon(0,QIcon(":/theme/icon/playing.png"));
        else deviceItem->setIcon(0,QIcon(":/theme/icon/stopped.png"));
    }
}

DeviceItem::DeviceItem(QTreeWidgetItem *parent, QString name)
    : QTreeWidgetItem(parent, QStringList(name))
    , cameraView(nullptr)
{
    cameraView = new CameraView();
}

DeviceItem::~DeviceItem() {
    if(QProcess::ProcessState::Running == camera.state())
        camera.write("exit");

    delete cameraView;
}

#include <iostream>
using namespace std;

bool DeviceItem::open() {
    cout << "open " << cameraName.toStdString() << endl;

    QStringList args;
    args.append("open");
    args.append(cameraName);
    camera.setProgram("mind-vision");
    camera.setArguments(args);
    camera.start();
    while(camera.bytesAvailable() == 0) {
        camera.waitForReadyRead(1000);
    }

    auto s = camera.readAll();
    cout << s.data();
    auto res = s.split(' ');
    cameraView->pipeName = cameraName;
    if(res[0] == "True")
        cameraView->play();
    else
        camera.waitForFinished();
    return res[0] == "True";
}

void DeviceItem::close() {
    cameraView->stop();
    camera.write("exit\n");
    camera.waitForFinished();
    cout << "close " << cameraName.toStdString() << endl;
}

QStringList DeviceItem::exposure() {
    cout << "exposure " << endl;
    camera.write("exposure\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

void DeviceItem::exposureMode(int value) {
    camera.write(QString("exposure-mode-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::brightness(int value) {
    cout << "brightness " << value << endl;
    camera.write(QString("brightness-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
}

void DeviceItem::flicker(int value) {
    camera.write(QString("flicker-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::gain(int value) {
    camera.write(QString("gain-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::exposureTime(int value) {
    camera.write(QString("exposure-time-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::frequency(int value) {
    camera.write(QString("frequency-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

QStringList DeviceItem::whiteBalance() {
    cout << "whiteBalance " << endl;
    camera.write("white-balance\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

void DeviceItem::whiteBalanceMode(int index){
    camera.write(QString("white-balance-mode-set %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::onceWhiteBalance(){
    camera.write("once-white-balance\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::rgb(int r,int g,int b){
    camera.write(QString("rgb-set %1 %2 %3\n").arg(r).arg(g).arg(b).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::saturation(int value) {
    camera.write(QString("saturation-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::monochrome(int enable) {
    camera.write(QString("monochrome-set %1\n").arg(enable).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::inverse(int enable) {
    camera.write(QString("inverse-set %1\n").arg(enable).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::algorithm(int index) {
    cout << "algorithm " << index << endl;
    camera.write(QString("algorithm-set %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::colorTemrature(int index) {
    cout << "colorTemrature " << index << endl;
    camera.write(QString("color-temrature-set %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

QString DeviceItem::lookupTableMode() {
    cout << "lookupTableMode " << endl;
    camera.write("lookup-table-mode\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    return res[1];
}

void DeviceItem::lookupTableMode(int index) {
    cout << "lookupTableMode " << endl;
    camera.write(QString("lookup-table-mode-set %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = camera.readAll().split(' ');
    if(res[0] != "True") throw runtime_error("");
}

QStringList DeviceItem::lookupTablesForDynamic() {
    cout << "lookupTablesForDynamic " << endl;
    camera.write("lookup-tables-for-dynamic\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

void DeviceItem::gamma(int value){
    camera.write(QString("gamma-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::contrastRatio(int value) {
    camera.write(QString("contrast-ratio-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

QStringList DeviceItem::lookupTablesForPreset() {
    cout << "lookupTablesForPreset " << endl;
    camera.write("lookup-tables-for-preset\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

QStringList DeviceItem::lookupTablesForCustom(int index) {
    cout << "lookupTablesForCustom " << endl;
    camera.write(QString("lookup-tables-for-custom %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

void DeviceItem::lookupTablePreset(int index) {
    camera.write(QString("lookup-table-preset-set %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

QStringList DeviceItem::resolutions() {
    cout << "resolutions " << endl;
    camera.write("resolutions\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split('\n');
    res.removeLast();

    snapshotDialog.resolutions(res);
    return res;
}

void DeviceItem::resolution() {
    camera.write("resolution-get\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::resolution(int index) {
    camera.write(QString("resolution-set %1\n").arg(index).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

QStringList DeviceItem::isp(){
    cout << "isp " << endl;
    camera.write("isp\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

void DeviceItem::horizontalMirror(int value){
    camera.write(QString("horizontal-mirror-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::verticalMirror(int value){
    camera.write(QString("vertical-mirror-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::acutance(int value){
    camera.write(QString("acutance-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

QStringList DeviceItem::controls() {
    cout << "controls " << endl;
    camera.write(QString("controls\n").toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

void DeviceItem::triggerMode(int value) {
    cout << "trigger-mode " << value << endl;
    camera.write(QString("trigger-mode-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
}

void DeviceItem::onceSoftTrigger() {
    camera.write("once-soft-trigger\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::flashMode(int value) {
    camera.write(QString("flash-mode-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::flashPolarity(int value) {
    camera.write(QString("flash-polarity-set %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::paramsReset() {
    camera.write("params-reset\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::paramsSave(int value) {
    camera.write(QString("params-save %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::paramsLoad(int value) {
    camera.write(QString("params-load %1\n").arg(value).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::paramsSaveToFile(QString filename) {
    camera.write(QString("params-save-to-file %1\n").arg(filename).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::paramsLoadFromFile(QString filename) {
    camera.write(QString("params-load-from-file %1\n").arg(filename).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto res = QString(camera.readAll()).split(' ');
}

void DeviceItem::snapshotStart(QString dir,int resolution,int format,int period) {
    cout << "snapshot-start " << dir.toStdString() << resolution << format << period << endl;
    camera.write(QString("snapshot-start %1 %2 %3 %4\n").arg(dir).arg(resolution).arg(format).arg(period).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
}

bool DeviceItem::snapshotState() {
//    cout << "snapshot-state " << endl;
    if(camera.state() == QProcess::NotRunning) {
        cout << "False 0" << endl;
        return false;
    }

    camera.write("snapshot-state\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    return res[1] == "1";
}

void DeviceItem::snapshotStop() {
    camera.write("snapshot-stop\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
}
