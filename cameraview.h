#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QMap>
#include <QGraphicsView>
#include <QProcess>
#include <QLocalSocket>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <tuple>

namespace Ui {
class CameraView;
}

class CameraView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CameraView(QWidget *parent = nullptr);
    ~CameraView();

    void play();
    void pause();
    void stop();
    bool playing;

    QProcess* camera;
    QString pipeName;
    QGraphicsPixmapItem* background;
    float currentScale;
    int displayFPS;
    unsigned long long frames;
    QMap<int,std::tuple<int,int,QPen>> lines;

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void process();
    void on_pushButton_close_clicked();

private:
    Ui::CameraView *ui;
    QLocalSocket sock;
    QTimer t;
    QByteArray rgbBuffer;
};

#endif // CAMERAVIEW_H
