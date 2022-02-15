#include "cameraprocess.h"
#include <QApplication>

CameraProcess::CameraProcess() : n(0)
{
//    connect(this,&CameraProcess::readyRead,[=](){
//        if(!canReadLine()) return;

//        auto s = QProcess::readAll();
//        responses.push_back(s);
//    });
//    n++;
}

//qint64 CameraProcess::write(const char *data) {
//    n++;
//    return QProcess::write(data);
//}

//QByteArray CameraProcess::readLine() {
//    while(n == 0 || responses.size() < n)
//        QApplication::processEvents();

//    n--;
//    return responses.takeLast();
//}

//QByteArray CameraProcess::readAll() {
//    while(n == 0 || responses.size() < n)
//        QApplication::processEvents();

//    n--;
//    return responses.takeLast();
//}
