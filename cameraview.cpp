#include "cameraview.h"
#include <QApplication>
#include <QDateTime>

#include <iostream>
using std::endl;
using std::cout;
CameraView::CameraView(QProcess* camera,QWidget *parent) : QGraphicsView(parent),camera(camera),currentScale(0.99f),displayFPS(0),frames(0)
{
    setScene(new QGraphicsScene(this));
    background = scene()->addPixmap(QPixmap());
    connect(&sock,SIGNAL(connected()),SLOT(process()),Qt::QueuedConnection);
}

CameraView::~CameraView() {
    sock.disconnectFromServer();
}


bool CameraView::playing() {
    return QLocalSocket::ConnectedState == sock.state();
}

void CameraView::play(QString pipeName) {
    sock.connectToServer(pipeName);
}

void CameraView::stop() {
    scene()->clear();
    sock.disconnectFromServer();
}

void CameraView::paintEvent(QPaintEvent *event) {
    QGraphicsView::paintEvent(event);
}

void CameraView::process() {
    static auto tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (QLocalSocket::ConnectedState != sock.state()) {
        scene()->clear();
        return;
    }

    sock.write("frame\n");
    while(sock.bytesAvailable() == 0) {
        sock.waitForReadyRead(10);
        QApplication::processEvents();
        if (QLocalSocket::ConnectedState != sock.state()) return;
    }

    auto whb = sock.readLine().split(' ');
    auto w = whb[0].toUInt();
    auto h = whb[1].toUInt();
    auto b = whb[2].toUInt();
    auto length = w*h*b;

    while(sock.bytesAvailable() < length) {
        sock.waitForReadyRead(10);
        QApplication::processEvents();

        if (QLocalSocket::ConnectedState != sock.state()) return;
    }

    auto rgbBuff = sock.readAll();
    QImage img((unsigned char*)rgbBuff.data(),w,h,b == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_Indexed8);

    if(QImage::Format::Format_Indexed8 == img.format()){
        QVector<QRgb> grayColorTable;
        for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
        img.setColorTable(grayColorTable);
    }

    scene()->clear();
    background = scene()->addPixmap(QPixmap::fromImage(img));

    resetTransform();

    auto sw = width() / float(w);
    auto sh = height() / float(h);
    auto scaleValue = std::min(sw,sh) * currentScale;

    scale(scaleValue,scaleValue);

    emit sock.connected();

    auto elapsedTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - tick;
    displayFPS = 1.0f / elapsedTime * 1000;

    tick = QDateTime::currentDateTime().toMSecsSinceEpoch();
    frames++;
}
