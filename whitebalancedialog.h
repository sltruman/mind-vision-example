#ifndef WHITEBALANCEDIALOG_H
#define WHITEBALANCEDIALOG_H

#include <QDialog>

namespace Ui {
class WhiteBalanceDialog;
}

class WhiteBalanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WhiteBalanceDialog(QWidget *parent = nullptr);
    ~WhiteBalanceDialog();

private:
    Ui::WhiteBalanceDialog *ui;
};

#endif // WHITEBALANCEDIALOG_H
