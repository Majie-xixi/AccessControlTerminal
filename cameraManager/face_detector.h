#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <QObject>
#include <QImage>
#include <QVector>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

class FaceDetector : public QObject
{
    Q_OBJECT
public:
    explicit FaceDetector(QObject* parent = nullptr);

    bool loadCascade(const QString& xmlPath);
    bool isLoaded() const;

    void detectAsync(const QImage& frame);

signals:
    void facesDetected(const QVector<cv::Rect>& faces, int scaledWidth, int scaledHeight);

private:
    cv::CascadeClassifier m_cascade;
};

#endif // FACE_DETECTOR_H
