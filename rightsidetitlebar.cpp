#include "rightsidetitlebar.h"
#include "ui_rightsidetitlebar.h"
#include "toplevelitemwidget.h"

RightSideTitleBar::RightSideTitleBar(QWidget *parent,QTreeWidget* tw) :
    QWidget(parent),
    ui(new Ui::RightSideTitleBar),
    tw(tw)
{
    ui->setupUi(this);
}

RightSideTitleBar::~RightSideTitleBar()
{
    delete ui;
}

void RightSideTitleBar::on_toolButton_save_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(tw->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    auto index = ui->comboBox_params->currentIndex();

    deviceItem->paramsSave(index);
    emit save_clicked();
}

void RightSideTitleBar::on_toolButton_default_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(tw->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->paramsReset();
    emit default_clicked();
}

void RightSideTitleBar::on_comboBox_params_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(tw->currentItem());
    if(!deviceItem || QProcess::NotRunning == deviceItem->camera.state()) return;

    deviceItem->paramsLoad(index);
    emit params_activated();
}
