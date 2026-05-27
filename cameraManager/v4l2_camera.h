#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QString>
#include <QImage>
#include <QTimer>
#include <QMutex>

class V4L2Camera : public QObject {
    Q_OBJECT
public:
    V4L2Camera(const QString& devPath = "/dev/video0");
    ~V4L2Camera();

    bool open();
    void close();
    bool isOpen() const;
    QImage getFrame();
    int width() const;
    int height() const;

private slots:
    void captureLoop();

private:
    QString devicePath;
    int v4l2_fd = -1;
    void* v4l2_buffer = nullptr;
    size_t v4l2_buffer_length = 0;
    int v4l2_width = 640, v4l2_height = 480;
    QImage lastFrame;
    QTimer* captureTimer = nullptr;
    QMutex frameMutex;
};

#endif // V4L2CAMERA_H
