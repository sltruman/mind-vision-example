#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QGraphicsView>
#include <QProcess>
#include <QLocalSocket>
#include <QTimer>
#include <QGraphicsPixmapItem>

class CameraView : public QGraphicsView
{
    Q_OBJECT
public:
    CameraView(QProcess* camera,QWidget *parent=nullptr);
    ~CameraView();

    void play(QString pipeName);
    void stop();
    bool playing();

    QProcess* camera;
    QGraphicsPixmapItem* background;
    float currentScale;
    int displayFPS;
    unsigned long long frames;

private slots:
    void process();

protected:
    void paintEvent(QPaintEvent *event);

    QLocalSocket sock;
    QTimer t;
};

#endif // CAMERAVIEW_H
