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
    if(!deviceItem || !deviceItem->camera->opened()) return;

    auto index = ui->comboBox_params->currentIndex();

    deviceItem->camera->params_save(index);
    emit save_clicked();
}

void RightSideTitleBar::on_toolButton_default_clicked()
{
    auto deviceItem = dynamic_cast<DeviceItem*>(tw->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->params_reset();
    emit default_clicked();
}

void RightSideTitleBar::on_comboBox_params_activated(int index)
{
    auto deviceItem = dynamic_cast<DeviceItem*>(tw->currentItem());
    if(!deviceItem || !deviceItem->camera->opened()) return;

    deviceItem->camera->params_load(index);
    emit params_activated();
}
