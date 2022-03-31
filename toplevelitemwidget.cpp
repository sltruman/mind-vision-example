#include "toplevelitemwidget.h"
#include "ui_toplevelitemwidget.h"

#include <QSettings>
#include <QSharedMemory>
#include <QTextStream>
#include <QDebug>

#include <iostream>
#include <string>
using namespace std;

TopLevelItemWidget::TopLevelItemWidget(QTreeWidgetItem *item,QString series,QTreeWidget *parent) :
    QWidget(parent),
    ui(new Ui::TopLevelItemWidget),
    topLevelItem(item),
    series(series),
    parent(parent)
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
    auto devices = dm.list();

    cout << "refreshing" << std::endl;

    auto topLevelItems = topLevelItem->takeChildren();
    for(auto it=topLevelItems.begin();it != topLevelItems.end();it++) {
        auto item = *it;
        auto deviceItem = dynamic_cast<DeviceItem*>(item);

        auto device_it = std::find(devices.begin(),devices.end(),deviceItem->camera);

        if(device_it == devices.end()) {
            cout<< "delete device " << device_it->get()->info.acFriendlyName << std::endl;
            deviceItem->close();
            delete deviceItem;
            cout << "ok" << std::endl;
        } else {
            auto device = *device_it;
            cout << "add device " << device_it->get()->info.acFriendlyName << std::endl;
            topLevelItem->addChild(deviceItem);

            auto deviceName = (device->busying() ? "("+tr("Opened")+")" : "") + device->info.acFriendlyName;
            deviceItem->setText(0,deviceName);
            devices.remove(device);
            cout << "ok" << std::endl;
        }
    }

    for(auto it=devices.begin();it!=devices.end();it++) {
        auto device = *it;
        auto info = device->info;//产品系列 产品名称 产品昵称 内核符号连接名 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
        if(-1 == series.indexOf(info.acProductSeries)) continue;

        auto deviceName = (device->opened() ? "("+tr("Opened")+")" : "") + info.acFriendlyName;
        cout << "new device " << deviceName.toStdString() << std::endl;

        auto child = new DeviceItem(this,topLevelItem,deviceName,device);
        child->setIcon(0,QIcon(":/theme/icon/camera.png"));
    }
}

void TopLevelItemWidget::statusUpdate() {
    for(auto i=0;i< topLevelItem->childCount();i++) {
        auto deviceItem = dynamic_cast<DeviceItem*>(topLevelItem->child(i));

        auto deviceName = (deviceItem->camera->busying() ? "("+tr("Opened")+")" : "") + deviceItem->camera->info.acFriendlyName;
        deviceItem->setText(0,deviceName);

        if (!deviceItem->camera->opened()) {
            deviceItem->setIcon(0,QIcon(":/theme/icon/camera.png"));
            continue;
        }

        if(deviceItem->cameraView->playing) deviceItem->setIcon(0,QIcon(":/theme/icon/playing.png"));
        else deviceItem->setIcon(0,QIcon(":/theme/icon/stopped.png"));
    }
}

DeviceItem::DeviceItem(TopLevelItemWidget *w,QTreeWidgetItem *parent, QString name,shared_ptr<Device> camera)
    : QTreeWidgetItem(parent, QStringList(name))
    , camera(camera)
{
    cameraView = new CameraView(parent);
    cameraView->camera = camera;
}

DeviceItem::~DeviceItem() {
    delete cameraView;
}

bool DeviceItem::open() {

    if(cameraView->playing) cameraView->stop();

    if(!camera->open()) return false;
    cameraView->play();

    calibrationDialog.camera = camera;
    return true;
}

void DeviceItem::close() {
    if(camera->recording) camera->record_stop();
    if(camera->snapshoting) camera->snapshot_stop();
    cameraView->stop();
    camera->close();
}

std::tuple<QStringList,QStringList> DeviceItem::exposure(int full) {
    auto res = camera->exposure(full);
    QTextStream ts(res.data());

    if(full) {
        if(-1 == ts.readLine().indexOf("True")) throw runtime_error("");
        auto s = ts.readLine();
        cout << s.data();
        auto values = QString(s).split(',');
        s = ts.readLine();
        cout << s.data();
        auto window = QString(s).split(',');
        return make_tuple(values,window);
    }

    if(-1 == ts.readLine().indexOf("True")) throw runtime_error("");
    auto s = ts.readLine();
    auto values = s.split(',');
    s = ts.readLine();
    auto window = s.split(',');
    return make_tuple(values,window);
}

