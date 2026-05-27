#include "face_detector.h"
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

FaceDetector::FaceDetector(QObject* parent)
    : QObject(parent)
{
}

bool FaceDetector::loadCascade(const QString& xmlPath)
{
    bool ok = m_cascade.load(xmlPath.toStdString());
    qDebug() << "[FaceDetector] cascade loaded:" << (ok ? "OK" : "FAILED");
    return ok;
}

bool FaceDetector::isLoaded() const
{
    return !m_cascade.empty();
}

void FaceDetector::detectAsync(const QImage& frame)
{
    if (m_cascade.empty() || frame.isNull()) return;

    QImage small = frame.scaled(320, 240, Qt::KeepAspectRatio);
    QtConcurrent::run([this, small]() {
        cv::Mat mat(small.height(), small.width(), CV_8UC4,
                    (void*)small.bits(), small.bytesPerLine());
        cv::Mat gray;
        cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);
        std::vector<cv::Rect> rects;
        m_cascade.detectMultiScale(gray, rects, 1.1, 3, 0, cv::Size(30, 30));
        QVector<cv::Rect> qfaces = QVector<cv::Rect>::fromStdVector(rects);
        emit facesDetected(qfaces, small.width(), small.height());
    });
}
