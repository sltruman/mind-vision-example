#include "calibrationdialog.h"
#include "ui_calibrationdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>

void DrawMat(cv::Mat const& img, QGraphicsView* view)
{
    cv::Mat imgTmp;
    cv::resize(img, imgTmp, cv::Size(img.cols,img.rows));// 缩放Mat并备份
    // 转一下格式 ,这段可以放外面,
    switch (imgTmp.channels())
    {
    case 1:
        cv::cvtColor(imgTmp, imgTmp, CV_GRAY2RGB); // GRAY单通道
        break;
    case 3:
        cv::cvtColor(imgTmp, imgTmp, CV_BGR2RGB);  // BGR三通道
        break;
    default:
        return;
    }

    QImage frame(imgTmp.data,imgTmp.cols,imgTmp.rows,QImage::Format_RGB888);

    view->scene()->clear();
    view->scene()->addPixmap(QPixmap::fromImage(frame));

    auto w=imgTmp.cols;
    auto h=imgTmp.rows;
    view->resetTransform();

    auto sw = view->width() / float(w);
    auto sh = view->height() / float(h);
    auto scaleValue = std::min(sw,sh);

    view->scale(scaleValue,scaleValue);
}

class CalibrationDialog::Result
{
public:

    Result(int width, int height,
        cv::Mat M = Mat::eye(3, 3, CV_64F), cv::Mat K = Mat::zeros(8, 1, CV_64F),
        double totalAvgErr = 0)
    {
        m_width = width;
        m_height = height;
        m_cameraMatrix = M;
        m_distCoeffs = K;
        m_totalAvgErr = totalAvgErr;
    }

    double fx() const { return m_cameraMatrix.at<double>(0, 0); }
    double fy() const { return m_cameraMatrix.at<double>(1, 1); }
    double cx() const { return m_cameraMatrix.at<double>(0, 2); }
    double cy() const { return m_cameraMatrix.at<double>(1, 2); }
    double k1() const { return m_distCoeffs.at<double>(0, 0); }
    double k2() const { return m_distCoeffs.at<double>(1, 0); }
    double p1() const { return m_distCoeffs.at<double>(2, 0); }
    double p2() const { return m_distCoeffs.at<double>(3, 0); }
    double k3() const { return m_distCoeffs.at<double>(4, 0); }
    double err() const { return m_totalAvgErr; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }

    void fx(double v) { m_cameraMatrix.at<double>(0, 0) = v; }
    void fy(double v) { m_cameraMatrix.at<double>(1, 1) = v; }
    void cx(double v) { m_cameraMatrix.at<double>(0, 2) = v; }
    void cy(double v) { m_cameraMatrix.at<double>(1, 2) = v; }
    void k1(double v) { m_distCoeffs.at<double>(0, 0) = v; }
    void k2(double v) { m_distCoeffs.at<double>(1, 0) = v; }
    void p1(double v) { m_distCoeffs.at<double>(2, 0) = v; }
    void p2(double v) { m_distCoeffs.at<double>(3, 0) = v; }
    void k3(double v) { m_distCoeffs.at<double>(4, 0) = v; }

    bool Check() const
    {
        if (m_width <= 0
            || m_height <= 0)
            return false;
        if (m_cameraMatrix.rows != 3
            || m_cameraMatrix.cols != 3)
            return false;
        if (!checkRange(m_cameraMatrix) )
            return false;
        if (m_cameraMatrix.at<double>(0, 1) != 0
            || m_cameraMatrix.at<double>(1, 0) != 0
            || m_cameraMatrix.at<double>(2, 0) != 0
            || m_cameraMatrix.at<double>(2, 1) != 0
            || m_cameraMatrix.at<double>(2, 2) != 1)
            return false;
        if (m_cameraMatrix.at<double>(0, 0) == 0
            || m_cameraMatrix.at<double>(1, 1) == 0)
            return false;

        return true;
    }

