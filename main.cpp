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
    else
        qtTranslator.load("language/app_en.qm");

    a.installTranslator(&qtTranslator);

    QString css;

    QFile cssOther("theme/black/black.css");
    cssOther.open(QFile::ReadOnly);
    css += cssOther.readAll();
    cssOther.close();

    QFile cssMenubar("theme/black/menubar.css");
    cssMenubar.open(QFile::ReadOnly);
    css += cssMenubar.readAll();
    cssMenubar.close();

    qApp->setStyleSheet(css);

    MainWindow w;
    w.show();
    return a.exec();
}
