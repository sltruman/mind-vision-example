#include "recorddialog.h"
#include "ui_recorddialog.h"

#include <QStandardPaths>
#include <QFileDialog>
#include <stdexcept>

RecordDialog::RecordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RecordDialog)
{
    ui->setupUi(this);
    auto dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation).replace('/','\\');
    ui->edit_dir->setText(dir);
}

RecordDialog::~RecordDialog()
{
    delete ui;
}

QString RecordDialog::dir() {
    return ui->edit_dir->text();
}

int RecordDialog::format() {
    auto format = ui->comboBox_format->currentText();
    if(format == "None")
        return 0;
    else if(format == "Microsoft Video Compress")
        return 1;
    else if(format == "DivX")
        return 3;
    else if(format == "H264")
        return 4;

    throw std::runtime_error("");
}

int RecordDialog::quality() {
    return ui->spinBox_quality->value();
}

int RecordDialog::frames() {
    return ui->spinBox_frames->value();
}

void RecordDialog::on_buttonBox_accepted()
{
    accept();
}

void RecordDialog::on_buttonBox_rejected()
{
    reject();
}

void RecordDialog::on_pushButton_snap_path_clicked()
{
    ui->edit_dir->setText(QFileDialog::getExistingDirectory(this,tr("Save Path"),ui->edit_dir->text()).replace('/','\\'));
}
