#include "mainmenu.h"
#include "ui_mainmenu.h"
#include <QMenu>

MainMenu::MainMenu(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainMenu)
{
    ui->setupUi(this);
    ui->pushButton_close->hide();
    ui->pushButton_minimum->hide();
    ui->pushButton_maximum->hide();
}

MainMenu::~MainMenu()
{
    delete ui;
}

void MainMenu::on_pushButton_close_clicked()
{
    parentWidget()->close();
}

void MainMenu::on_pushButton_imageSetting_clicked()
{
    auto qMenu = new QMenu(ui->pushButton_imageSetting);
    qMenu->addAction(ui->action_takingSetting);
    qMenu->addAction(ui->action_recordingSetting);
    qMenu->exec(QCursor::pos()); //在鼠标点击的位置显示鼠标右键菜单
}
