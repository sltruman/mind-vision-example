#include "camerascene.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>

#include <iostream>
using namespace std;

CameraScene::CameraScene(QObject *parent) : QGraphicsScene(parent)
  , deadPixelWindow(false)
  , whiteBalanceWindow(false)
  , leftButtonPressed(false)
{
    background = addPixmap(QPixmap(0,0));
}

void CameraScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = true;

    if(whiteBalanceWindow) {
        whiteBalanceWindowPos.setLeft(event->scenePos().x());
        whiteBalanceWindowPos.setTop(event->scenePos().y());
    } else if(deadPixelWindow) {
        if(-1 == deadPixelPos.indexOf(QPoint(event->scenePos().x(),event->scenePos().y())))
            deadPixelPos.append(QPoint(event->scenePos().x(),event->scenePos().y()));
        else
            deadPixelPos.removeOne(QPoint(event->scenePos().x(),event->scenePos().y()));
    }
}

void CameraScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(whiteBalanceWindow && leftButtonPressed) {
        whiteBalanceWindowPos.setRight(event->scenePos().x());
        whiteBalanceWindowPos.setBottom(event->scenePos().y());
    }
}

void CameraScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = false;
}

void CameraScene::update(const QImage &rimg)
{
    auto img = const_cast<QImage&>(rimg);

    clear();

    if(deadPixelWindow) {
        for(auto pos : deadPixelPos)
            img.setPixel(pos.x(),pos.y(), qRgb(255,0,0));
    }

    auto w = img.width();
    auto h = img.height();

    setSceneRect(0,0,w,h);
    background = addPixmap(QPixmap::fromImage(img));

    for(auto line : lines) {
        auto x = std::get<0>(line);
        auto y = std::get<1>(line);
        auto pen = std::get<2>(line);
        addLine(x,0,x,h,pen);
        addLine(0,y,w,y,pen);
    }

    if(whiteBalanceWindow && whiteBalanceWindowPos.isValid()) {
        addRect(whiteBalanceWindowPos,QPen(QColor(Qt::red),4));
    }
}
