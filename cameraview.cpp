#include "cameraview.h"
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
    setAcceptDrops(false);
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

void CameraView::paintEvent(QPaintEvent *event) {
    QGraphicsView::paintEvent(event);
}

void CameraView::play() {
    cout << "play " << pipeName.toStdString() << endl;

    camera->write("play\n");
    playing = true;
    if(task.joinable()) return;

    interupt = false;

    sm.setKey(pipeName + ".0.sm");
    sm.attach();

    tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

    task = std::thread([=]() {
      while(!this->interupt) {
          auto frame_head_length = sizeof(FrameHead);

          if(framesCaptured > 1) {
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
              continue;
          }

          if(!sm.lock()) {
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
              continue;
          }

          auto frameHeadBuffer = reinterpret_cast<unsigned char*>(sm.data());
          auto frame_head = (FrameHead*)frameHeadBuffer;

          if(current_frame_head.num != frame_head->num) {
              current_frame_head = *frame_head;

              auto rgbBuffer = frameHeadBuffer + frame_head_length;

              img = QImage(rgbBuffer,frame_head->width,frame_head->height,frame_head->bits == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_Indexed8).copy();

              auto elapsedTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - tick;
              displayFPS = 1.0f / elapsedTime * 1000;
              tick = QDateTime::currentDateTime().toMSecsSinceEpoch();
          }

          sm.unlock();

          if(QImage::Format::Format_Indexed8 == img.format()) {
              QVector<QRgb> grayColorTable;
              for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
              img.setColorTable(grayColorTable);
              img = img.convertToFormat(QImage::Format::Format_RGB16);
          }

          framesCaptured++;
          emit updated(img);
      }
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

    task.join();
    sm.detach();

    scene()->addPixmap(QPixmap(0,0));
}

void CameraView::on_pushButton_close_clicked()
{
    setParent(nullptr);
}
