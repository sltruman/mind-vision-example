#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <opencv2/imgproc/types_c.h>
#include <opencv2/opencv.hpp>
using namespace cv;

#include <vector>
#include <memory>

#include <QDialog>
#include <QProcess>
#include <QListWidgetItem>

namespace Ui {
class CalibrationDialog;
}

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    class Board;
    class Result;
    enum Pattern { NOT_EXISTING, CHESSBOARD, CIRCLES_GRID, ASYMMETRIC_CIRCLES_GRID };

    CalibrationDialog(QWidget *parent = nullptr);
    ~CalibrationDialog();
    int GetMarkWidth();
    int GetMarkHeight();
    bool FindBoardMark(int index);
    bool SetCurSelBoard(int index);
    void AddBoard(Mat const& img);
    bool runCalibration(int& imageCount, Pattern calibrationPattern = CHESSBOARD, bool fisheye = false);
    bool UpdateCalibResult(bool toUI);
    void EnableManualInput(bool bEnable);
    void ReadParamsFromCamera();
    void DrawBoard(int index);
    void DrawCurrentBoard();
    void RemoveBoard(int index);
    int width();
    int height();
    QString cameraMatraix();
    QString distortCoeffs();

    std::vector<Board*> m_Boards;
    std::auto_ptr<Result> m_Result;
    QProcess* camera;

private slots:
    void on_pushButton_load_clicked();

    void on_pushButton_calibrate_clicked();

    void on_listWidget_calibrations_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_listWidget_calibrations_customContextMenuRequested(const QPoint &pos);

    void on_actionRemove_triggered();

    void on_actionClear_triggered();

    void on_pushButton_applyCalibration_clicked();

private:
    Ui::CalibrationDialog *ui;
};

#endif // CALIBRATIONDIALOG_H
