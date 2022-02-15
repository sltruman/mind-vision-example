#include "loadingdialog.h"
#include "ui_loadingdialog.h"

LoadingDialog::LoadingDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadingDialog)
{
    ui->setupUi(this);
}

LoadingDialog::~LoadingDialog()
{
    delete ui;
}
