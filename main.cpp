#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss("theme/black.css");
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();

    QTranslator qtTranslator;
    qtTranslator.load("language/app_zh.qm");
    a.installTranslator(&qtTranslator);

    MainWindow w;
    w.show();
    return a.exec();
}
