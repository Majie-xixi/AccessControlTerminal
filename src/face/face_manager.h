#ifndef FACE_MANAGER_H
#define FACE_MANAGER_H

#include <QObject>
#include <QVector>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

class FaceManager : public QObject
{
    Q_OBJECT
public:
    explicit FaceManager(QObject* parent = nullptr);
    ~FaceManager();

    // 注册与识别接口
    void addFaceSample(const QImage& faceImg);
    void train();
    void clear();
    void recognize(const QImage& faceImg);

    int sampleCount() const;

signals:
    void faceSampled(int current, int total);
    void trainFinished(bool success);
    void recognizeResult(bool success, double confidence);

private:
    QVector<cv::Mat> registeredFaces;
    int collectTarget = 3;
    cv::Ptr<cv::face::LBPHFaceRecognizer> faceRecognizer;
};

#endif // FACE_MANAGER_H