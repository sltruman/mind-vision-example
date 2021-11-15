#include "toplevelitemwidget.h"
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
    cmd.start("/home/sl.truman/Desktop/build-mind-vision-Desktop-Debug/mind-vision",{"list"});
    cmd.waitForFinished();
    auto s = cmd.readAllStandardOutput().split('\n'); s.removeLast();

    for(auto item : topLevelItem->takeChildren()) {
        auto deviceItem = dynamic_cast<DeviceItem*>(item);

        for(auto line : QByteArrayList(s)) {
            auto info = line.split(' '); //产品系列 产品名称 产品昵称 内核符号连接名 内部使用 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
            if(info[0] != series) continue;

            if(deviceItem->cameraName == info[2]) {
                topLevelItem->addChild(deviceItem);
                s.removeOne(line);
                goto FOUND_DEVICE;
            }
        }

        delete item;

FOUND_DEVICE:
        ;
    }

    for(QString line : s) {
        auto info = line.split(' '); //产品系列 产品名称 产品昵称 内核符号连接名 内部使用 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
        if(info[0] != series) continue;

        auto deviceName = info[0] == "GIGE" ? QString("%1(%2)").arg(info[1]).arg(info[10]) : info[2];
        auto child = new DeviceItem(topLevelItem,deviceName);
        child->setData(0,Qt::UserRole,info);
        child->cameraName = info[2];
        child->setIcon(0,QIcon(":/切图-首页/录像.png"));
    }
}

void TopLevelItemWidget::statusUpdate() {
    for(auto i=0;i< topLevelItem->childCount();i++) {
        auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(i));

        if (QProcess::NotRunning == deviceItem->camera.state()) {
            deviceItem->setIcon(0,QIcon(":/切图-首页/录像.png"));
            continue;
        }

        if(deviceItem->cameraView->playing()) deviceItem->setIcon(0,QIcon(":/切图-首页/绿.png"));
        else deviceItem->setIcon(0,QIcon(":/切图-首页/红.png"));
    }
}

DeviceItem::DeviceItem(QTreeWidgetItem *parent, QString name)
    : QTreeWidgetItem(parent, {name})
    , cameraView(nullptr)
{
    cameraView = new CameraView();
}

DeviceItem::~DeviceItem() {
    if(QProcess::ProcessState::Running == camera.state())
        camera.write("exit");

    delete cameraView;
}

bool DeviceItem::open() {
    camera.start("/home/sl.truman/Desktop/build-mind-vision-Desktop-Debug/mind-vision",{"open",cameraName});
    camera.waitForReadyRead();
    auto res = camera.readLine().split(' ');

    cameraView->play(cameraName);
    return res[0] == "True";
}

void DeviceItem::close() {
    cameraView->stop();
    camera.write("exit\n");
    camera.waitForFinished();
}

#include <iostream>
using namespace std;

QStringList DeviceItem::exposure() {
    camera.write("exposure\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
    if(res[0] == "False") return {};
    res.removeFirst();
    return res;
}

void DeviceItem::exposureMode(int value) {
    camera.write(QString("exposure-mode-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::brightness(int value) {
    camera.write(QString("brightness-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::flicker(int value) {
    camera.write(QString("flicker-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::gain(int value) {
    camera.write(QString("gain-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::exposureTime(int value) {
    camera.write(QString("exposure-time-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::frequency(int value) {
    camera.write(QString("frequency-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

QStringList DeviceItem::whiteBalance() {
    camera.write("white-balance\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
    res.removeFirst();
    return res;
}

void DeviceItem::whiteBalanceMode(int index){
    camera.write(QString("white-balance-mode-set %1\n").arg(index).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::onceWhiteBalance(){
    camera.write("once-white-balance\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::r(int value){
    camera.write(QString("r-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::g(int value){
    camera.write(QString("g-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::b(int value){
    camera.write(QString("b-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::saturation(int value) {
    camera.write(QString("saturation-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

QStringList DeviceItem::lookupTables() {
    camera.write("lookup-tables\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
    res.removeFirst();
    return res;
}

void DeviceItem::gamma(int value){
    camera.write(QString("gamma-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::contrastRatio(int value) {
    camera.write(QString("contrast-ratio-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

QStringList DeviceItem::resolutions() {
    camera.write("resolutions\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readAll()).split('\n');
    res.removeLast();
    return res;
}

void DeviceItem::resolution() {
    camera.write("resolution-get\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::resolution(int index) {
    camera.write(QString("resolution-set %1\n").arg(index).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

QStringList DeviceItem::isp(){
    camera.write("isp\n");
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
    res.removeFirst();
    return res;
}

void DeviceItem::horizontalMirror(int value){
    camera.write(QString("horizontal-mirror-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::verticalMirror(int value){
    camera.write(QString("vertical-mirror-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::acutance(int value){
    camera.write(QString("acutance-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

QStringList DeviceItem::controls() {
    camera.write(QString("controls\n").toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
    res.removeFirst();
    return res;
}

void DeviceItem::triggerMode(int value) {
    camera.write(QString("trigger-mode-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::onceSoftTrigger() {
    camera.write(QString("once-soft-trigger\n").toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::flashMode(int value) {
    camera.write(QString("flash-mode-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}

void DeviceItem::flashPolarity(int value) {
    camera.write(QString("flash-polarity-set %1\n").arg(value).toLocal8Bit());
    camera.waitForReadyRead();
    auto res = QString(camera.readLine()).split(' ');
}
