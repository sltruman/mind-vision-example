﻿#include "cameraview.h"
#include "ui_cameraview.h"

#include <QApplication>
#include <QDateTime>
#include <QGraphicsScene>

#include <iostream>
using std::endl;
using std::cout;

CameraView::CameraView(QTreeWidgetItem* owner,QWidget *parent) :
    QGraphicsView(parent),
    ui(new Ui::CameraView),
    currentScale(0.99f),displayFPS(0),playing(false),
    leftButtonPressed(false),
    owner(owner),
    avgBrightness(false),
    framesCaptured(0)
{
    ui->setupUi(this);
    ui->pushButton_close->hide();
    setScene(new CameraScene(this));
    connect(this,SIGNAL(updated(const QImage&)),SLOT(update(const QImage&)));
    setAcceptDrops(false);
}

CameraView::~CameraView()
{
    delete ui;
}

void CameraView::closeEvent(QCloseEvent *event) {
    event->ignore();
}

void CameraView::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Escape) {
        setWindowFlags(Qt::SubWindow);
        showNormal();
    }
}

void CameraView::enterEvent(QEvent *event) {

}

void CameraView::leaveEvent(QEvent *event) {

}

void CameraView::wheelEvent(QWheelEvent *event) {
    if(event->angleDelta().y() > 0) zoom(1.1);
    else zoom(0.9);
}

void CameraView::focusInEvent(QFocusEvent *event) {
    cout << "focused " << pipeName.toStdString() << endl;
    emit focused();
}

void CameraView::mouseDoubleClickEvent(QMouseEvent *event) {
    cout << "doubleClick"  << pipeName.toStdString() << endl;
    emit doubleClick();
}

//void CameraView::mousePressEvent(QMouseEvent *event) {

//}

//void CameraView::mouseMoveEvent(QMouseEvent *event) {
//    if(leftButtonPressed) {
//        posEnd = event->pos() - posBegin;
//    }
//}

//void CameraView::mouseReleaseEvent(QMouseEvent *event) {
//    leftButtonPressed = false;
//}

void CameraView::zoom(float v) {
    if(v < 1 && currentScale < 0.1f) return;
    if(v > 1 && currentScale > 500 ) return;
    currentScale *= v;
    update(img);
}

void CameraView::play() {
    cout << "play " << pipeName.toStdString() << endl;

    camera->write("play\n");
    playing = true;

    if(task.joinable()) return;
    interupt = false;

    task = std::thread([&](){
        QLocalSocket sock;

        sock.connectToServer(pipeName);
        sock.waitForConnected();

        while(QLocalSocket::ConnectedState == sock.state() && !this->interupt) {
            tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

            if(-1 == sock.write("normal\n")) break;

            while(sock.bytesAvailable() == 0 || framesCaptured > 1) {
                sock.waitForReadyRead(100);
                if(this->interupt) break;
            }

            auto whb = sock.readLine().split(' ');
            if(whb.size() < 3) break;

            auto wm = whb[0].toUInt();
            auto hm = whb[1].toUInt();
            auto w = whb[2].toUInt();
            auto h = whb[3].toUInt();
            auto b = whb[4].toUInt();
            auto length = w*h*b;

            if(length) {
                rgbBuffer.resize(length);
                sock.setReadBufferSize(length);

                if(-1 == sock.write("frame\n")) break;

                for(auto i=0;i < length;i += sock.read(rgbBuffer.data() + i,length - i)) {
                    sock.waitForReadyRead(100);
                    if(this->interupt) break;
                }

                img = QImage((unsigned char*)rgbBuffer.data(),w,h,b == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_Indexed8).copy();

                if(QImage::Format::Format_Indexed8 == img.format()) {
                    QVector<QRgb> grayColorTable;
                    for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
                    img.setColorTable(grayColorTable);
                    img = img.convertToFormat(QImage::Format::Format_RGB16);
                }

                auto elapsedTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - tick;
                displayFPS = 1.0f / elapsedTime * 1000;
                tick = QDateTime::currentDateTime().toMSecsSinceEpoch();
            } else {
                displayFPS = 0;
            }

            framesCaptured++;
            emit updated(img);
        }

        sock.disconnectFromServer();
    });
}

void CameraView::update(const QImage &img) {
    framesCaptured = --framesCaptured < 0 ? 0 : framesCaptured;

    auto cs = dynamic_cast<CameraScene*>(scene());

    auto w=img.width();
    auto h=img.height();
    cs->update(img);

    resetTransform();

    auto sw = width() / float(w);
    auto sh = height() / float(h);
    auto scaleValue = std::min(sw,sh) * currentScale;

    scale(scaleValue,scaleValue);

    if(cs->exposureWindow || cs->whiteBalanceWindow || cs->deadPixelWindow || cs->resolutionWindow)
        setDragMode(QGraphicsView::NoDrag);
    else
        setDragMode(QGraphicsView::ScrollHandDrag);

    coordinate = QString("%1,%2").arg(cs->coordinate.x()).arg(cs->coordinate.y());
    if(img.valid(cs->coordinate.x(),cs->coordinate.y())) {
        rgb = img.pixel(cs->coordinate.x(),cs->coordinate.y());
        brightness = 0.299*rgb.red() + 0.587*rgb.green() + 0.114*rgb.blue();
    }
}

void CameraView::pause() {
    cout << "pause " << pipeName.toStdString() << endl;
    camera->write("pause\n");
    playing = false;
}

void CameraView::stop() {
    if(interupt) {
        return;
    }

    emit ui->pushButton_close->clicked();

    interupt = true;
    playing = false;

    cout << "stop " << pipeName.toStdString() << endl;
    camera->write("stop\n");
    task.join();

    scene()->addPixmap(QPixmap(0,0));
}

void CameraView::on_pushButton_close_clicked()
{
    setParent(nullptr);
}
