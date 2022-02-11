#ifndef RIGHTSIDETITLEBAR_H
#define RIGHTSIDETITLEBAR_H

#include <QWidget>
#include <QTreeWidget>

namespace Ui {
class RightSideTitleBar;
}

class RightSideTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit RightSideTitleBar(QWidget *parent,QTreeWidget* tw);
    ~RightSideTitleBar();

signals:
    void save_clicked();
    void default_clicked();
    void params_activated();

private slots:
    void on_toolButton_save_clicked();

    void on_toolButton_default_clicked();

    void on_comboBox_params_activated(int index);
private:
    Ui::RightSideTitleBar *ui;
    QTreeWidget* tw;
};

#endif // RIGHTSIDETITLEBAR_H