    bool UndistImage(cv::Mat Image, cv::Mat& OutImage)
    {
        cv::Size imageSize(m_width, m_height);
        if (Image.size() != imageSize)
            return false;

        if (m_map1.empty())
        {
            CreateUndistMap();
        }

        cv::remap(Image, OutImage, m_map1, m_map2, cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        return true;
    }

private:

    void CreateUndistMap()
    {
        cv::Mat map1, map2;
        cv::Size imageSize(m_width, m_height);
        cv::Mat M = m_cameraMatrix;
        cv::Mat K = m_distCoeffs;
        cv::initUndistortRectifyMap(M, K, cv::Matx33d::eye(),
            cv::getOptimalNewCameraMatrix(M, K, imageSize, 0, imageSize, 0), imageSize, CV_16SC2, map1, map2);
        m_map1 = map1;
        m_map2 = map2;
    }

private:

    int m_width;
    int m_height;
    cv::Mat m_cameraMatrix;
    cv::Mat m_distCoeffs;
    double m_totalAvgErr;
    cv::Mat m_map1, m_map2;
};

class CalibrationDialog::Board
{
public:

    Board(cv::Mat image)
    {
        if (image.channels() > 1)
            cv::cvtColor(image, m_image, cv::COLOR_BGR2GRAY);
        else
            m_image = image;
    }

    ~Board()
    {

    }

    void Draw(QGraphicsView* view, Result* pCalib)
    {
        cv::Mat image = m_image.clone();

        if (MarkWasFound())
        {
            drawChessboardCorners(image, m_patternSize, Mat(m_markPoints), true);
        }

        if (pCalib != NULL)
        {
            cv::Mat imageUndist;
            if (pCalib->UndistImage(image, imageUndist) )
                image = imageUndist;
        }

        DrawMat(image, view);
    }

    bool MarkWasFound() const
    {
        return !m_markPoints.empty();
    }

    bool FindMarkPoints(cv::Size patternSize, bool bFastCheck)
    {
        if (MarkWasFound())
            return true;

        std::vector<cv::Point2f> corners;
        int flags = CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE;
        if (bFastCheck)
            flags |= CALIB_CB_FAST_CHECK;
        if (!FindChessboardCorners(m_image, patternSize, corners, flags) )
            return false;

        m_markPoints = corners;
        m_patternSize = patternSize;
        return true;
    }

    std::vector<cv::Point2f> const& GetMarkPoints() const
    {
        return m_markPoints;
    }

    cv::Size GetImageSize() const
    {
        return m_image.size();
    }

    cv::Size GetPatternSize() const
    {
        return m_patternSize;
    }

    bool Save(char const* fname)
    {
        return imwrite(fname, m_image);
    }

private:

    static bool FindChessboardCorners(cv::InputArray image, Size patternSize, std::vector<cv::Point2f>& corners, int flags)
    {
        float const scale_width = 640.0f;
        float const scale_height = 480.0f;

        Size winSize(11, 11);
        Size size = image.size();
        float sw = size.width / scale_width;
        float sh = size.height / scale_height;
        float s = MAX(sw, sh);
        if (s > 1.0f)
        {
            cv::Mat scaled;
            cv::resize(image, scaled, Size(size.width / s, size.height / s));
            if (!cv::findChessboardCorners(scaled, patternSize, corners, flags) )
            {
                return false;
            }

            for (auto it = corners.begin(); it != corners.end(); ++it)
            {
                it->x *= s;
                it->y *= s;
            }
            winSize.width += 5 * s;
            winSize.height += 5 * s;
        }
        else
        {
            if (!cv::findChessboardCorners(image, patternSize, corners, flags) )
            {
                return false;
            }
        }

        cornerSubPix(image, corners, winSize,Size(-1,-1), TermCriteria( TermCriteria::EPS+TermCriteria::COUNT, 30, 0.1 ));
        return true;
    }

private:

