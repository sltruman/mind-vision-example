#include "mainmenu.h"
#include "ui_mainmenu.h"
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProcess>
#include "aboutdialog.h"

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
//    menubar->addMenu(file);

    auto preview = new QMenu(tr("Preview"),menubar);
//    menubar->addMenu(preview);

    auto image = new QMenu(tr("Images"),menubar);
    image->addAction(ui->action_snapshotSetting);
    image->addAction(ui->action_recordingSetting);
    menubar->addMenu(image);

    auto software = new QMenu(tr("Softwares"),menubar);
//    menubar->addMenu(software);

    auto tools= new QMenu(tr("Tools"),menubar);
    tools->addAction(ui->action_ipConfiguration);
    tools->addAction(ui->action_log);
    menubar->addMenu(tools);

    auto helper= new QMenu(tr("Helper"),menubar);
    helper->addAction(ui->action_about);
    helper->addAction(ui->action_sdk);
    helper->addAction(ui->action_demo);
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

void MainMenu::on_action_ipConfiguration_triggered()
{
    QProcess::execute("C:/Users/SLTru/Desktop/mind-vision-0.0.4/Tools/网口相机IP配置工具");
}

void MainMenu::on_action_demo_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///C:/Program Files/MindVision/Demo", QUrl::TolerantMode));
}

void MainMenu::on_action_sdk_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///C:/Program Files/MindVision/Document", QUrl::TolerantMode));
}

void MainMenu::on_action_log_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///C:/Program Files/MindVision/Camera/log", QUrl::TolerantMode));
}

void MainMenu::on_action_about_triggered()
{
    AboutDialog dialog;
    dialog.exec();
}

void MainMenu::on_action_snapshotSetting_triggered()
{

}

void MainMenu::on_action_recordingSetting_triggered()
{

}
