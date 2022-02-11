#ifndef CAMERASCENE_H
#define CAMERASCENE_H

#include <QMouseEvent>
#include <QGraphicsView>
#include <QList>
#include <QPair>


class CameraScene : public QGraphicsScene
{
    Q_OBJECT
public:
    CameraScene(QObject *parent);
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    QPoint coordinate;

    bool resolutionWindow;
    QRect resolutionWindowPos;

    bool exposureWindow;
    QRect exposureWindowPos;

    bool whiteBalanceWindow;
    QRect whiteBalanceWindowPos;

    bool deadPixelWindow;
    QList<QPoint> existedPixels,manualPixels,deadPixels,brightPixels;

    QMap<int,std::tuple<int,int,QPen>> lines;
    QGraphicsPixmapItem* background;

    void update(const QImage &img);

    bool leftButtonPressed;
};

#endif // CAMERASCENE_H
