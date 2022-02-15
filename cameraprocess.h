#ifndef CAMERAPROCESS_H
#define CAMERAPROCESS_H

#include <QProcess>

class CameraProcess : public QProcess
{
    Q_OBJECT

public:
    CameraProcess();
//    qint64 write(const char *data);
//    QByteArray readLine();
//    QByteArray readAll();

    int n;
    QByteArrayList responses;
};

#endif // CAMERAPROCESS_H
