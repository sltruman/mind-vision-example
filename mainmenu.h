#ifndef MAINMENU_H
#define MAINMENU_H

#include "snapshotdialog.h"
#include "aboutdialog.h"
#include "recorddialog.h"

#include <QWidget>
#include <QMouseEvent>
#include <QSettings>
#include <iostream>

namespace Ui {
class MainMenu;
}

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);
    ~MainMenu();

    SnapshotDialog snapshotDialog;
    RecordDialog recordDialog;
    AboutDialog dialog;
private slots:
    void on_pushButton_close_clicked();
    void on_pushButton_language_clicked(bool checked);
    void on_action_ipConfiguration_triggered();
    void on_action_demo_triggered();
    void on_action_sdk_triggered();
    void on_action_log_triggered();
    void on_action_about_triggered();
    void on_action_snapshotSetting_triggered();
    void on_action_recordingSetting_triggered();
    void on_pushButton_minimum_clicked();
    void on_pushButton_maximum_clicked(bool checked);
private:
    Ui::MainMenu *ui;
    QSettings settings;
};

#endif // MAINMENU_H
