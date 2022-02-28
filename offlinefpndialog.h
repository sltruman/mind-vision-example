#ifndef OFFLINEFPNDIALOG_H
#define OFFLINEFPNDIALOG_H

#include <QDialog>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QResizeEvent>
#include <QAbstractGraphicsShapeItem>

class OriginScene : public QGraphicsScene
{
    Q_OBJECT
public:
    OriginScene(QObject *parent);
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    bool whiteBalanceWindow;
    QRect whiteBalanceWindowPos;
    QGraphicsPixmapItem* background;
    QGraphicsRectItem* foreground;
    bool leftButtonPressed;
};


namespace Ui {
class OfflineFpnDialog;
}

class OfflineFpnDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfflineFpnDialog(QWidget *parent,QImage img);
    ~OfflineFpnDialog();

    QString filename;
    QString fpnfilepath;
    QString wbfilepath;
protected:
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;

private slots:
    void on_pushButton_load_clicked();

    void on_pushButton_calculate_clicked();

    void on_pushButton_apply_clicked();

private:
    Ui::OfflineFpnDialog *ui;
};

#endif // OFFLINEFPNDIALOG_H
