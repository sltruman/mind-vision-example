#include "snapshotdialog.h"
#include "ui_snapshotdialog.h"

#include <QStandardPaths>
#include <QFileDialog>

SnapshotDialog::SnapshotDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SnapshotDialog)
{
    ui->setupUi(this);
    auto imageDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    ui->edit_dir->setText(imageDir);
}

SnapshotDialog::~SnapshotDialog()
{
    delete ui;
}

int SnapshotDialog::format() {
    auto format = ui->comboBox_format->currentText();
    if(format == "JPEG")
        return 1;
    else if(format == "BMP")
        return 2;
    else if(format == "PNG")
        return 8;
    else if(format == "RAW")
        return 4;

    throw std::runtime_error("");
}

void SnapshotDialog::resolutions(QStringList items) {
    ui->comboBox_resolution->clear();
    ui->comboBox_resolution->addItems(items);
}

int SnapshotDialog::resolution() {
    return ui->comboBox_resolution->currentIndex();
}

QString SnapshotDialog::dir() {
    return ui->edit_dir->text();
}

int SnapshotDialog::period() {
    if(ui->spinBox_period->isEnabled())
        return ui->spinBox_period->value();
    return -1;
}

void SnapshotDialog::on_buttonBox_accepted()
{
    accept();
}

void SnapshotDialog::on_buttonBox_rejected()
{
    reject();
}

void SnapshotDialog::on_checkBox_period_stateChanged(int enable)
{
    ui->spinBox_period->setEnabled(enable);
}

void SnapshotDialog::on_pushButton_snap_path_clicked()
{
    ui->edit_dir->setText(QFileDialog::getExistingDirectory(this,tr("Save Path"),ui->edit_dir->text()));
}
