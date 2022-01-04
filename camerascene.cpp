#include "camerascene.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>

#include <iostream>
using namespace std;

CameraScene::CameraScene(QObject *parent) : QGraphicsScene(parent)
  , deadPixelWindow(false)
  , whiteBalanceWindow(false)
  , resolutionWindow(false)
  , leftButtonPressed(false)
{
    background = addPixmap(QPixmap(0,0));
}

void CameraScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = true;

    if(resolutionWindow) {
        resolutionWindowPos.setLeft(event->scenePos().x());
        resolutionWindowPos.setTop(event->scenePos().y());
    } else if(whiteBalanceWindow) {
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
    coordinate = event->scenePos().toPoint();

    if(resolutionWindow && leftButtonPressed) {
        resolutionWindowPos.setRight(event->scenePos().x());
        resolutionWindowPos.setBottom(event->scenePos().y());
    } else if(whiteBalanceWindow && leftButtonPressed) {
        whiteBalanceWindowPos.setRight(event->scenePos().x());
        whiteBalanceWindowPos.setBottom(event->scenePos().y());
    }
}

void CameraScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = false;

    if(!resolutionWindowPos.isValid()) {
        resolutionWindowPos.setRect(0,0,sceneRect().width(),sceneRect().height());
    }

    if(!whiteBalanceWindowPos.isValid()) {
        whiteBalanceWindowPos.setRect(0,0,sceneRect().width(),sceneRect().height());
    }
}

void CameraScene::update(int x,int y,const QImage &rimg)
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

    if(whiteBalanceWindow) {
        addRect(whiteBalanceWindowPos,QPen(QColor(Qt::red),4));
    }

    if(resolutionWindow) {
        addRect(resolutionWindowPos,QPen(QColor(Qt::red),4));
    }
}
