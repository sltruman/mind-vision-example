#include "cameraview.h"
#include <QApplication>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>

CameraView::CameraView(QWidget *parent,QProcess* camera) : QGraphicsView(parent),camera(camera)
{
    setScene(new QGraphicsScene(this));
    background = nullptr;
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
    sock.disconnectFromServer();
}

void CameraView::paintEvent(QPaintEvent *event) {
    QGraphicsView::paintEvent(event);
}

void CameraView::process() {
    if (QLocalSocket::ConnectedState != sock.state()) return;

    sock.write("frame\n");
    sock.waitForBytesWritten();
    sock.waitForReadyRead();

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

    setSceneRect(0, 0, w, h);

    resetTransform();
    auto sw = width() / float(w);
    auto sh = height() / float(h);
    auto s = std::min(sw,sh);
    scale(s,s);

    emit sock.connected();
}
