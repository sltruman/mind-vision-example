#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTranslator>
#include <QSettings>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSettings settings("MindVision","Example");
    QTranslator qtTranslator;

    if("zh" == settings.value("language","zh"))
        qtTranslator.load("language/app_zh.qm");

    a.installTranslator(&qtTranslator);

    QString css;

    QFile cssMainWindow("theme/black/mainwindow.css");
    cssMainWindow.open(QFile::ReadOnly);
    css += cssMainWindow.readAll();
    cssMainWindow.close();

    QFile cssMenubar("theme/black/menubar.css");
    cssMenubar.open(QFile::ReadOnly);
    css += cssMenubar.readAll();
    cssMenubar.close();

    QFile cssPreview("theme/black/preview.css");
    cssPreview.open(QFile::ReadOnly);
    css += cssPreview.readAll();
    cssPreview.close();

    QFile cssLeftSide("theme/black/leftside.css");
    cssLeftSide.open(QFile::ReadOnly);
    css += cssLeftSide.readAll();
    cssLeftSide.close();

    QFile cssRightSide("theme/black/rightside.css");
    cssRightSide.open(QFile::ReadOnly);
    css += cssRightSide.readAll();
    cssRightSide.close();

    QFile cssSnapshot("theme/black/snapshot.css");
    cssSnapshot.open(QFile::ReadOnly);
    css += cssSnapshot.readAll();
    cssSnapshot.close();

    QFile cssRecord("theme/black/record.css");
    cssRecord.open(QFile::ReadOnly);
    css += cssRecord.readAll();
    cssRecord.close();

    QFile cssCalibration("theme/black/calibration.css");
    cssCalibration.open(QFile::ReadOnly);
    css += cssCalibration.readAll();
    cssCalibration.close();

    QFile cssOfflineFpn("theme/black/offlinefpn.css");
    cssOfflineFpn.open(QFile::ReadOnly);
    css += cssOfflineFpn.readAll();
    cssOfflineFpn.close();

    qApp->setStyleSheet(css);

    MainWindow w;
    w.show();
    return a.exec();
}
