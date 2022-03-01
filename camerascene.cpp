#include "camerascene.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>

#include <iostream>
using namespace std;

CameraScene::CameraScene(QObject *parent) : QGraphicsScene(parent)
  , deadPixelWindow(false)
  , whiteBalanceWindow(false)
  , exposureWindow(false)
  , resolutionWindow(false)
  , leftButtonPressed(false)
{
    background = addPixmap(QPixmap(0,0));
}

void CameraScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = true;

    auto x = event->scenePos().x();
    auto y = event->scenePos().y();

    if(resolutionWindow) {
        resolutionWindowPos.setLeft(x < sceneRect().x() ? 0 : x);
        resolutionWindowPos.setTop(y < sceneRect().y() ? 0 : y);
        resolutionWindowPos.setRight(resolutionWindowPos.left());
        resolutionWindowPos.setBottom(resolutionWindowPos.top());
    } else if(whiteBalanceWindow) {
        whiteBalanceWindowPos.setLeft(x < sceneRect().x() ? 0 : x);
        whiteBalanceWindowPos.setTop(y < sceneRect().y() ? 0 : y);
        whiteBalanceWindowPos.setRight(whiteBalanceWindowPos.left());
        whiteBalanceWindowPos.setBottom(whiteBalanceWindowPos.top());
    } else if(exposureWindow) {
        exposureWindowPos.setLeft(x < sceneRect().x() ? 0 : x);
        exposureWindowPos.setTop(y < sceneRect().y() ? 0 : y);
        exposureWindowPos.setRight(exposureWindowPos.left());
        exposureWindowPos.setBottom(exposureWindowPos.top());
    } else if(deadPixelWindow) {
        if(-1 == manualPixels.indexOf(QPoint(event->scenePos().x(),event->scenePos().y())))
            manualPixels.append(QPoint(event->scenePos().x(),event->scenePos().y()));
        else
            manualPixels.removeOne(QPoint(event->scenePos().x(),event->scenePos().y()));
    }
}

void CameraScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    coordinate = event->scenePos().toPoint();
    auto x = event->scenePos().x();
    auto y = event->scenePos().y();

    if(resolutionWindow && leftButtonPressed) {
        resolutionWindowPos.setRight(x > sceneRect().width() ? sceneRect().width() : x);
        resolutionWindowPos.setBottom(y > sceneRect().height() ? sceneRect().height() : y);
    } else if(whiteBalanceWindow && leftButtonPressed) {
        whiteBalanceWindowPos.setRight(x > sceneRect().width() ? sceneRect().width() : x);
        whiteBalanceWindowPos.setBottom(y > sceneRect().height() ? sceneRect().height() : y);
    }else if(exposureWindow && leftButtonPressed) {
        exposureWindowPos.setRight(x > sceneRect().width() ? sceneRect().width() : x);
        exposureWindowPos.setBottom(y > sceneRect().height() ? sceneRect().height() : y);
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

    if(!exposureWindowPos.isValid()) {
        exposureWindowPos.setRect(0,0,sceneRect().width(),sceneRect().height());
    }
}

void CameraScene::update(const QPixmap &img)
{
    clear();

    auto w = img.width();
    auto h = img.height();

    setSceneRect(0,0,w,h);
    background = addPixmap(img);

    for(auto line : lines) {
        auto x = std::get<0>(line);
        auto y = std::get<1>(line);
        auto pen = std::get<2>(line);
        addLine(x,0,x,h,pen);
        addLine(0,y,w,y,pen);
    }

    if(exposureWindow) {
        addRect(exposureWindowPos,QPen(QColor(Qt::red),4));
    }

    if(whiteBalanceWindow) {
        addRect(whiteBalanceWindowPos,QPen(QColor(Qt::red),4));
    }

    if(resolutionWindow) {
        addRect(resolutionWindowPos,QPen(QColor(Qt::red),4));
    }
}
