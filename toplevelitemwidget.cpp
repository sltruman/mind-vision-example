#include "toplevelitemwidget.h"
#include "ui_toplevelitemwidget.h"

#include <QSharedMemory>
#include <QTextStream>

#include <iostream>
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

                auto deviceName = (info[8].toUInt() ? "("+tr("Opened")+")" : "") + info[2];
                deviceItem->setText(0,deviceName);

                s.removeOne(*it);
                goto FOUND_DEVICE;
            }
        }

        deviceItem->close();
        delete deviceItem;

FOUND_DEVICE:
        ;
    }
    auto lines = QList<QByteArray>(s);
    for(auto it=lines.begin();it!=lines.end();it++) {
        QString line = QString::fromLocal8Bit(*it);
        auto info = line.split(','); //产品系列 产品名称 产品昵称 内核符号连接名 驱动版本 sensor类型 接口类型 产品唯一序列号 实例索引号 相机IP 相机子网掩码 相机网关 网卡IP 网卡子网掩码 网卡网关
        if(-1 == series.indexOf(info[0])) continue;

//        auto deviceName = info[2].isEmpty() ? QString("%1#%2").arg(info[1]).arg(info.size() > 9 ? info[9] : info[8]) : info[2];
        auto deviceName = (info[8].toUInt() ? "("+tr("Opened")+")" : "") + info[2];

        auto child = new DeviceItem(this,topLevelItem,deviceName);
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

        if(deviceItem->cameraView->playing) deviceItem->setIcon(0,QIcon(":/theme/icon/playing.png"));
        else deviceItem->setIcon(0,QIcon(":/theme/icon/stopped.png"));
    }
}



DeviceItem::DeviceItem(TopLevelItemWidget *w,QTreeWidgetItem *parent, QString name)
    : QTreeWidgetItem(parent, QStringList(name))
{
    cameraView = new CameraView(parent);
}

DeviceItem::~DeviceItem() {
    if(QProcess::ProcessState::Running == camera.state())
        camera.write("exit");

    delete cameraView;
}

bool DeviceItem::open() {
    cout << "open " << cameraName.toStdString() << endl;

    QStringList args;
    args.append("open");
    args.append(cameraName);
    camera.setProgram("mind-vision");
    camera.setArguments(args);
    camera.start();

    while(camera.bytesAvailable() == 0)  {
        camera.waitForReadyRead(10);
    }

    auto s = camera.readAll();
    auto res = s.split(' ');
    cameraView->camera = &camera;
    cameraView->pipeName = cameraName;
    calibrationDialog.camera = &camera;
    if(-1 == res[0].indexOf("True")) {
        camera.waitForFinished();
        return false;
    }

    cameraView->play();
    calibrationDialog.camera = &camera;
    return true;
}

void DeviceItem::close() {
    cameraView->stop();
    if(recordState()) recordStop();
    if(snapshotState()) snapshotStop();

    camera.write("exit\n");
    camera.waitForFinished();
    cout << "close " << cameraName.toStdString() << endl;
}


std::tuple<QStringList,QStringList> DeviceItem::exposure(int full) {
    cout << "exposure " << endl;
    camera.write(QString("exposure %1\n").arg(full).toLocal8Bit());

    if(full) {
        while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
        if(-1 == camera.readLine().indexOf("True")) throw runtime_error("");
        auto s = camera.readLine();
        cout << s.data();
        auto values = QString(s).split(',');
        s = camera.readLine();
        cout << s.data();
        auto window = QString(s).split(',');
        return make_tuple(values,window);
    }

    QTextStream ts(cameraView->current_frame_head.exposure_status);
    if(-1 == ts.readLine().indexOf("True")) throw runtime_error("");
    auto s = ts.readLine();
    auto values = QString(s).split(',');
    s = ts.readLine();
    auto window = QString(s).split(',');
    return make_tuple(values,window);
}

