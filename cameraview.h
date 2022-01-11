#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include "camerascene.h"

#include <tuple>
#include <thread>

#include <QMap>
#include <QGraphicsView>
#include <QProcess>
#include <QLocalSocket>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QKeyEvent>

namespace Ui {
class CameraView;
}

class CameraView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CameraView(QWidget *parent = nullptr);
    ~CameraView();

    QProcess* camera;

    void zoom(float);
    void play();
    void pause();
    void stop();
    bool playing,interupt;

    QString pipeName;
    float currentScale;
    int displayFPS;
    unsigned long long frames;
    QString coordinate;
    bool leftButtonPressed;
    QImage img;
protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

Q_SIGNALS:
    void updated(const QImage &img);

private slots:
    void update(const QImage &img);
    void on_pushButton_close_clicked();

private:
    Ui::CameraView *ui;
    std::thread task;
    qint64 tick;
    QByteArray rgbBuffer;
};

#endif // CAMERAVIEW_H
