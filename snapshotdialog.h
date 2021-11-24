#ifndef SNAPSHOTDIALOG_H
#define SNAPSHOTDIALOG_H

#include <QDialog>

namespace Ui {
class SnapshotDialog;
}

class SnapshotDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SnapshotDialog(QWidget *parent = nullptr);
    ~SnapshotDialog();

    void resolutions(QStringList);
    int resolution();
    QString dir();
    int period();
    int format();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_checkBox_period_stateChanged(int arg1);

private:
    Ui::SnapshotDialog *ui;
};

#endif // SNAPSHOTDIALOG_H
