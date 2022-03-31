#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include "camerascene.h"
#include "wrap/device.hpp"
#include "wrap/gf120.hpp"

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

    std::shared_ptr<Device> camera;

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
    void updated(const QPixmap &img);

private slots:
    void update(const QPixmap &img);
    void on_pushButton_close_clicked();

private:
    Ui::CameraView *ui;
    std::thread task;
    int framesCaptured;

    QTreeWidgetItem* owner;
};

#endif // CAMERAVIEW_H
