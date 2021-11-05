#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QGraphicsView>
#include <QProcess>
#include <QLocalSocket>
#include <QTimer>

class CameraView : public QGraphicsView
{
    Q_OBJECT
public:
    CameraView(QWidget *parent,QProcess* camera);
    ~CameraView();

    void play(QString pipeName);
    void stop();
    bool playing();

    QProcess* camera;
    QGraphicsPixmapItem* background;

private slots:
    void process();

protected:
    void paintEvent(QPaintEvent *event);

    QLocalSocket sock;
    QTimer t;
};

#endif // CAMERAVIEW_H
