#include "v4l2_camera.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <QImage>
#include <QDebug>
#include <cstring>
#include <unistd.h>
#include <QTimer>
#include <QMutexLocker>

extern "C" {
    extern unsigned char* g_last_jpeg;
    extern size_t g_last_jpeg_size;
    extern pthread_mutex_t g_jpeg_mutex;
}

V4L2Camera::V4L2Camera(const QString& devPath)
    : QObject(nullptr), devicePath(devPath)
{
    captureTimer = new QTimer(this);
    connect(captureTimer, &QTimer::timeout, this, &V4L2Camera::captureLoop);
}

V4L2Camera::~V4L2Camera() {
    close();
    if (captureTimer) {
        captureTimer->stop();
        delete captureTimer;
        captureTimer = nullptr;
    }
}

bool V4L2Camera::open() {
    v4l2_fd = ::open(devicePath.toStdString().c_str(), O_RDWR);
    if (v4l2_fd < 0) {
        qDebug() << "[V4L2Camera] 打开摄像头失败";
        return false;
    }
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = v4l2_width;
    fmt.fmt.pix.height = v4l2_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "[V4L2Camera] 设置格式失败";
        ::close(v4l2_fd);
        v4l2_fd = -1;
        return false;
    }
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(v4l2_fd, VIDIOC_REQBUFS, &req) < 0) {
        qDebug() << "[V4L2Camera] 请求缓冲区失败";
        ::close(v4l2_fd);
        v4l2_fd = -1;
        return false;
    }
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf) < 0) {
        qDebug() << "[V4L2Camera] 查询缓冲区失败";
        ::close(v4l2_fd);
        v4l2_fd = -1;
        return false;
    }
    v4l2_buffer_length = buf.length;
    v4l2_buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, v4l2_fd, buf.m.offset);
    if (v4l2_buffer == MAP_FAILED) {
        qDebug() << "[V4L2Camera] mmap失败";
        ::close(v4l2_fd);
        v4l2_fd = -1;
        return false;
    }
    if (ioctl(v4l2_fd, VIDIOC_QBUF, &buf) < 0) {
        qDebug() << "[V4L2Camera] 入队失败";
        ::close(v4l2_fd);
        v4l2_fd = -1;
        return false;
    }
    int type = buf.type;
    if (ioctl(v4l2_fd, VIDIOC_STREAMON, &type) < 0) {
        qDebug() << "[V4L2Camera] 启动采集失败";
        ::close(v4l2_fd);
        v4l2_fd = -1;
        return false;
    }
    qDebug() << "[V4L2Camera] 摄像头打开成功";
    if (captureTimer) captureTimer->start(50); // 20fps
    return true;
}

void V4L2Camera::close() {
    if (captureTimer) captureTimer->stop();
    if (v4l2_fd >= 0) {
        ::close(v4l2_fd);
        v4l2_fd = -1;
    }
    if (v4l2_buffer && v4l2_buffer != MAP_FAILED) {
        munmap(v4l2_buffer, v4l2_buffer_length);
        v4l2_buffer = nullptr;
        v4l2_buffer_length = 0;
    }
}

bool V4L2Camera::isOpen() const {
    return v4l2_fd >= 0;
}

void V4L2Camera::captureLoop() {
    if (v4l2_fd < 0) return;
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(v4l2_fd, VIDIOC_DQBUF, &buf) < 0) {
        return;
    }
    QImage img;
    bool loaded = img.loadFromData((const uchar*)v4l2_buffer, buf.bytesused, "JPEG");
    if (loaded) {
        QMutexLocker locker(&frameMutex);
        lastFrame = img;
    }
    ioctl(v4l2_fd, VIDIOC_QBUF, &buf);
}

QImage V4L2Camera::getFrame() {
    QMutexLocker locker(&frameMutex);
    return lastFrame.copy();
}

int V4L2Camera::width() const { return v4l2_width; }
int V4L2Camera::height() const { return v4l2_height; }