    cv::Mat m_image;
    std::vector<cv::Point2f> m_markPoints;
    cv::Size m_patternSize;
};

CalibrationDialog::CalibrationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrationDialog)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(new QGraphicsScene(this));
}

CalibrationDialog::~CalibrationDialog()
{
    delete ui;
}

int CalibrationDialog::GetMarkWidth()
{
    int n = ui->spinBox_colums->value();
    return MAX(n, 3);
}

int CalibrationDialog::GetMarkHeight()
{
    int n = ui->spinBox_rows->value();
    return MAX(n, 3);
}

bool CalibrationDialog::FindBoardMark(int index)
{
    Board* board = m_Boards[index];
    bool r = board->FindMarkPoints(Size(GetMarkWidth(), GetMarkHeight()), true);
    ui->listWidget_calibrations->item(index)->setText(board->MarkWasFound() ? "OK!" : "Mark point extraction failed!");
    return r;
}

bool CalibrationDialog::SetCurSelBoard(int index)
{
    if (index >= 0 && index < ui->listWidget_calibrations->count())
    {
        ui->listWidget_calibrations->setCurrentRow(ui->listWidget_calibrations->count()-1);
        return true;
    }
    return false;
}


void CalibrationDialog::AddBoard(Mat const& img)
{
    Board* board = new Board(img);

    int index = ui->listWidget_calibrations->count();
    m_Boards.push_back(board);
    ui->listWidget_calibrations->insertItem(index,QString::number(index + 1));

    FindBoardMark(index);
    SetCurSelBoard(index);
    QApplication::processEvents();
}


static double computeReprojectionErrors( const std::vector<std::vector<Point3f> >& objectPoints,
    const std::vector<std::vector<Point2f> >& imagePoints,
    const std::vector<Mat>& rvecs, const std::vector<Mat>& tvecs,
    const Mat& cameraMatrix , const Mat& distCoeffs,
    std::vector<float>& perViewErrors, bool fisheye)
{
    std::vector<Point2f> imagePoints2;
    size_t totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for(size_t i = 0; i < objectPoints.size(); ++i )
    {
        if (fisheye)
        {
            fisheye::projectPoints(objectPoints[i], imagePoints2, rvecs[i], tvecs[i], cameraMatrix,
                distCoeffs);
        }
        else
        {
            projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
        }
        err = norm(imagePoints[i], imagePoints2, NORM_L2);

        size_t n = objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err*err/n);
        totalErr        += err*err;
        totalPoints     += n;
    }

    return std::sqrt(totalErr/totalPoints);
}

static void calcBoardCornerPositions(cv::Size boardSize, float squareSize, std::vector<Point3f>& corners,
    CalibrationDialog::Pattern patternType = CalibrationDialog::CHESSBOARD)
{
    corners.clear();

    switch(patternType)
    {
    case CalibrationDialog::CHESSBOARD:
    case CalibrationDialog::CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; ++i )
            for( int j = 0; j < boardSize.width; ++j )
                corners.push_back(Point3f(j*squareSize, i*squareSize, 0));
        break;

    case CalibrationDialog::ASYMMETRIC_CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++ )
            for( int j = 0; j < boardSize.width; j++ )
                corners.push_back(Point3f((2*j + i % 2)*squareSize, i*squareSize, 0));
        break;
    default:
        break;
    }
}

