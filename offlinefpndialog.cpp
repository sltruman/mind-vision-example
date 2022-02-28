#include "offlinefpndialog.h"
#include "ui_offlinefpndialog.h"
#include <QFileDialog>
#include <QProcess>
#include <QStandardPaths>
#include <QMessageBox>

OriginScene::OriginScene(QObject *parent) : QGraphicsScene(parent)
  , whiteBalanceWindow(false)
  , leftButtonPressed(false)
  , background(nullptr)
  , foreground(nullptr)
{}

void OriginScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = true;

    auto x = event->scenePos().x();
    auto y = event->scenePos().y();

    if(whiteBalanceWindow) {
        whiteBalanceWindowPos.setLeft(x < sceneRect().x() ? 0 : x);
        whiteBalanceWindowPos.setTop(y < sceneRect().y() ? 0 : y);
        whiteBalanceWindowPos.setRight(whiteBalanceWindowPos.left());
        whiteBalanceWindowPos.setBottom(whiteBalanceWindowPos.top());
    }
}

void OriginScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    auto x = event->scenePos().x();
    auto y = event->scenePos().y();

    if(whiteBalanceWindow && leftButtonPressed) {
        whiteBalanceWindowPos.setRight(x > sceneRect().width() ? sceneRect().width() : x);
        whiteBalanceWindowPos.setBottom(y > sceneRect().height() ? sceneRect().height() : y);
    }
}

void OriginScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    leftButtonPressed = false;

    if(!whiteBalanceWindowPos.isValid()) {
        whiteBalanceWindowPos.setRect(0,0,sceneRect().width(),sceneRect().height());
    }
}

OfflineFpnDialog::OfflineFpnDialog(QWidget *parent,QImage img) :
    QDialog(parent),
    ui(new Ui::OfflineFpnDialog)
{
    ui->setupUi(this);
    filename = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/mv-snapshot.jpg";
    fpnfilepath = filename+".fpn";

    QFile::remove(fpnfilepath);
    QFile::remove(filename+".fpn.jpg");
    QFile::remove(filename+".wb.jpg");

    auto scene = new OriginScene(this);
    scene->whiteBalanceWindow = true;
    auto w = img.width();
    auto h = img.height();
    scene->background = scene->addPixmap(QPixmap::fromImage(img));
    ui->graphicsView_origin->setScene(scene);
    ui->graphicsView_origin->setSceneRect(0,0,w,h);

    ui->graphicsView_fpn->setScene(new OriginScene(this));
    ui->graphicsView_wb->setScene(new OriginScene(this));

    QApplication::postEvent(this,new QResizeEvent(QSize(),QSize()));
}

OfflineFpnDialog::~OfflineFpnDialog()
{
    delete ui;
}

void OfflineFpnDialog::resizeEvent(QResizeEvent *) {
    update();
}

void OfflineFpnDialog::paintEvent(QPaintEvent *e)  {
    QDialog::paintEvent(e);

    OriginScene* scene = nullptr;
    QGraphicsView* view = nullptr;

    switch(ui->tabWidget->currentIndex()) {
    case 0:
        view = ui->graphicsView_origin;
        scene = dynamic_cast<OriginScene*>(ui->graphicsView_origin->scene());
        break;
    case 1:
        view = ui->graphicsView_fpn;
        scene = dynamic_cast<OriginScene*>(ui->graphicsView_fpn->scene());
        break;
    case 2:
        view = ui->graphicsView_wb;
        scene = dynamic_cast<OriginScene*>(ui->graphicsView_wb->scene());
        break;
    }

    if(scene->foreground != nullptr) scene->removeItem(scene->foreground);
    if(scene->whiteBalanceWindow) scene->foreground = scene->addRect(scene->whiteBalanceWindowPos,QPen(QColor(Qt::red),4));

    auto w = view->sceneRect().width();
    auto h = view->sceneRect().height();

    view->resetTransform();

    auto sw = view->width() / float(w) * 0.99f;
    auto sh = view->height() / float(h) * 0.99f;
    auto scaleValue = std::min(sw,sh);
    view->scale(scaleValue,scaleValue);

    update();
}

void OfflineFpnDialog::on_pushButton_load_clicked()
{
    auto filepath = QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("Image Files(*.jpg *.bmp *.png)"));
    if(filepath.isEmpty()) return;
    filename = filepath;

    QImage img(filename);

    auto w = img.width();
    auto h = img.height();
    ui->graphicsView_origin->setSceneRect(0,0,w,h);

    auto scene = dynamic_cast<OriginScene*>(ui->graphicsView_origin->scene());
    scene->clear();
    scene->background = scene->addPixmap(QPixmap::fromImage(img));
    update();
}

#include <QDebug>

void OfflineFpnDialog::on_pushButton_calculate_clicked()
{
    auto scene = dynamic_cast<OriginScene*>(ui->graphicsView_origin->scene());
    qDebug() << scene->background->pixmap().width();
    if(scene->background->pixmap().width() != 4096) {
        QMessageBox::critical(nullptr,tr("Offline FPN"), tr("Image width must be 4076px!"));

        return;
    }

    if(scene->whiteBalanceWindowPos.isValid()) {
        auto x = scene->whiteBalanceWindowPos.x();
        auto y = scene->whiteBalanceWindowPos.y();
        auto w = scene->whiteBalanceWindowPos.width();
        auto h = scene->whiteBalanceWindowPos.height();
        scene->background->pixmap().copy(x,y,w,h).save(filename);
    } else {
        scene->background->pixmap().save(filename);
    }

    auto exe = QApplication::applicationDirPath() + "/modules/fpn";
    auto exitCode = QProcess::execute(exe,{filename,QString::number(ui->comboBox->currentIndex() + 1)});
    qDebug() << exitCode << ' ' << exe << ' ' << filename << ' ' << QString::number(ui->comboBox->currentIndex() + 1);

    QImage fpnImg(filename+".fpn.jpg");
    ui->graphicsView_fpn->scene()->addPixmap(QPixmap::fromImage(fpnImg));
    ui->graphicsView_fpn->setSceneRect(0,0,fpnImg.width(),fpnImg.height());
    QImage wbImg(filename+".wb.jpg");
    ui->graphicsView_wb->scene()->addPixmap(QPixmap::fromImage(wbImg));
    ui->graphicsView_wb->setSceneRect(0,0,wbImg.width(),wbImg.height());
}

void OfflineFpnDialog::on_pushButton_apply_clicked()
{
    QFile fpn(fpnfilepath);
    if(fpn.exists()) {
        accept();
    } else {
        QMessageBox::critical(nullptr,tr("Offline FPN"), tr("Nothing to apply!"));
    }
}

