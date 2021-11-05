#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss("/home/sl.truman/Desktop/mind-vision-example/theme/black.css");
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();

    QTranslator qtTranslator;
    qtTranslator.load("/home/sl.truman/Desktop/mind-vision-example/language/app_zh.qm");
    a.installTranslator(&qtTranslator);

    MainWindow w;
    w.show();
    return a.exec();
}
