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

    qApp->setStyleSheet(css);

    MainWindow w;
    w.show();
    return a.exec();
}
