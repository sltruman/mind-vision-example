#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include "camerascene.h"

#include <tuple>
#include <thread>
#include <mutex>

#include <QMap>
#include <QGraphicsView>
#include <QProcess>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTreeWidgetItem>
#include <QSharedMemory>

namespace Ui {
class CameraView;
}

class CameraView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CameraView(QTreeWidgetItem* owner, QWidget *parent = nullptr);
    ~CameraView();
    void paintEvent(QPaintEvent *event) override;

    QProcess* camera;

    void zoom(float);
    void play();
    void pause();
    void stop();
    bool playing,interupt;

    QString pipeName;
    float currentScale;
    unsigned int displayFPS;
    QString coordinate;

    QColor rgb;
    int brightness;

    bool avgBrightness;

    bool leftButtonPressed;
    QImage img;
    std::mutex m_img;
    QImage snapshot();

    struct FrameHead {
        int num = 0;
        int width = 0;
        int height = 0;
        int bits = 0;
        char camera_status[256];
        char exposure_status[256];
        int record_status = 0;
        int snapshot_status = 0;
        char _[8];
    } current_frame_head;

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

Q_SIGNALS:
    void doubleClick();
    void focused();
    void updated(const QImage &img);

private slots:
    void update(const QImage &img);
    void on_pushButton_close_clicked();

private:
    Ui::CameraView *ui;
    std::thread task;
    int framesCaptured;
    qint64 tick;

    QSharedMemory sm;
    QTreeWidgetItem* owner;
};

#endif // CAMERAVIEW_H