std::tuple<QStringList,QStringList> DeviceItem::whiteBalance() {
    auto res = camera->white_balance();
    QTextStream ts(res.data());

    if(ts.readLine().indexOf("True")) throw runtime_error("");
    auto s = ts.readLine();
    cout << s.data();
    auto values = QString(s).split(',');
    s = ts.readLine();
    cout << s.data();
    auto colorTemplates = s.split(',');
    colorTemplates.removeLast();
    return make_tuple(values,colorTemplates);
}

QStringList DeviceItem::lookupTablesForDynamic() {
    QTextStream ts(camera->lookup_tables_for_dynamic().data());
    auto s = ts.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

QStringList DeviceItem::lookupTablesForPreset() {
    QTextStream ts(camera->lookup_tables_preset().data());

    auto s = ts.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

QStringList DeviceItem::lookupTablesForCustom(int index) {
    QTextStream ts(camera->lookup_tables_for_custom(index).data());
    auto s = ts.readAll();
    cout << s.data();
    auto res = QString(s).split(' ');
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    return res;
}

QStringList DeviceItem::isp(){
    auto isp = camera->isp();
    QTextStream ts(isp.c_str());
    auto res = ts.readLine();
    cout << res.data();
    return res.split(' ');
}

QStringList DeviceItem::dead_pixels_analyze_for_bright(int threshold) {
    auto f = camera->dead_pixels_analyze_for_bright(threshold);
    auto res = QString(f.data()).split('\n');
    if(res.size()) res.removeLast();
    return res;
}

QStringList DeviceItem::dead_pixels_analyze_for_dead(int threshold) {
    auto f = camera->dead_pixels_analyze_for_dead(threshold);
    auto res = QString(f.data()).split('\n');
    if(res.size()) res.removeLast();
    return res;
}

std::tuple<QStringList,QStringList,QStringList> DeviceItem::video() {
    QTextStream ts(camera->video().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error(ts.readLine().toStdString());
    auto res = ts.readLine();
    cout << res.data();
    auto s = QString(res).split(',');
    res = ts.readLine();
    cout << res.data();
    auto output_formats = QString(res).split(',');
    output_formats.removeLast();
    res = ts.readLine();
    cout << res.data();
    auto frameSpeedTypes = res.split(',');
    frameSpeedTypes.removeLast();
    return make_tuple(s,output_formats,frameSpeedTypes);
}

QString DeviceItem::resolutionMode() {
    QTextStream ts(camera->resolutions().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error(ts.readLine().toStdString());
    auto s = ts.readAll();
    auto res = s.split('\n');
    return res[0].split(',')[0];
}

QString DeviceItem::resolutionIndex() {
    QTextStream ts(camera->resolutions().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error(ts.readLine().toStdString());
    auto res = ts.readAll().split('\n');
    return res[0].split(',')[1];
}

QStringList DeviceItem::resolution() {
    QTextStream ts(camera->resolutions().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error(ts.readLine().toStdString());
    auto res = ts.readAll().split('\n');
    return res[1].split(',');
}

QStringList DeviceItem::resolutions() {
    QTextStream ts(camera->resolutions().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error(ts.readLine().toStdString());
    auto s = ts.readAll();
    cout << s.data();
    auto res = s.split('\n');
    auto values = res[2].split(',');
    values.removeLast();
    return values;
}

QStringList DeviceItem::io() {
    auto io = camera->io();
    QTextStream ts(io.c_str());
    auto s = ts.readAll();
    cout << s.data();
    auto res = s.split("\n");
    res.removeLast();
    return res;
}

std::tuple<QStringList,QStringList,QStringList> DeviceItem::controls() {
    QTextStream ts(camera->controls().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error("");

    auto res = ts.readLine();
    cout << res.data();
    auto values = QString(res).split(',');

    res = ts.readLine();
    cout << res.data();
    auto signalTypes = QString(res).split(',');
    signalTypes.removeLast();

    res = ts.readLine();
    cout << res.data();
    auto shutterTypes = QString(res).split(',');
    shutterTypes.removeLast();

    return make_tuple(values,signalTypes,shutterTypes);
}

QStringList DeviceItem::firmware() {
    QTextStream ts(camera->firmware().data());
    if(0 != ts.readLine().indexOf("True")) throw runtime_error("");
    auto res = ts.readLine();
    cout << res.data();
    return res.split(',');
}

QStringList DeviceItem::status() {
    return QString(camera->status_string.str().data()).split(',');
}

//QString DeviceItem::brightness() {
//    return QString::number(camera->brightness());
//}