void DeviceItem::exposureMode(int value) {
    camera.write(QString("exposure-mode-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::brightness(int value) {
    cout << "brightness " << value << endl;
    camera.write(QString("brightness-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::threshold(int value) {
    cout << "threshold " << value << endl;
    camera.write(QString("threshold-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::flicker(int value) {
    camera.write(QString("flicker-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::gain(int value) {
    cout << "gain-set " << value << endl;
    camera.write(QString("gain-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::gainRange(int minimum,int maximum) {
    cout << "gain-range-set " << minimum << ' ' << maximum << endl;
    camera.write(QString("gain-range-set %1 %2\n").arg(minimum).arg(maximum).toLocal8Bit());
}

void DeviceItem::exposureTime(int value) {
    cout << "exposure-time-set " << value << endl;
    camera.write(QString("exposure-time-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::exposureTimeRange(int minimum,int maximum) {
    cout << "exposure-time-range-set " << minimum << ' ' << maximum << endl;
    camera.write(QString("exposure-time-range-set %1 %2\n").arg(minimum).arg(maximum).toLocal8Bit());
}

void DeviceItem::frequency(int value) {
    camera.write(QString("frequency-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::exposureWindow(int x,int y,int w,int h) {
    camera.write(QString("exposure-window-set %1 %2 %3 %4\n").arg(x).arg(y).arg(w).arg(h).toLocal8Bit());
}

std::tuple<QStringList,QStringList> DeviceItem::whiteBalance() {
    cout << "whiteBalance " << endl;
    camera.write("white-balance\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(camera.readLine().indexOf("True")) throw runtime_error("");
    auto s = camera.readLine();
    cout << s.data();
    auto values = QString(s).split(',');
    s = camera.readLine();
    cout << s.data();
    auto colorTemplates = QString::fromLocal8Bit(s).split(',');
    colorTemplates.removeLast();
    return make_tuple(values,colorTemplates);
}

void DeviceItem::whiteBalanceMode(int index){
    camera.write(QString("white-balance-mode-set %1\n").arg(index).toLocal8Bit());
}

void DeviceItem::onceWhiteBalance(){
    camera.write("once-white-balance\n");
}

void DeviceItem::whiteBalanceWindow(int x,int y,int w,int h) {
    camera.write(QString("white-balance-window-set %1 %2 %3 %4\n").arg(x).arg(y).arg(w).arg(h).toLocal8Bit());
}

void DeviceItem::rgb(int r,int g,int b){
    camera.write(QString("rgb-set %1 %2 %3\n").arg(r).arg(g).arg(b).toLocal8Bit());
}

void DeviceItem::saturation(int value) {
    camera.write(QString("saturation-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::monochrome(int enable) {
    camera.write(QString("monochrome-set %1\n").arg(enable).toLocal8Bit());
}

void DeviceItem::inverse(int enable) {
    camera.write(QString("inverse-set %1\n").arg(enable).toLocal8Bit());
}

void DeviceItem::algorithm(int index) {
    cout << "algorithm " << index << endl;
    camera.write(QString("algorithm-set %1\n").arg(index).toLocal8Bit());
}

void DeviceItem::colorTemrature(int index) {
    cout << "colorTemrature " << index << endl;
    camera.write(QString("color-temrature-set %1\n").arg(index).toLocal8Bit());
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
}

void DeviceItem::contrastRatio(int value) {
    camera.write(QString("contrast-ratio-set %1\n").arg(value).toLocal8Bit());
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
}

QString DeviceItem::resolutionMode() {
    cout << "resolutions " << endl;
    camera.write("resolutions\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto s = camera.readAll();
    auto res = QString(s).split('\n');
    return res[0].split(',')[0];
}

QString DeviceItem::resolutionIndex() {
    cout << "resolution " << endl;
    camera.write("resolutions\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto res = QString(camera.readAll()).split('\n');
    return res[0].split(',')[1];
}

QStringList DeviceItem::resolution() {
    cout << "resolution " << endl;
    camera.write("resolutions\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto res = QString(camera.readAll()).split('\n');
    return res[1].split(',');
}

QStringList DeviceItem::resolutions() {
    cout << "resolutions " << endl;
    camera.write("resolutions\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString::fromLocal8Bit(s).split('\n');
    auto values = res[2].split(',');
    values.removeLast();
    return values;
}

void DeviceItem::resolution(int index) {
    cout << "resolution-set " << index << endl;
    camera.write(QString("resolution-set %1\n").arg(index).toLocal8Bit());
}

void DeviceItem::resolution(int x,int y,int w,int h) {
    cout << "resolution-custom-set " << x << ',' << y << ',' << w << ',' << h << endl;
    camera.write(QString("resolution-custom-set %1 %2 %3 %4\n").arg(x).arg(y).arg(w).arg(h).toLocal8Bit());
}

QStringList DeviceItem::isp(){
    cout << "transform " << endl;
    camera.write("transform\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error("");
    auto res = camera.readLine();
    cout << res.data();
    return QString(res).split(' ');
}

void DeviceItem::horizontalMirror(int hard,int value){
    camera.write(QString("horizontal-mirror-set %1 %2\n").arg(hard).arg(value).toLocal8Bit());
}

void DeviceItem::verticalMirror(int hard,int value){
    camera.write(QString("vertical-mirror-set %1 %2\n").arg(hard).arg(value).toLocal8Bit());
}

void DeviceItem::acutance(int value){
    camera.write(QString("acutance-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::noise(int enable) {
    camera.write(QString("noise-set %1\n").arg(enable).toLocal8Bit());
}

void DeviceItem::noise3d(int enable,int value) {
    camera.write(QString("noise3d-set %1 %2\n").arg(enable).arg(value).toLocal8Bit());
}

void DeviceItem::rotate(int value) {
    camera.write(QString("rotate-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::deadPixels(QString x, QString y) {
    camera.write("dead-pixels-set\n");
    camera.write(QString("%1\n").arg(x.isEmpty() ? "None" : x).toLocal8Bit());
    camera.write(QString("%1\n").arg(y.isEmpty() ? "None" : y).toLocal8Bit());
}

QStringList DeviceItem::dead_pixels_analyze_for_bright(int threshold) {
    camera.write(QString("dead-pixels-analyze-for-bright %1\n").arg(threshold).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto ipc = camera.readAll();

    QFile f(ipc.replace("\r\n",""));
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    auto res = QString(f.readAll()).split('\n');
    if(res.size()) res.removeLast();
    f.close();
    return res;
}

QStringList DeviceItem::dead_pixels_analyze_for_dead(int threshold) {
    cout << "dead-pixels-analyze-for-dead " << threshold << endl;
    camera.write(QString("dead-pixels-analyze-for-dead %1\n").arg(threshold).toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto ipc = camera.readAll();

    QFile f(ipc.replace("\r\n",""));
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    auto res = QString(f.readAll()).split('\n');
    if(res.size()) res.removeLast();
    f.close();
    return res;
}

void DeviceItem::flatFieldCorrent(int enable) {
    camera.write(QString("flat-field-corrent-set %1\n").arg(enable).toLocal8Bit());
}

void DeviceItem::flatFieldInit(int light) {
    camera.write(QString("flat-field-init %1\n").arg(light).toLocal8Bit());
}

void DeviceItem::flatFieldParamsSave(QString filepath) {
    camera.write(QString("flat-field-params-save %1\n").arg(filepath).toLocal8Bit());
}

void DeviceItem::flatFieldParamsLoad(QString filepath) {
    camera.write(QString("flat-field-params-load %1\n").arg(filepath).toLocal8Bit());
}

void DeviceItem::undistort(int enable) {
    cout << "undistort " << enable << endl;
    camera.write(QString("undistort-set %1\n").arg(enable).toLocal8Bit());
}

void DeviceItem::undistortParams(int w,int h,QString cameraMatrix,QString distortCoeffs) {
    cout << "undistort " << w << ' ' << h << ' ' << cameraMatrix.toStdString() << ' ' << distortCoeffs.toStdString() << endl;
    camera.write(QString("undistort-params-set %1 %2 %3 %4\n").arg(w).arg(h).arg(cameraMatrix).arg(distortCoeffs).toLocal8Bit());
}

std::tuple<QStringList,QStringList,QStringList> DeviceItem::video() {
    cout << "video " << endl;
    camera.write("video\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error(camera.readLine());
    auto res = camera.readLine();
    cout << res.data();
    auto s = QString(res).split(',');
    res = camera.readLine();
    cout << res.data();
    auto output_formats = QString(res).split(',');
    output_formats.removeLast();
    res = camera.readLine();
    cout << res.data();
    auto frameSpeedTypes = QString::fromLocal8Bit(res).split(',');
    frameSpeedTypes.removeLast();
    return make_tuple(s,output_formats,frameSpeedTypes);
}

void DeviceItem::frameRateSpeed(int index) {
    camera.write(QString("frame-rate-speed-set %1\n").arg(index).toLocal8Bit());
}

void DeviceItem::frameRateLimit(int value) {
    camera.write(QString("frame-rate-limit-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::rawOutputFormat(int index) {
    camera.write(QString("video-output-format-set %1\n").arg(index).toLocal8Bit());
}

void DeviceItem::rawOutputRange(int value) {
    cout << "raw-output-range-set " << value << endl;
    camera.write(QString("raw-output-range-set %1\n").arg(value).toLocal8Bit());
}

QStringList DeviceItem::io() {
    cout << "io " << endl;
    camera.write(QString("io\n").toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    auto s = camera.readAll();
    cout << s.data();
    auto res = QString(s).split("\r\n");
    if(res[0] != "True") throw runtime_error("");
    res.removeFirst();
    res.removeLast();
    return res;
}

void DeviceItem::ioMode(QString type,int index,int value) {
    cout << "io-mode-set " <<  type.toLocal8Bit().data() << ' ' << index << ' ' << value << endl;
    camera.write(QString("io-mode-set %1 %2 %3\n").arg(type).arg(index).arg(value).toLocal8Bit());
}

void DeviceItem::ioState(QString type,int index,int value) {
    cout << "io-state-set " <<  type.toLocal8Bit().data() << ' ' << index << ' ' << value << endl;
    camera.write(QString("io-state-set %1 %2 %3\n").arg(type).arg(index).arg(value).toLocal8Bit());
}

std::tuple<QStringList,QStringList,QStringList> DeviceItem::controls() {
    cout << "controls " << endl;
    camera.write(QString("controls\n").toLocal8Bit());
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error("");

    auto res = camera.readLine();
    cout << res.data();
    auto values = QString(res).split(',');

    res = camera.readLine();
    cout << res.data();
    auto signalTypes = QString(res).split(',');
    signalTypes.removeLast();

    res = camera.readLine();
    cout << res.data();
    auto shutterTypes = QString(res).split(',');
    shutterTypes.removeLast();

    return make_tuple(values,signalTypes,shutterTypes);
}

void DeviceItem::triggerMode(int value) {
    cout << "trigger-mode " << value << endl;
    camera.write(QString("trigger-mode-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::onceSoftTrigger() {
    camera.write("once-soft-trigger\n");
}

void DeviceItem::triggerFrames(int value) {
    camera.write(QString("trigger-frames-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::triggerDelay(int value) {
    camera.write(QString("trigger-delay-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::triggerInterval(int value) {
    camera.write(QString("trigger-interval-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::outsideTriggerMode(int value) {
    camera.write(QString("outside-trigger-mode-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::outsideTriggerDebounce(int value) {
    camera.write(QString("outside-trigger-debounce-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::outsideShutter(int index) {
    camera.write(QString("outside-shutter-set%1\n").arg(index).toLocal8Bit());
}

void DeviceItem::flashMode(int value) {
    camera.write(QString("flash-mode-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::flashPolarity(int value) {
    camera.write(QString("flash-polarity-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::strobeDelay(int value) {
    camera.write(QString("flash-delay-set %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::strobePulse(int value) {
    camera.write(QString("flash-pulse-set %1\n").arg(value).toLocal8Bit());
}

QStringList DeviceItem::firmware() {
    camera.write("firmware\n");
    while(camera.bytesAvailable() == 0) camera.waitForReadyRead(10);
    if(0 != camera.readLine().indexOf("True")) throw runtime_error("");
    auto res = camera.readLine();
    cout << res.data();
    return QString::fromLocal8Bit(res).split(',');
}

void DeviceItem::rename(QString name) {
    if(name.isEmpty()) {
        camera.write("rename-empty\n");
    } else {
        camera.write(QString("rename %1\n").arg(name).toLocal8Bit());
    }
}

void DeviceItem::paramsReset() {
    camera.write("params-reset\n");
}

void DeviceItem::paramsSave(int value) {
    camera.write(QString("params-save %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::paramsLoad(int value) {
    camera.write(QString("params-load %1\n").arg(value).toLocal8Bit());
}

void DeviceItem::paramsSaveToFile(QString filename) {
    camera.write(QString("params-save-to-file %1\n").arg(filename).toLocal8Bit());
}

void DeviceItem::paramsLoadFromFile(QString filename) {
    camera.write(QString("params-load-from-file %1\n").arg(filename).toLocal8Bit());
}

void DeviceItem::snapshotStart(QString dir,int resolution,int format,int period) {
    cout << "snapshot-start " << dir.toStdString() << resolution << format << period << endl;
    camera.write(QString("snapshot-start %1 %2 %3 %4\n").arg(dir).arg(resolution).arg(format).arg(period).toLocal8Bit());
}

bool DeviceItem::snapshotState() {
    return cameraView->current_frame_head.snapshot_status;
}

void DeviceItem::snapshotStop() {
    camera.write("snapshot-stop\n");
}

void DeviceItem::recordStart(QString dir,int format,int quality,int frames) {
    cout << "record-start " << dir.toStdString()<< ' ' <<  format << ' ' << quality << ' ' << frames << endl;
    camera.write(QString("record-start %1 %2 %3 %4\n").arg(dir).arg(format).arg(quality).arg(frames).toLocal8Bit());
}

bool DeviceItem::recordState() {
    return cameraView->current_frame_head.record_status;
}

void DeviceItem::recordStop() {
    camera.write("record-stop\n");
}

QStringList DeviceItem::status(QString type) {
    cout << "status " << type.toLocal8Bit().data() << endl;
    camera.write(QString("status %1\n").arg(type).toLocal8Bit());

    QTextStream ts(cameraView->current_frame_head.camera_status);
    if(-1 == ts.readLine().indexOf("True")) throw runtime_error("");
    return QString(ts.readLine()).split(',');
}

QString DeviceItem::brightness() {
    cout << "brightness " << endl;
    camera.write("brightness \n");
    while(camera.bytesAvailable() == 0)
        camera.waitForReadyRead(10);

    if(camera.readLine().indexOf("True")) throw runtime_error("");
    auto s = camera.readLine();
    cout << s.data();
    return QString(s).split(',')[0];
}
