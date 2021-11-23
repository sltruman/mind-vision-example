﻿#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QMouseEvent>
#include <QSettings>

#include <iostream>
using namespace std;

namespace Ui {
class MainMenu;
}

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);
    ~MainMenu();


private slots:
    void on_pushButton_close_clicked();

    void on_pushButton_language_clicked(bool checked);



private:
    Ui::MainMenu *ui;
    QSettings settings;
};

#endif // MAINMENU_H