bool CalibrationDialog::runCalibration(int& imageCount, Pattern calibrationPattern, bool fisheye)
{
    cv::Size boardSize = cv::Size(GetMarkWidth(), GetMarkHeight());
    std::vector<std::vector<Point2f> > imagePoints;
    for (auto it = m_Boards.begin(); it != m_Boards.end(); ++it)
    {
        Board* board = *it;
        if (board->MarkWasFound() && board->GetPatternSize() == boardSize)
        {
            imagePoints.push_back(board->GetMarkPoints());
        }
    }
    if (imagePoints.empty())
        return false;

    std::vector<std::vector<Point3f>> objectPoints(1);
    calcBoardCornerPositions(boardSize, 50, objectPoints[0], calibrationPattern);
    objectPoints.resize(imagePoints.size(),objectPoints[0]);

    cv::Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
    cv::Mat distCoeffs;
    if (fisheye) {
        distCoeffs = Mat::zeros(4, 1, CV_64F);
    } else {
        distCoeffs = Mat::zeros(8, 1, CV_64F);
    }

    //Find intrinsic and extrinsic camera parameters
    double rms;
    Size imageSize = m_Boards[0]->GetImageSize();

    int flag = CALIB_FIX_PRINCIPAL_POINT
        | CALIB_ZERO_TANGENT_DIST
        | CALIB_FIX_ASPECT_RATIO
        | CALIB_FIX_K4
        | CALIB_FIX_K5;

    if (fisheye)
    {
        flag = fisheye::CALIB_FIX_SKEW | fisheye::CALIB_RECOMPUTE_EXTRINSIC;
        flag |= fisheye::CALIB_FIX_K4;
        flag |= fisheye::CALIB_FIX_PRINCIPAL_POINT;
    }

    std::vector<Mat> rvecs, tvecs;
    if (fisheye) {
        Mat _rvecs, _tvecs;
        rms = fisheye::calibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs,
            _rvecs, _tvecs, flag);
    } else {
        rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs,
            rvecs, tvecs, flag);
    }

    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);
    if (!ok)
        return false;

    std::vector<float> reprojErrs;
    double totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix,
        distCoeffs, reprojErrs, fisheye);
    imageCount = (int)imagePoints.size();
    m_Result.reset(new Result(imageSize.width, imageSize.height, cameraMatrix, distCoeffs, totalAvgErr));
    return ok;
}


bool CalibrationDialog::UpdateCalibResult(bool toUI)
{
    if (toUI)
    {
        if (m_Result.get() != NULL)
        {
            ui->spinBox_w->setValue(m_Result->Width());
            ui->spinBox_h->setValue(m_Result->Height());
            ui->doubleSpinBox_fx->setValue(m_Result->fx());
            ui->doubleSpinBox_fy->setValue(m_Result->fy());
            ui->doubleSpinBox_cx->setValue(m_Result->cx());
            ui->doubleSpinBox_cy->setValue(m_Result->cy());
            ui->doubleSpinBox_k1->setValue(m_Result->k1());
            ui->doubleSpinBox_k2->setValue(m_Result->k2());
            ui->doubleSpinBox_k3->setValue(m_Result->k3());
            ui->doubleSpinBox_p1->setValue(m_Result->p1());
            ui->doubleSpinBox_p2->setValue(m_Result->p2());
            ui->doubleSpinBox_err->setValue(m_Result->err());
        }
        else
        {
            ui->spinBox_w->setValue(0);
            ui->spinBox_h->setValue(0);
            ui->doubleSpinBox_fx->setValue(0);
            ui->doubleSpinBox_fy->setValue(0);
            ui->doubleSpinBox_cx->setValue(0);
            ui->doubleSpinBox_cy->setValue(0);
            ui->doubleSpinBox_k1->setValue(0);
            ui->doubleSpinBox_k2->setValue(0);
            ui->doubleSpinBox_k3->setValue(0);
            ui->doubleSpinBox_p1->setValue(0);
            ui->doubleSpinBox_p2->setValue(0);
            ui->doubleSpinBox_err->setValue(0);
        }

        return true;
    }
    else
    {
        Result r(ui->spinBox_w->value(), ui->spinBox_h->value());
        r.fx(ui->doubleSpinBox_fx->value());
        r.fy(ui->doubleSpinBox_fy->value());
        r.cx(ui->doubleSpinBox_cx->value());
        r.cy(ui->doubleSpinBox_cy->value());
        r.k1(ui->doubleSpinBox_k1->value());
        r.k2(ui->doubleSpinBox_k2->value());
        r.k3(ui->doubleSpinBox_k3->value());
        r.p1(ui->doubleSpinBox_p1->value());
        r.p2(ui->doubleSpinBox_p2->value());

        if (!r.Check())
            return false;

        m_Result.reset(new Result(r));
        return true;
    }
}

