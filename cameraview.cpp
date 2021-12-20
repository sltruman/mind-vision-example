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
    ui->pushButton_close->hide();
    setScene(new CameraScene(this));
    connect(this,SIGNAL(updated(const QImage&)),SLOT(update(const QImage&)));
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
//    ui->pushButton_close->show();
}

void CameraView::leaveEvent(QEvent *event) {
//    ui->pushButton_close->hide();
}


void CameraView::play() {
    cout << "play " << pipeName.toStdString() << endl;

    camera->write("play\n");
    while(camera->bytesAvailable() == 0) camera->waitForReadyRead(10);
    auto s = camera->readAll();
    playing = true;

    task = std::thread([&](){
        QLocalSocket sock;

        sock.connectToServer(pipeName);
        sock.waitForConnected();

        while(QLocalSocket::ConnectedState == sock.state() && playing) {
            tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

            if(-1 == sock.write("size\n")) break;

            while(sock.bytesAvailable() == 0) {
                sock.waitForReadyRead(20);
            }

            auto whb = sock.readLine().split(' ');
            auto w = whb[0].toUInt();
            auto h = whb[1].toUInt();
            auto b = whb[2].toUInt();
            auto length = w*h*b;
            rgbBuffer.resize(length);
            sock.setReadBufferSize(length);

            if(-1 == sock.write("frame\n")) break;

            for(auto i=0;i < length;i += sock.read(rgbBuffer.data() + i,length - i)) {
                sock.waitForReadyRead(20);
            }

            QImage img((unsigned char*)rgbBuffer.data(),w,h,b == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_Indexed8);

            if(QImage::Format::Format_Indexed8 == img.format()){
                QVector<QRgb> grayColorTable;
                for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
                img.setColorTable(grayColorTable);
            }

            auto elapsedTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - tick;
            displayFPS = 1.0f / elapsedTime * 1000;

            tick = QDateTime::currentDateTime().toMSecsSinceEpoch();
            frames++;

            emit updated(img);
        }

        sock.disconnectFromServer();
    });
}

void CameraView::update(const QImage &img) {
    auto cs = dynamic_cast<CameraScene*>(scene());
    cs->update(img);

    auto w = img.width();
    auto h = img.height();

    resetTransform();

    auto sw = width() / float(w);
    auto sh = height() / float(h);
    auto scaleValue = std::min(sw,sh) * currentScale;

    scale(scaleValue,scaleValue);
}

void CameraView::pause() {
    cout << "pause " << pipeName.toStdString() << endl;
    camera->write("pause\n");
    while(camera->bytesAvailable() == 0) camera->waitForReadyRead(10);
    auto s = camera->readAll();
    playing = false;
}

void CameraView::stop() {
    emit ui->pushButton_close->clicked();

    playing = false;

    cout << "stop " << pipeName.toStdString() << endl;
    camera->write("stop\n");
    while(camera->bytesAvailable() == 0) camera->waitForReadyRead(10);
    auto s = camera->readAll();

    task.join();
}

void CameraView::on_pushButton_close_clicked()
{
    setParent(nullptr);
}
