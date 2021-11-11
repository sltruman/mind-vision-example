#include "whitebalancedialog.h"
#include "ui_whitebalancedialog.h"

WhiteBalanceDialog::WhiteBalanceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WhiteBalanceDialog)
{
    ui->setupUi(this);
}

WhiteBalanceDialog::~WhiteBalanceDialog()
{
    delete ui;
}
