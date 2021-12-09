#include "cameraview.h"
#include "ui_cameraview.h"

#include <QApplication>
#include <QDateTime>
#include <QGraphicsScene>


#include <iostream>
using std::endl;
using std::cout;

CameraView::CameraView(QWidget *parent) :
    QGraphicsView(parent),
    ui(new Ui::CameraView),
    currentScale(0.99f),displayFPS(0),frames(0),playing(false)
{
    ui->setupUi(this);
    setScene(new QGraphicsScene(this));
    background = scene()->addPixmap(QPixmap(0,0));
    connect(&sock,SIGNAL(connected()),SLOT(process()),Qt::QueuedConnection);
    ui->pushButton_close->hide();
}

CameraView::~CameraView()
{
    sock.disconnectFromServer();
    delete ui;
}

void CameraView::enterEvent(QEvent *event) {
//    ui->pushButton_close->show();
}

void CameraView::leaveEvent(QEvent *event) {
//    ui->pushButton_close->hide();
}

void CameraView::play() {
    cout << "play " << pipeName.toStdString() << endl;
    sock.connectToServer(pipeName);
    camera->write("play\n");
    while(camera->bytesAvailable() == 0) camera->waitForReadyRead(10);
    auto s = camera->readAll();
    playing = true;
}

void CameraView::pause() {
    cout << "pause " << sock.serverName().toStdString() << endl;
    camera->write("pause\n");
    while(camera->bytesAvailable() == 0) camera->waitForReadyRead(10);
    auto s = camera->readAll();
    playing = false;
}

void CameraView::stop() {
    cout << "stop " << sock.serverName().toStdString() << endl;
    camera->write("stop\n");
    while(camera->bytesAvailable() == 0) camera->waitForReadyRead(10);
    auto s = camera->readAll();
    sock.disconnectFromServer();
    playing = false;
}

void CameraView::process() {
    static auto tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (QLocalSocket::ConnectedState != sock.state()) {
        return;
    }

    sock.write("size\n");
    sock.waitForBytesWritten();

    while(sock.bytesAvailable() == 0) {
        QApplication::processEvents();
        sock.waitForReadyRead(20);
        if (QLocalSocket::ConnectedState != sock.state()) return;
    }

    auto whb = sock.readLine().split(' ');
    auto w = whb[0].toUInt();
    auto h = whb[1].toUInt();
    auto b = whb[2].toUInt();
    auto length = w*h*b;
    rgbBuffer.resize(length);

    sock.write("frame\n");

    for(auto i=0;i < length;i += sock.read(rgbBuffer.data() + i,length - i)) {
        QApplication::processEvents();
        sock.waitForReadyRead(20);
        if (QLocalSocket::ConnectedState != sock.state()) return;
    }

    QImage img((unsigned char*)rgbBuffer.data(),w,h,b == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_Indexed8);

    if(QImage::Format::Format_Indexed8 == img.format()){
        QVector<QRgb> grayColorTable;
        for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
        img.setColorTable(grayColorTable);
    }

    scene()->clear();
    background = scene()->addPixmap(QPixmap::fromImage(img));

    for(auto line : lines) {
        auto x = std::get<0>(line);
        auto y = std::get<1>(line);
        auto pen = std::get<2>(line);
        scene()->addLine(x,0,x,h,pen);
        scene()->addLine(0,y,w,y,pen);
    }

    setSceneRect(0,0,w,h);
    resetTransform();

    auto sw = width() / float(w);
    auto sh = height() / float(h);
    auto scaleValue = std::min(sw,sh) * currentScale;

    scale(scaleValue,scaleValue);

    auto elapsedTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - tick;
    displayFPS = 1.0f / elapsedTime * 1000;

    tick = QDateTime::currentDateTime().toMSecsSinceEpoch();
    frames++;

    emit sock.connected();
}

void CameraView::on_pushButton_close_clicked()
{
    setParent(nullptr);
}

