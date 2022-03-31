#include "cameraview.h"
#include "ui_cameraview.h"

#include <QApplication>
#include <QDateTime>
#include <QGraphicsScene>
#include <QTextStream>

#include <iostream>
#include <tuple>
using std::endl;
using std::cout;
using std::tuple;
using std::make_tuple;

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
    connect(this,SIGNAL(updated(const QPixmap&)),SLOT(update(const QPixmap&)));
}

CameraView::~CameraView()
{
    stop();
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
    update(QPixmap::fromImage(img));
}

void CameraView::paintEvent(QPaintEvent *event) {
    QGraphicsView::paintEvent(event);
}

void CameraView::play() {
    camera->play();
    playing = true;
    if(task.joinable()) return;

    interupt = false;

    task = std::thread([=]() {
        unsigned long long lastStatusTick = 0,tick = 0;

        while(!this->interupt) {
          auto lastStatusTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - lastStatusTick;

          if(lastStatusTime > 1000) {
              camera->status_sync();
              lastStatusTick = QDateTime::currentDateTime().toMSecsSinceEpoch();
          }

          if(framesCaptured > 1) continue;

          auto rgbBuffer = camera->frame().data();

          std::lock_guard<std::mutex> locked(m_img);
          img = QImage(rgbBuffer,camera->frame_head.iWidth,camera->frame_head.iHeight,camera->frame_head.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? QImage::Format::Format_Indexed8 : QImage::Format::Format_RGB888);

          auto lastFrameTime = QDateTime::currentDateTime().toMSecsSinceEpoch() - tick;
          displayFPS = 1000 / lastFrameTime;
          tick = QDateTime::currentDateTime().toMSecsSinceEpoch();

          if(QImage::Format::Format_Indexed8 == img.format()) {
              QVector<QRgb> grayColorTable;
              for(int i = 0; i < 256; i++) grayColorTable.append(qRgb(i, i, i));
              img.setColorTable(grayColorTable);
              img = img.convertToFormat(QImage::Format::Format_RGB16);
          }

          framesCaptured++;

          auto cs = dynamic_cast<CameraScene*>(scene());
          if(cs->deadPixelWindow) {
              for(auto pos : cs->manualPixels + cs->existedPixels)
                  img.setPixel(pos.x(),pos.y(), qRgb(255,255,0));
              for(auto pos : cs->deadPixels + cs->brightPixels)
                  img.setPixel(pos.x(),pos.y(), qRgb(0,255,0));
          }

          coordinate = QString("%1,%2").arg(cs->coordinate.x()).arg(cs->coordinate.y());
          if(img.valid(cs->coordinate.x(),cs->coordinate.y())) {
              rgb = img.pixel(cs->coordinate.x(),cs->coordinate.y());
              brightness = 0.299*rgb.red() + 0.587*rgb.green() + 0.114*rgb.blue();
          }

          emit updated(QPixmap::fromImage(img));
        }
    });
}

void CameraView::update(const QPixmap &img) {
    framesCaptured = --framesCaptured < 0 ? 0 : framesCaptured;

    auto cs = dynamic_cast<CameraScene*>(scene());
    cs->update(img);
    resetTransform();

    auto w=img.width();
    auto h=img.height();
    auto sw = width() / float(w);
    auto sh = height() / float(h);
    auto scaleValue = std::min(sw,sh) * currentScale;

    scale(scaleValue,scaleValue);

    if(cs->exposureWindow || cs->whiteBalanceWindow || cs->deadPixelWindow || cs->resolutionWindow)
        setDragMode(QGraphicsView::NoDrag);
    else
        setDragMode(QGraphicsView::ScrollHandDrag);
}

void CameraView::pause() {
    camera->pause();
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

    scene()->addPixmap(QPixmap(0,0));
}

void CameraView::on_pushButton_close_clicked()
{
    setParent(nullptr);
}

QImage CameraView::snapshot() {
    std::lock_guard<std::mutex> locked(m_img);
    auto temp = img.copy();
    return temp;
}