void CalibrationDialog::DrawBoard(int index)
{
    Board* board = m_Boards[index];

    board->Draw(ui->graphicsView, m_Result.get());

}

void CalibrationDialog::DrawCurrentBoard()
{
    int index = ui->listWidget_calibrations->currentIndex().row();
    if (index >= 0)
    {
        DrawBoard(index);
    }
}

void CalibrationDialog::on_pushButton_load_clicked()
{
    auto filenames = QFileDialog::getOpenFileNames(this,tr("Select file"),"",tr("config Files(*.jpg)"));
    for(auto filename : filenames) {
        cv::Mat image = cv::imread(filename.toLocal8Bit().data());
        if (!image.empty())
        {
            AddBoard(image);
        }
        else
        {
            QMessageBox::warning(this,tr("Calibration"),tr("Invalid images!"));
        }
    }
}

void CalibrationDialog::on_pushButton_calibrate_clicked()
{
    int imageCount;
    if (runCalibration(imageCount) )
    {
        UpdateCalibResult(true);
        DrawCurrentBoard();

        QMessageBox::information(this,tr("Calibration"),tr("Calibration ok!"));
        ui->pushButton_applyCalibration->setEnabled(true);
    }
    else
    {
        QMessageBox::warning(this,tr("Calibration"),tr("Calibration failed!"));
        ui->pushButton_applyCalibration->setDisabled(true);
    }
}

void CalibrationDialog::on_listWidget_calibrations_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(current == nullptr) return;
    DrawBoard(ui->listWidget_calibrations->currentIndex().row());
}

void CalibrationDialog::on_listWidget_calibrations_customContextMenuRequested(const QPoint &pos)
{
    auto qMenu = new QMenu(ui->listWidget_calibrations);
    qMenu->addAction(ui->actionRemove);
    qMenu->addAction(ui->actionClear);
    qMenu->exec(QCursor::pos()); //在鼠标点击的位置显示鼠标右键菜单
}


void CalibrationDialog::RemoveBoard(int index)
{
    if (index >= 0 && index < ui->listWidget_calibrations->count())
    {
        delete ui->listWidget_calibrations->takeItem(index);
        delete m_Boards[index];
        m_Boards.erase(m_Boards.begin() + index);
    }
}


void CalibrationDialog::on_actionRemove_triggered()
{
    int index = ui->listWidget_calibrations->currentRow();
    if (index >= 0)
    {
        RemoveBoard(index);
    }
}

void CalibrationDialog::on_actionClear_triggered()
{
    while (!m_Boards.empty())
    {
        RemoveBoard(m_Boards.size() - 1);
    }
}

void CalibrationDialog::on_pushButton_applyCalibration_clicked()
{
    accept();
}

int CalibrationDialog::width() {
    if(nullptr == m_Result.get())
        return 0;
    return m_Result->Width();
}

int CalibrationDialog::height() {
    if(nullptr == m_Result.get())
        return 0;

    return m_Result->Height();
}

QString CalibrationDialog::cameraMatraix() {
    if(nullptr == m_Result.get())
        return "0,0,0,0";

    return QString("%1,%2,%3,%4").arg(m_Result->fx()).arg(m_Result->fy()).arg(m_Result->cx()).arg(m_Result->cy());
}

QString CalibrationDialog::distortCoeffs() {
    if(nullptr == m_Result.get())
        return "0,0,0,0,0";

    return QString("%1,%2,%3,%4,%5").arg(m_Result->k1()).arg(m_Result->k2()).arg(m_Result->p1()).arg(m_Result->p2()).arg(m_Result->k3());
}
