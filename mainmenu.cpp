#include "mainmenu.h"
#include "ui_mainmenu.h"
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QMessageBox>


MainMenu::MainMenu(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainMenu),
    settings("MindVision","Example")
{
    ui->setupUi(this);
    ui->pushButton_close->hide();
    ui->pushButton_minimum->hide();
    ui->pushButton_maximum->hide();

    auto menubar = new QMenuBar(ui->widget_mainMenu);
    auto file = new QMenu(tr("Files"),menubar);
    menubar->addMenu(file);

    auto preview = new QMenu(tr("Preview"),menubar);
    menubar->addMenu(preview);

    auto image = new QMenu(tr("Images"),menubar);
    image->addAction(ui->action_takingSetting);
    image->addAction(ui->action_recordingSetting);
    menubar->addMenu(image);

    auto software = new QMenu(tr("Softwares"),menubar);
    menubar->addMenu(software);

    auto tools= new QMenu(tr("Tools"),menubar);
    menubar->addMenu(tools);

    auto helper= new QMenu(tr("Helper"),menubar);
    menubar->addMenu(helper);

    ui->horizontalLayout->addWidget(menubar);
    ui->pushButton_language->setChecked("zh" == settings.value("language","zh"));
}

MainMenu::~MainMenu()
{
    delete ui;
}

void MainMenu::on_pushButton_close_clicked()
{
    parentWidget()->close();
}

void MainMenu::on_pushButton_language_clicked(bool checked)
{
    cout << "language " << (checked ? "zh" : "en") << endl;
    settings.setValue("language",(checked ? "zh" : "en"));
    QMessageBox::information(this, tr("Luanguage"), tr("Luanguage has been switched and to restart the application!"), QMessageBox::Ok);
    qApp->exit();
}


void MainMenu::on_action_takingSetting_triggered()
{

}

