#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QMouseEvent>

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
    void on_pushButton_imageSetting_clicked();

private:
    Ui::MainMenu *ui;
};

#endif // MAINMENU_H
