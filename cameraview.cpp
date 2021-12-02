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
    currentScale(0.99f),displayFPS(0),frames(0)
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

bool CameraView::playing() {
    return QLocalSocket::ConnectedState == sock.state();
}

void CameraView::play() {
    cout << "play " << pipeName.toStdString() << endl;
    sock.connectToServer(pipeName);
}

void CameraView::stop() {
    cout << "stop " << sock.serverName().toStdString() << endl;
    scene()->clear();
    background = scene()->addPixmap(QPixmap(0,0));
    sock.disconnectFromServer();
}

void CameraView::process() {
    static auto tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (QLocalSocket::ConnectedState != sock.state()) {
        scene()->clear();
        return;
    }

    sock.write("size\n");
    sock.waitForBytesWritten();

    while(sock.bytesAvailable() == 0) {
        sock.waitForReadyRead(100);
        QApplication::processEvents();
        if (QLocalSocket::ConnectedState != sock.state()) return;
    }

    auto whb = sock.readLine().split(' ');
    auto w = whb[0].toUInt();
    auto h = whb[1].toUInt();
    auto b = whb[2].toUInt();
    auto length = w*h*b;
    rgbBuffer.resize(length);

    sock.write("frame\n");
    while(sock.bytesAvailable() < length) {
        sock.waitForReadyRead(100);
        QApplication::processEvents();

        if (QLocalSocket::ConnectedState != sock.state()) return;
    }

    sock.read(rgbBuffer.data(),rgbBuffer.size());
    QImage img((unsigned char*)rgbBuffer.data(),w,h,b == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_Indexed8);

    if(QImage::Format::Format_Indexed8 == img.format()){
        QVector<QRgb> grayColorTable;
        for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
        img.setColorTable(grayColorTable);
    }

    scene()->clear();
    background = scene()->addPixmap(QPixmap::fromImage(img));
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

