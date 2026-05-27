#include "face_manager.h"
#include <QDebug>
#include <QImage>

FaceManager::FaceManager(QObject* parent)
    : QObject(parent)
{
    faceRecognizer = cv::face::LBPHFaceRecognizer::create();
}

FaceManager::~FaceManager() {}

void FaceManager::addFaceSample(const QImage& faceImg)
{
    if (registeredFaces.size() >= collectTarget) return;
    QImage gray = faceImg.convertToFormat(QImage::Format_Grayscale8);
    cv::Mat mat(gray.height(), gray.width(), CV_8UC1, (void*)gray.bits(), gray.bytesPerLine());
    registeredFaces.append(mat.clone());
    emit faceSampled(registeredFaces.size(), collectTarget);
}

void FaceManager::train()
{
    if (registeredFaces.size() < collectTarget) {
        emit trainFinished(false);
        return;
    }
    std::vector<int> labels(collectTarget, 1);
    faceRecognizer->train(QVector<cv::Mat>(registeredFaces).toStdVector(), labels);
    emit trainFinished(true);
}

void FaceManager::clear()
{
    registeredFaces.clear();
}

void FaceManager::recognize(const QImage& faceImg)
{
    if (registeredFaces.size() < collectTarget) {
        emit recognizeResult(false, 0.0);
        return;
    }
    QImage gray = faceImg.convertToFormat(QImage::Format_Grayscale8);
    cv::Mat testFace(gray.height(), gray.width(), CV_8UC1, (void*)gray.bits(), gray.bytesPerLine());
    int predictedLabel = -1;
    double confidence = 0.0;
    faceRecognizer->predict(testFace, predictedLabel, confidence);
    bool success = (predictedLabel == 1 && confidence < 90);
    emit recognizeResult(success, confidence);
}

int FaceManager::sampleCount() const
{
    return registeredFaces.size();
}