# SmartPro 模块化重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 SmartPro 终端-T113 的功能代码迁移到 access_control_terminal 模板，按 Manager 模式 + 功能模块化重组目录结构。

**Architecture:** 11 个功能目录 (`src/` 下)，AppCoordinator 作为信号编排中枢，Widget 瘦身为纯容器。同层模块不直接依赖，通过 Coordinator 转发。

**Tech Stack:** Qt5 (Core/Gui/Widgets/Network/SerialPort/Concurrent), OpenCV4 (core/imgproc/objdetect/face), ALSA (libasound), libgpiod (可选), CMake

**Source:** SmartPro 源码位于 `E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew/`

---

### Task 1: 创建目录结构与迁移图片资源

**Files:**
- Create: `src/core/`, `src/camera/`, `src/face/`, `src/fingerprint/`, `src/password/`, `src/call/`, `src/audio/`, `src/network/`, `src/hardware/` 目录

- [ ] **Step 1: 创建所有模块目录**

```bash
mkdir -p src/{core,camera,face,fingerprint,password,call,audio,network,hardware}
```

- [ ] **Step 2: 迁移 UI 图片资源（保留模板的 fufu 背景风格）**

模板的 `pic/` 目录和 `pic.qrc` 保持不动。后续如果需要 SmartPro 的额外图片再单独添加。

- [ ] **Step 3: 更新 res.qrc**

模板有两份资源文件引用（`pic.qrc`），统一为一个。将 `pic.qrc` 重命名为 `res.qrc`，删除 `.pro` 中重复的 `pic.qrc` 引用。

```bash
mv pic.qrc res.qrc
```

更新 `res.qrc` 确保内容不变：

```xml
<RCC>
    <qresource prefix="/">
        <file>pic/tel2.png</file>
        <file>pic/tel.png</file>
        <file>pic/power2.png</file>
        <file>pic/power.png</file>
        <file>pic/password2.png</file>
        <file>pic/password.png</file>
        <file>pic/fufu.png</file>
        <file>pic/fingerprint2.png</file>
        <file>pic/fingerprint.png</file>
        <file>pic/face2.png</file>
        <file>pic/face.png</file>
        <file>pic/addAdmin2.png</file>
        <file>pic/addAdmin.png</file>
        <file>pic/fufu1.png</file>
    </qresource>
</RCC>
```

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "chore: create src/ module directories"
```

---

### Task 2: 迁移并瘦身主窗口 Widget → src/core/

**Files:**
- Move: `widget.h` → `src/core/widget.h`
- Move: `widget.cpp` → `src/core/widget.cpp`
- Move: `widget.ui` → `src/core/widget.ui`
- Modify: `src/core/widget.ui` — 统一按钮 objectName，匹配 SmartPro 的命名
- Modify: `src/core/widget.h` — 移除 `TimerUpdate`，添加按钮槽声明
- Modify: `src/core/widget.cpp` — 只保留 UI 初始化和时间刷新
- Modify: `main.cpp` — include 路径更新

- [ ] **Step 1: 移动 UI 文件**

```bash
mv widget.h src/core/widget.h
mv widget.cpp src/core/widget.cpp
mv widget.ui src/core/widget.ui
```

- [ ] **Step 2: 修改 src/core/widget.ui — 按钮 objectName 对齐 SmartPro**

将模板的 6 个 objectName 改为与 SmartPro 一致，方便后续代码复用：

| 原名称 | 新名称 |
|---|---|
| `pushButton_call` | `btncall` |
| `pushButton_fingerprint` | `btnfinger_step` |
| `pushButton_face` | `btnface` |
| `pushButton_addadmin` | `btnadd` |
| `pushButton_password` | `btnpsswd` |
| `pushButton_power` | `btnpower` |

在 `widget.ui` 中逐个替换：

替换 `pushButton_call` → `btncall`（共出现 2 处：name 属性 + 对应的 widget 闭合标签）
替换 `pushButton_fingerprint` → `btnfinger_step`
替换 `pushButton_face` → `btnface`
替换 `pushButton_addadmin` → `btnadd`
替换 `pushButton_password` → `btnpsswd`
替换 `pushButton_power` → `btnpower`

同时将 `widget_video` 重命名为 `widget_4`，`widget_dateTime` 重命名为 `widget_2`，`widget_button` 重命名为 `widget_3`，内部的 QLabel 名称保持 `label_time`/`label_date`，但后面需要在 widget.h 里引用。

在 `widget_4` 内部加一个 QLabel 用于摄像头显示，命名 `labelcam`（跟 SmartPro 对齐）：

在 widget.ui 中 widget_4 内添加：

```xml
<widget class="QLabel" name="labelcam">
 <property name="geometry">
  <rect><x>20</x><y>20</y><width>430</width><height>475</height></rect>
 </property>
 <property name="minimumSize">
  <size><width>430</width><height>475</height></size>
 </property>
 <property name="maximumSize">
  <size><width>430</width><height>475</height></size>
 </property>
 <property name="styleSheet">
  <string>background-color: rgb(85, 87, 83); border: none;</string>
 </property>
 <property name="text"><string/></property>
</widget>
```

- [ ] **Step 3: 修改 `src/core/widget.h`**

```cpp
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class AppCoordinator;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void setCoordinator(AppCoordinator* coordinator);

signals:
    void btnCallClicked();
    void btnAddClicked();
    void btnPasswordClicked();
    void btnPowerClicked();
    void btnFaceClicked();

private:
    Ui::Widget *ui;
    AppCoordinator* m_coordinator = nullptr;
};

#endif // WIDGET_H
```

- [ ] **Step 4: 修改 `src/core/widget.cpp`**

```cpp
#include "widget.h"
#include "ui_widget.h"
#include "app_coordinator.h"
#include <QDateTime>
#include <QTimer>
#include <QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 隐藏鼠标
    this->setCursor(Qt::BlankCursor);

    // 按钮去焦点框
    QList<QPushButton*> btns = {
        ui->btnadd, ui->btncall, ui->btnface, ui->btnpower, ui->btnpsswd, ui->btnfinger_step
    };
    for (QPushButton* btn : btns) {
        btn->setFocusPolicy(Qt::NoFocus);
    }

    // 时间日期刷新
    QTimer *dateTimeTimer = new QTimer(this);
    connect(dateTimeTimer, &QTimer::timeout, this, [this]() {
        QDateTime current = QDateTime::currentDateTime();
        ui->label_date->setText(current.date().toString("yyyy-MM-dd"));
        ui->label_time->setText(current.time().toString("HH:mm:ss"));
    });
    dateTimeTimer->start(1000);
    QDateTime current = QDateTime::currentDateTime();
    ui->label_date->setText(current.date().toString("yyyy-MM-dd"));
    ui->label_time->setText(current.time().toString("HH:mm:ss"));

    // 按钮点击 → 发信号
    connect(ui->btncall, &QPushButton::clicked, this, &Widget::btnCallClicked);
    connect(ui->btnadd, &QPushButton::clicked, this, &Widget::btnAddClicked);
    connect(ui->btnpsswd, &QPushButton::clicked, this, &Widget::btnPasswordClicked);
    connect(ui->btnpower, &QPushButton::clicked, this, &Widget::btnPowerClicked);
    connect(ui->btnface, &QPushButton::clicked, this, [this]() {
        emit btnAddClicked(); // 人脸按钮复用到注册流程
    });

    // 去掉全局焦点虚框
    this->setStyleSheet("QPushButton:focus { outline: none; }");
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setCoordinator(AppCoordinator* coordinator)
{
    m_coordinator = coordinator;
}
```

- [ ] **Step 5: 修改 main.cpp — 更新 include 路径**

```cpp
#include "src/core/widget.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // OpenGL ES (T113 需要)
    QSurfaceFormat format;
    format.setVersion(2, 0);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    QSurfaceFormat::setDefaultFormat(format);

    Widget w;
    w.show();
    return a.exec();
}
```

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "refactor: move Widget to src/core/, strip to thin container with signals"
```

---

### Task 3: GPIO 控制器 — src/hardware/

**Files:**
- Create: `src/hardware/gpio_controller.h`
- Create: `src/hardware/gpio_controller.cpp`

从 SmartPro `widget.h:75-118` 内联的 `GpioPe4MonitorThread` 类提取出来。

- [ ] **Step 1: 创建 `src/hardware/gpio_controller.h`**

```cpp
#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include <QObject>
#include <QThread>
#include <atomic>
#include <gpiod.h>

class GpioMonitorThread : public QThread {
    Q_OBJECT
public:
    GpioMonitorThread(QObject* parent = nullptr)
        : QThread(parent), m_running(true), m_paused(false) {}
    void stop() { m_running = false; }
    void pause() { m_paused = true; }
    void resume() { m_paused = false; }

signals:
    void pe4High();
    void pe4Low();

protected:
    void run() override {
        struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
        if (!chip) return;
        struct gpiod_line *line = gpiod_chip_get_line(chip, 132); // PE4
        if (!line) { gpiod_chip_close(chip); return; }
        gpiod_line_request_input(line, "pe4_monitor");
        int highCount = 0;
        while (m_running) {
            if (m_paused) {
                msleep(100);
                continue;
            }
            int value = gpiod_line_get_value(line);
            if (value == 1) {
                highCount++;
                if (highCount >= 1) {
                    emit pe4High();
                    highCount = 0;
                }
            } else {
                highCount = 0;
                emit pe4Low();
            }
            msleep(400);
        }
        gpiod_line_release(line);
        gpiod_chip_close(chip);
    }

private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_paused;
};

class GpioController : public QObject
{
    Q_OBJECT
public:
    explicit GpioController(QObject* parent = nullptr);
    ~GpioController();

    void startMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();
    void setFillLight(bool on);

signals:
    void personDetected();
    void personLeft();

private:
    GpioMonitorThread* m_monitorThread = nullptr;
    struct gpiod_line* m_fillLightLine = nullptr;  // PE5, GPIO 133
};

#endif // GPIO_CONTROLLER_H
```

- [ ] **Step 2: 创建 `src/hardware/gpio_controller.cpp`**

```cpp
#include "gpio_controller.h"
#include <QDebug>

GpioController::GpioController(QObject* parent)
    : QObject(parent)
{
#ifdef HAS_LIBGPIOD
    struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
    if (chip) {
        m_fillLightLine = gpiod_chip_get_line(chip, 133); // PE5
        if (m_fillLightLine)
            gpiod_line_request_output(m_fillLightLine, "fill_light", 0);
    }
#endif
    m_monitorThread = new GpioMonitorThread(this);
}

GpioController::~GpioController()
{
    if (m_monitorThread) {
        m_monitorThread->stop();
        m_monitorThread->wait();
    }
}

void GpioController::startMonitoring()
{
    connect(m_monitorThread, &GpioMonitorThread::pe4High,
            this, &GpioController::personDetected);
    connect(m_monitorThread, &GpioMonitorThread::pe4Low,
            this, &GpioController::personLeft);
    m_monitorThread->start();
}

void GpioController::pauseMonitoring()
{
    m_monitorThread->pause();
}

void GpioController::resumeMonitoring()
{
    m_monitorThread->resume();
}

void GpioController::setFillLight(bool on)
{
#ifdef HAS_LIBGPIOD
    if (m_fillLightLine)
        gpiod_line_set_value(m_fillLightLine, on ? 1 : 0);
#else
    Q_UNUSED(on);
#endif
}
```

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "feat: add GpioController — PE4 body detection + PE5 fill light"
```

---

### Task 4: 音频管理器 — src/audio/

**Files:**
- Copy: SmartPro `managers/audio_manager.h` → `src/audio/audio_manager.h`
- Copy: SmartPro `managers/audio_manager.cpp` → `src/audio/audio_manager.cpp`

- [ ] **Step 1: 复制并调整 include 路径**

```bash
cp "E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew/managers/audio_manager.h" src/audio/audio_manager.h
cp "E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew/managers/audio_manager.cpp" src/audio/audio_manager.cpp
```

`src/audio/audio_manager.cpp` 第 1 行 include 确保为：
```cpp
#include "audio_manager.h"
```

- [ ] **Step 2: Commit**

```bash
git add -A
git commit -m "feat: add AudioManager from SmartPro"
```

---

### Task 5: TCP 信令客户端 — src/network/

**Files:**
- Create: `src/network/tcp_signaling.h`
- Create: `src/network/tcp_signaling.cpp`

从 SmartPro `widget.cpp:195-206` 提取 TCP 连接逻辑。

- [ ] **Step 1: 创建 `src/network/tcp_signaling.h`**

```cpp
#ifndef TCP_SIGNALING_H
#define TCP_SIGNALING_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class TcpSignaling : public QObject
{
    Q_OBJECT
public:
    explicit TcpSignaling(const QString& serverIp = "192.168.1.4",
                          quint16 port = 8887,
                          QObject* parent = nullptr);
    ~TcpSignaling();

    void connectToServer();
    void send(const QString& message);
    bool isConnected() const;

signals:
    void messageReceived(const QString& msg);
    void connected();
    void disconnected();

private:
    QTcpSocket* m_socket;
    QString m_serverIp;
    quint16 m_serverPort;
};

#endif // TCP_SIGNALING_H
```

- [ ] **Step 2: 创建 `src/network/tcp_signaling.cpp`**

```cpp
#include "tcp_signaling.h"
#include <QDebug>

TcpSignaling::TcpSignaling(const QString& serverIp, quint16 port, QObject* parent)
    : QObject(parent), m_serverIp(serverIp), m_serverPort(port)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
        QByteArray datagram = m_socket->readAll();
        QStringList messages = QString::fromUtf8(datagram).split('\n', QString::SkipEmptyParts);
        for (const QString& msg : messages) {
            QString trimmed = msg.trimmed();
            qDebug() << "[TCP] received:" << trimmed;
            emit messageReceived(trimmed);
        }
    });
    connect(m_socket, &QTcpSocket::connected, this, &TcpSignaling::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "[TCP] disconnected, auto-reconnect in 3s";
        emit disconnected();
        QTimer::singleShot(3000, this, &TcpSignaling::connectToServer);
    });
}

TcpSignaling::~TcpSignaling()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->disconnectFromHost();
}

void TcpSignaling::connectToServer()
{
    qDebug() << "[TCP] connecting to" << m_serverIp << m_serverPort;
    m_socket->connectToHost(m_serverIp, m_serverPort);
}

void TcpSignaling::send(const QString& message)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write((message + "\n").toUtf8());
        m_socket->flush();
    }
}

bool TcpSignaling::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}
```

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "feat: add TcpSignaling — TCP client with auto-reconnect"
```

---

### Task 6: 摄像头模块 — src/camera/

**Files:**
- Copy: SmartPro `camera/v4l2camera.h` → `src/camera/v4l2_camera.h`
- Copy: SmartPro `camera/v4l2camera.cpp` → `src/camera/v4l2_camera.cpp`
- Copy: SmartPro `camera/camera_udp_server.h` → `src/camera/camera_udp_server.h`
- Copy: SmartPro `camera/camera_udp_server.cpp` → `src/camera/camera_udp_server.cpp`
- Copy: SmartPro `camera_push_thread.h` → `src/camera/camera_push_thread.h`
- Copy: SmartPro `camera_push_thread.cpp` → `src/camera/camera_push_thread.cpp`

- [ ] **Step 1: 复制文件并修复 include 路径**

```bash
SRC="E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew"
cp "$SRC/camera/v4l2camera.h" src/camera/v4l2_camera.h
cp "$SRC/camera/v4l2camera.cpp" src/camera/v4l2_camera.cpp
cp "$SRC/camera/camera_udp_server.h" src/camera/camera_udp_server.h
cp "$SRC/camera/camera_udp_server.cpp" src/camera/camera_udp_server.cpp
cp "$SRC/camera_push_thread.h" src/camera/camera_push_thread.h
cp "$SRC/camera_push_thread.cpp" src/camera/camera_push_thread.cpp
```

修复 `src/camera/camera_udp_server.cpp`：
- `#include "wdiget.h"` → `#include "src/core/widget.h"`  (同时修正拼写错误，但尽可能只依赖需要的宏 `SELF_IP`。改为内联定义)
- `SELF_IP` 在该文件中被使用，改为在 `camera_udp_server.h` 中本地定义：

```cpp
// camera_udp_server.h 顶部
#ifndef SELF_IP
#define SELF_IP "192.168.1.8"
#endif
```

`src/camera/camera_udp_server.cpp` 的 `#include "wdiget.h"` 改为 `#include "camera_udp_server.h"`（已经包含了，移除错误的 include）。

修复 `src/camera/v4l2_camera.h` — 将头文件内的 `captureLoop` 改为 `private slots:` 保持（已经是），include 添加 `QObject` 基类。

修复 `src/camera/v4l2_camera.cpp` — 第 1 行 include:
```cpp
#include "v4l2_camera.h"
```

修复 `src/camera/camera_push_thread.h` — include:
```cpp
#include "v4l2_camera.h"
```

修复 `src/camera/camera_push_thread.cpp` — include:
```cpp
#include "camera_push_thread.h"
#include "v4l2_camera.h"
```

- [ ] **Step 2: Commit**

```bash
git add -A
git commit -m "feat: migrate camera module — V4L2 driver, UDP server, push thread"
```

---

### Task 7: 人脸模块 — src/face/

**Files:**
- Copy: SmartPro `managers/face_manager.h` → `src/face/face_manager.h`
- Copy: SmartPro `managers/face_manager.cpp` → `src/face/face_manager.cpp`
- Create: `src/face/face_detector.h`
- Create: `src/face/face_detector.cpp`

FaceDetector 从 SmartPro `widget.cpp:646-658` 提取 Haar 级联检测逻辑。

- [ ] **Step 1: 复制 FaceManager**

```bash
SRC="E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew"
cp "$SRC/managers/face_manager.h" src/face/face_manager.h
cp "$SRC/managers/face_manager.cpp" src/face/face_manager.cpp
```

`src/face/face_manager.cpp` include 调整为：
```cpp
#include "face_manager.h"
```

- [ ] **Step 2: 创建 `src/face/face_detector.h`**

```cpp
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

    // 异步检测，结果通过 signal 返回
    void detectAsync(const QImage& frame);

signals:
    void facesDetected(const QVector<cv::Rect>& faces, int scaledWidth, int scaledHeight);

private:
    cv::CascadeClassifier m_cascade;
};

#endif // FACE_DETECTOR_H
```

- [ ] **Step 3: 创建 `src/face/face_detector.cpp`**

```cpp
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

    // 缩小图像加速检测
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
```

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "feat: add face module — FaceManager + FaceDetector"
```

---

### Task 8: 指纹模块 — src/fingerprint/

**Files:**
- Copy: SmartPro `HailinFingerprint.h` → `src/fingerprint/hailin_fingerprint.h`
- Copy: SmartPro `HailinFingerprint.cpp` → `src/fingerprint/hailin_fingerprint.cpp`
- Copy: SmartPro `managers/fingerprint_manager.h` → `src/fingerprint/fingerprint_manager.h`
- Copy: SmartPro `managers/fingerprint_manager.cpp` → `src/fingerprint/fingerprint_manager.cpp`
- Copy: SmartPro `form_finger.h` → `src/fingerprint/form_finger.h`
- Copy: SmartPro `form_finger.cpp` → `src/fingerprint/form_finger.cpp`
- Copy: SmartPro `form_finger.ui` → `src/fingerprint/form_finger.ui`

- [ ] **Step 1: 复制文件**

```bash
SRC="E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew"
cp "$SRC/HailinFingerprint.h" src/fingerprint/hailin_fingerprint.h
cp "$SRC/HailinFingerprint.cpp" src/fingerprint/hailin_fingerprint.cpp
cp "$SRC/managers/fingerprint_manager.h" src/fingerprint/fingerprint_manager.h
cp "$SRC/managers/fingerprint_manager.cpp" src/fingerprint/fingerprint_manager.cpp
cp "$SRC/form_finger.h" src/fingerprint/form_finger.h
cp "$SRC/form_finger.cpp" src/fingerprint/form_finger.cpp
cp "$SRC/form_finger.ui" src/fingerprint/form_finger.ui
```

- [ ] **Step 2: 修复 include 路径**

`src/fingerprint/hailin_fingerprint.cpp` line 1:
```cpp
#include "hailin_fingerprint.h"
```

`src/fingerprint/fingerprint_manager.h` line 8:
```cpp
#include "hailin_fingerprint.h"
```

`src/fingerprint/fingerprint_manager.cpp` line 1-2:
```cpp
#include "fingerprint_manager.h"
#include "hailin_fingerprint.h"
```

`src/fingerprint/form_finger.h` line 4-6 保持不变（Qt 头文件），内部 QWidget 保持。

`src/fingerprint/form_finger.cpp` line 1-4:
```cpp
#include "form_finger.h"
#include "ui_form_finger.h"
```

**关键修复 — form_finger.cpp 中通过 parentWidget() 获取 Widget 的方式**：
原代码：
```cpp
QWidget* w = parentWidget();
while (w && !qobject_cast<Widget*>(w)) w = w->parentWidget();
Widget* mainWidget = qobject_cast<Widget*>(w);
```

改为通过信号/槽方式，form_finger 不再直接访问 Widget：
- `form_finger.h` 添加信号 `void requestPlayAudio(const QString& path)` 和 `void requestFingerprintCommand(int cmd, int id)`
- Coordinator 连接这些信号到实际的 Manager

在 `form_finger.h` 添加：
```cpp
signals:
    void addFaceRequested();
    void addFingerRequested();
    void requestPlayAudio(const QString& filePath);
    void requestGetNextFingerId(int& id);
```

在 `form_finger.cpp` 中，所有原来通过 `mainWidget->` 调用的地方改为 emit 信号。

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "feat: add fingerprint module — driver, manager, registration UI"
```

---

### Task 9: 密码模块 — src/password/

**Files:**
- Copy: SmartPro `managers/password_manager.h` → `src/password/password_manager.h`
- Copy: SmartPro `managers/password_manager.cpp` → `src/password/password_manager.cpp`
- Copy: SmartPro `form_passwd.h` → `src/password/form_passwd.h`
- Copy: SmartPro `form_passwd.cpp` → `src/password/form_passwd.cpp`
- Copy: SmartPro `form_passwd.ui` → `src/password/form_passwd.ui`

- [ ] **Step 1: 复制文件并修复 include**

```bash
SRC="E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew"
cp "$SRC/managers/password_manager.h" src/password/password_manager.h
cp "$SRC/managers/password_manager.cpp" src/password/password_manager.cpp
cp "$SRC/form_passwd.h" src/password/form_passwd.h
cp "$SRC/form_passwd.cpp" src/password/form_passwd.cpp
cp "$SRC/form_passwd.ui" src/password/form_passwd.ui
```

`src/password/form_passwd.cpp` — 移除 `#include "widget.h"`，将按键音播放改为信号方式。

在 `form_passwd.h` 添加信号：
```cpp
signals:
    void passwordEntered(const QString& password);
    void requestPlayAudio(const QString& filePath);
```

`form_passwd.cpp` 中 `playKeySound()` 改为 `emit requestPlayAudio(path)`，移除 `getMainWidget()` 函数。

- [ ] **Step 2: Commit**

```bash
git add -A
git commit -m "feat: add password module — manager + input UI"
```

---

### Task 10: 对讲模块 — src/call/

**Files:**
- Copy: SmartPro `audiosenderthread.h` → `src/call/audio_sender.h`
- Copy: SmartPro `audiosenderthread.cpp` → `src/call/audio_sender.cpp`
- Copy: SmartPro `audioreceiverthread.h` → `src/call/audio_receiver.h`
- Copy: SmartPro `audioreceiverthread.cpp` → `src/call/audio_receiver.cpp`
- Copy: SmartPro `form_call.h` → `src/call/form_call.h`
- Copy: SmartPro `form_call.cpp` → `src/call/form_call.cpp`
- Copy: SmartPro `form_call.ui` → `src/call/form_call.ui`
- Create: `src/call/call_controller.h`
- Create: `src/call/call_controller.cpp`

CallController 从 SmartPro `widget.cpp:339-535` 提取呼叫流程：
- `on_btncall_clicked()` → `startCall()`
- `onServerAccept()` → `onAccept()`
- `hangupCall()` → `hangup()`
- `onTcpReadyRead()` 中的 accept/recstop 分发

- [ ] **Step 1: 复制 AudioSender/AudioReceiver/FormCall**

```bash
SRC="E:/MJ_Demo/smartpronew项目最终源码/最终项目/终端-T113/smartpronew"
cp "$SRC/audiosenderthread.h" src/call/audio_sender.h
cp "$SRC/audiosenderthread.cpp" src/call/audio_sender.cpp
cp "$SRC/audioreceiverthread.h" src/call/audio_receiver.h
cp "$SRC/audioreceiverthread.cpp" src/call/audio_receiver.cpp
cp "$SRC/form_call.h" src/call/form_call.h
cp "$SRC/form_call.cpp" src/call/form_call.cpp
cp "$SRC/form_call.ui" src/call/form_call.ui
```

修复各文件 include：
- `audio_sender.cpp` line 1: `#include "audio_sender.h"`
- `audio_receiver.cpp` line 1: `#include "audio_receiver.h"`
- `form_call.cpp` line 1-2: `#include "form_call.h"` / `#include "ui_form_call.h"`

**移除 form_call.cpp 中对 Widget 的直接依赖**：
原代码：
```cpp
#include "widget.h"
// ...
QWidget* w = parentWidget();
while (w && !qobject_cast<Widget*>(w)) w = w->parentWidget();
auto widget = qobject_cast<Widget*>(w);
```

改为添加信号：
在 `form_call.h` 添加：
```cpp
signals:
    void hangupRequested();
    void requestStopAudio();
    void requestSendStopsend();
```

`form_call.cpp` 中挂断按钮的 lambda 改为直接 `emit hangupRequested()`，其余 audioManager->stop() 和 tcp write 的逻辑移到 Coordinator。

- [ ] **Step 2: 创建 `src/call/call_controller.h`**

```cpp
#ifndef CALL_CONTROLLER_H
#define CALL_CONTROLLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QString>
#include <memory>

class V4L2Camera;
class CameraPushThread;
class AudioSenderThread;
class AudioReceiverThread;
class Form_CALL;
class TcpSignaling;
class AudioManager;
class GpioController;

enum class CallState { Idle, Calling, Connected, HangingUp };

class CallController : public QObject
{
    Q_OBJECT
public:
    explicit CallController(QObject* parent = nullptr);
    ~CallController();

    void setCamera(V4L2Camera* camera);
    void setSignaling(TcpSignaling* signaling);
    void setAudioManager(AudioManager* am);
    void setConnAudioManager(AudioManager* am);
    void setGpioController(GpioController* gpio);
    void setCallFormParent(QWidget* parent, const QPoint& anchorPos);

    CallState state() const { return m_state; }

public slots:
    void startCall();
    void onAccept();
    void hangup();

signals:
    void callStateChanged(CallState state);

private:
    void cleanupThreads();

    CallState m_state = CallState::Idle;
    V4L2Camera* m_camera = nullptr;
    TcpSignaling* m_signaling = nullptr;
    AudioManager* m_audioManager = nullptr;
    AudioManager* m_connAudioManager = nullptr;
    GpioController* m_gpio = nullptr;
    QWidget* m_formParent = nullptr;
    QPoint m_formAnchor;

    Form_CALL* m_callForm = nullptr;
    CameraPushThread* m_cameraPush = nullptr;
    AudioSenderThread* m_audioSender = nullptr;
    AudioReceiverThread* m_audioReceiver = nullptr;
    int m_audioSocketFd = -1;
};

#endif // CALL_CONTROLLER_H
```

- [ ] **Step 3: 创建 `src/call/call_controller.cpp`**

```cpp
#include "call_controller.h"
#include "form_call.h"
#include "audio_sender.h"
#include "audio_receiver.h"
#include "src/camera/v4l2_camera.h"
#include "src/camera/camera_push_thread.h"
#include "src/network/tcp_signaling.h"
#include "src/audio/audio_manager.h"
#include "src/hardware/gpio_controller.h"
#include <QDebug>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

CallController::CallController(QObject* parent)
    : QObject(parent)
{
}

CallController::~CallController()
{
    hangup();
}

void CallController::setCamera(V4L2Camera* camera) { m_camera = camera; }
void CallController::setSignaling(TcpSignaling* signaling) { m_signaling = signaling; }
void CallController::setAudioManager(AudioManager* am) { m_audioManager = am; }
void CallController::setConnAudioManager(AudioManager* am) { m_connAudioManager = am; }
void CallController::setGpioController(GpioController* gpio) { m_gpio = gpio; }

void CallController::setCallFormParent(QWidget* parent, const QPoint& anchorPos)
{
    m_formParent = parent;
    m_formAnchor = anchorPos;
}

void CallController::startCall()
{
    if (m_state != CallState::Idle) return;
    m_state = CallState::Calling;
    emit callStateChanged(m_state);

    m_audioManager->playAsync("/tmp/pcm/calling.pcm");

    // chained playback: calling.pcm → conn.pcm
    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = connect(m_audioManager, &AudioManager::playbackFinished, this,
        [this, conn](const QString& file) {
            if (file == "/tmp/pcm/calling.pcm") {
                m_connAudioManager->playAsync("/tmp/pcm/conn.pcm");
                QObject::disconnect(*conn);
                delete conn;
            }
        });

    // Show call form
    if (m_callForm) { m_callForm->close(); delete m_callForm; }
    m_callForm = new Form_CALL(m_formParent);
    m_callForm->move(m_formAnchor);
    m_callForm->setStatusText(QString::fromUtf8("正在呼叫..."));
    m_callForm->show();
    m_callForm->raise();

    connect(m_callForm, &Form_CALL::hangupRequested, this, &CallController::hangup);

    // Send call signal
    m_signaling->send("call");

    if (m_gpio) m_gpio->pauseMonitoring();
}

void CallController::onAccept()
{
    if (m_state != CallState::Calling) return;
    m_state = CallState::Connected;
    emit callStateChanged(m_state);

    if (m_connAudioManager) m_connAudioManager->stop();

    // Create audio UDP socket
    m_audioSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_audioSocketFd < 0) {
        qWarning() << "[Call] failed to create audio socket";
        hangup();
        return;
    }
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(12345);
    if (bind(m_audioSocketFd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        qWarning() << "[Call] failed to bind port 12345";
        ::close(m_audioSocketFd);
        m_audioSocketFd = -1;
        hangup();
        return;
    }

    // Start camera push
    m_cameraPush = new CameraPushThread(m_camera, "192.168.1.4", 23456, this);
    m_cameraPush->start();

    // Start audio sender
    m_audioSender = new AudioSenderThread(m_audioSocketFd, "192.168.1.4", 12345, this);
    m_audioSender->start();

    // Start audio receiver (delayed 400ms)
    QTimer::singleShot(400, this, [this]() {
        m_audioReceiver = new AudioReceiverThread(m_audioSocketFd, this);
        m_audioReceiver->setVolume(30);
        m_audioReceiver->start();
    });

    if (m_callForm) m_callForm->startCallTimer();
}

void CallController::hangup()
{
    CallState prevState = m_state;
    m_state = CallState::HangingUp;
    emit callStateChanged(m_state);

    if (m_connAudioManager) m_connAudioManager->stop();

    if (m_callForm) {
        m_callForm->stopCallTimer();
        m_callForm->close();
        delete m_callForm;
        m_callForm = nullptr;
    }

    cleanupThreads();

    if (m_signaling) m_signaling->send("stopsend");

    if (m_audioSocketFd >= 0) {
        ::close(m_audioSocketFd);
        m_audioSocketFd = -1;
    }

    if (m_gpio) m_gpio->resumeMonitoring();

    m_state = CallState::Idle;
    emit callStateChanged(m_state);
}

void CallController::cleanupThreads()
{
    if (m_cameraPush) {
        m_cameraPush->stop();
        m_cameraPush->wait(3000);
        delete m_cameraPush;
        m_cameraPush = nullptr;
    }
    if (m_audioSender) {
        m_audioSender->stop();
        m_audioSender->wait(3000);
        delete m_audioSender;
        m_audioSender = nullptr;
    }
    if (m_audioReceiver) {
        m_audioReceiver->stop();
        m_audioReceiver->wait(3000);
        delete m_audioReceiver;
        m_audioReceiver = nullptr;
    }
}
```

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "feat: add call module — controller, audio sender/receiver, call UI"
```

---

### Task 11: AppCoordinator 集成中枢 — src/core/

**Files:**
- Create: `src/core/app_coordinator.h`
- Create: `src/core/app_coordinator.cpp`

将所有模块组装在一起，连接跨模块信号。

- [ ] **Step 1: 创建 `src/core/app_coordinator.h`**

```cpp
#ifndef APP_COORDINATOR_H
#define APP_COORDINATOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QVector>
#include <opencv2/core/types.hpp>

class Widget;
class GpioController;
class AudioManager;
class TcpSignaling;
class V4L2Camera;
class CameraUdpServer;
class FaceDetector;
class FaceManager;
class HailinFingerprint;
class FingerprintManager;
class PasswordManager;
class CallController;
class Form_finger;
class Form_passwd;

class AppCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit AppCoordinator(Widget* widget, QObject* parent = nullptr);
    ~AppCoordinator();

    void initialize();

signals:
    void faceRegistered();
    void fingerprintRegistered();
    void doorOpened();

private slots:
    void onCallClicked();
    void onAddClicked();
    void onPasswordClicked();
    void onPowerClicked();

    void onPersonDetected();
    void onPersonLeft();
    void onFrameUpdate();

    void onFacesDetected(const QVector<cv::Rect>& faces, int imgW, int imgH);
    void onFaceRecognizeResult(bool success, double confidence);
    void onFingerprintIdentify(bool success, int id, int score, const QString& msg);
    void onFingerprintEnroll(bool success, int id, const QString& msg);
    void onPasswordChecked(bool success);
    void onTcpMessage(const QString& msg);
    void onCallStateChanged(int state);

private:
    Widget* m_widget;
    QTimer* m_frameTimer = nullptr;

    // 硬件层
    GpioController* m_gpio = nullptr;
    AudioManager* m_audioManager = nullptr;
    AudioManager* m_connAudioManager = nullptr;
    TcpSignaling* m_tcp = nullptr;

    // 摄像头
    V4L2Camera* m_camera = nullptr;
    CameraUdpServer* m_cameraUdp = nullptr;

    // 人脸
    FaceDetector* m_faceDetector = nullptr;
    FaceManager* m_faceManager = nullptr;

    // 指纹
    HailinFingerprint* m_hailinFp = nullptr;
    FingerprintManager* m_fpManager = nullptr;

    // 密码
    PasswordManager* m_pwManager = nullptr;

    // 对讲
    CallController* m_callCtrl = nullptr;

    // 弹窗
    Form_finger* m_fingerForm = nullptr;
    Form_passwd* m_passwdForm = nullptr;

    // 状态
    bool m_hasPerson = false;
    int m_nextFingerId = 1;
    int m_frameDetectCounter = 0;
    QElapsedTimer m_faceCooldown;
    QVector<cv::Rect> m_currentFaces;
};

#endif // APP_COORDINATOR_H
```

- [ ] **Step 2: 创建 `src/core/app_coordinator.cpp`**

```cpp
#include "app_coordinator.h"
#include "widget.h"
#include "src/hardware/gpio_controller.h"
#include "src/audio/audio_manager.h"
#include "src/network/tcp_signaling.h"
#include "src/camera/v4l2_camera.h"
#include "src/camera/camera_udp_server.h"
#include "src/face/face_detector.h"
#include "src/face/face_manager.h"
#include "src/fingerprint/hailin_fingerprint.h"
#include "src/fingerprint/fingerprint_manager.h"
#include "src/fingerprint/form_finger.h"
#include "src/password/password_manager.h"
#include "src/password/form_passwd.h"
#include "src/call/call_controller.h"
#include <QTimer>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QDateTime>

AppCoordinator::AppCoordinator(Widget* widget, QObject* parent)
    : QObject(parent), m_widget(widget)
{
}

AppCoordinator::~AppCoordinator()
{
    if (m_frameTimer) { m_frameTimer->stop(); }
}

void AppCoordinator::initialize()
{
    qDebug() << "\n========== [AppCoordinator] Initializing ==========";

    // ---- Hardware ----
    m_gpio = new GpioController(this);
    m_gpio->startMonitoring();

    // ---- Audio ----
    m_audioManager = new AudioManager(this);
    m_connAudioManager = new AudioManager(this);

    // ---- Network ----
    m_tcp = new TcpSignaling("192.168.1.4", 8887, this);
    m_tcp->connectToServer();

    // ---- Camera ----
    m_camera = new V4L2Camera("/dev/video0");
    if (!m_camera->open()) {
        qWarning() << "[AppCoordinator] Camera open failed";
    }
    m_cameraUdp = new CameraUdpServer(m_camera, 50000, this);

    // ---- Face ----
    m_faceDetector = new FaceDetector(this);
    m_faceDetector->loadCascade("/root/haarcascade_frontalface_default.xml");
    m_faceManager = new FaceManager(this);

    // ---- Fingerprint ----
    m_hailinFp = new HailinFingerprint("/dev/ttyS4", this);
    if (m_hailinFp->open()) {
        qDebug() << "[AppCoordinator] fingerprint module opened";
    }
    m_fpManager = new FingerprintManager(this);
    m_fpManager->setDevice(m_hailinFp);

    // ---- Password ----
    m_pwManager = new PasswordManager(this);

    // ---- Call ----
    m_callCtrl = new CallController(this);
    m_callCtrl->setCamera(m_camera);
    m_callCtrl->setSignaling(m_tcp);
    m_callCtrl->setAudioManager(m_audioManager);
    m_callCtrl->setConnAudioManager(m_connAudioManager);
    m_callCtrl->setGpioController(m_gpio);
    m_callCtrl->setCallFormParent(m_widget,
        m_widget->findChild<QWidget*>("widget_3")->mapToGlobal(QPoint(0, 0)));

    // ==== Signal wiring ====

    // Widget buttons → Coordinator
    connect(m_widget, &Widget::btnCallClicked, this, &AppCoordinator::onCallClicked);
    connect(m_widget, &Widget::btnAddClicked, this, &AppCoordinator::onAddClicked);
    connect(m_widget, &Widget::btnPasswordClicked, this, &AppCoordinator::onPasswordClicked);
    connect(m_widget, &Widget::btnPowerClicked, this, &AppCoordinator::onPowerClicked);

    // GPIO
    connect(m_gpio, &GpioController::personDetected, this, &AppCoordinator::onPersonDetected);
    connect(m_gpio, &GpioController::personLeft, this, &AppCoordinator::onPersonLeft);

    // TCP signaling
    connect(m_tcp, &TcpSignaling::messageReceived, this, &AppCoordinator::onTcpMessage);

    // Face detection
    connect(m_faceDetector, &FaceDetector::facesDetected, this, &AppCoordinator::onFacesDetected);
    connect(m_faceManager, &FaceManager::recognizeResult, this, &AppCoordinator::onFaceRecognizeResult);
    connect(m_faceManager, &FaceManager::faceSampled, this, [this](int current, int total) {
        if (current == 1) { m_audioManager->playAsync("/tmp/pcm/faceone.pcm"); m_gpio->setFillLight(true); }
        else if (current == 2) m_audioManager->playAsync("/tmp/pcm/facetwo.pcm");
        else if (current == 3) { m_audioManager->playAsync("/tmp/pcm/facethree.pcm"); m_gpio->setFillLight(false); }
    });

    // Fingerprint
    connect(m_fpManager, &FingerprintManager::identifyResult, this, &AppCoordinator::onFingerprintIdentify);
    connect(m_fpManager, &FingerprintManager::enrollResult, this, &AppCoordinator::onFingerprintEnroll);

    // Password
    connect(m_pwManager, &PasswordManager::passwordChecked, this, &AppCoordinator::onPasswordChecked);

    // Call state
    connect(m_callCtrl, &CallController::callStateChanged, this, &AppCoordinator::onCallStateChanged);

    // ---- Start loops ----

    // Fingerprint auto-identify
    QTimer::singleShot(0, this, [this]() { m_fpManager->startIdentify(); });

    // Frame timer: 50ms = 20fps, face detect every 10 frames
    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &AppCoordinator::onFrameUpdate);
    m_frameTimer->start(50);

    qDebug() << "[AppCoordinator] initialization complete\n";
}

// ========== Button handlers ==========

void AppCoordinator::onCallClicked()
{
    m_callCtrl->startCall();
}

void AppCoordinator::onAddClicked()
{
    m_audioManager->playAsync("/tmp/pcm/regnew.pcm");
    if (!m_fingerForm) {
        m_fingerForm = new Form_finger(m_widget);
        connect(m_fingerForm, &Form_finger::addFaceRequested, this, [this]() {
            if (m_currentFaces.empty()) return;
            auto maxFace = std::max_element(m_currentFaces.begin(), m_currentFaces.end(),
                [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
            if (maxFace == m_currentFaces.end()) return;
            QImage img = m_camera->getFrame();
            if (img.isNull()) return;
            QRect qface(maxFace->x, maxFace->y, maxFace->width, maxFace->height);
            QImage faceImg = img.copy(qface).scaled(100, 100, Qt::KeepAspectRatio);
            m_faceManager->addFaceSample(faceImg);
            if (m_faceManager->sampleCount() == 3) m_faceManager->train();
        });
        connect(m_fingerForm, &Form_finger::addFingerRequested, this, [this]() {
            m_hailinFp->stepRegister(m_nextFingerId, 3);
            m_nextFingerId++;
        });
    }
    QPoint pos = m_widget->findChild<QWidget*>("widget_3")->mapToGlobal(QPoint(0, 0));
    m_fingerForm->move(pos);
    m_fingerForm->show();
    m_fingerForm->raise();
}

void AppCoordinator::onPasswordClicked()
{
    Form_passwd* pwForm = new Form_passwd(m_widget);
    pwForm->setAttribute(Qt::WA_DeleteOnClose);
    connect(pwForm, &Form_passwd::passwordEntered, m_pwManager, &PasswordManager::checkPassword);
    QPoint pos = m_widget->findChild<QWidget*>("widget_3")->mapToGlobal(QPoint(0, 0));
    pwForm->move(pos);
    pwForm->show();
    pwForm->raise();
    m_audioManager->playAsync("/tmp/pcm/inputpasswd.pcm");
}

void AppCoordinator::onPowerClicked()
{
    m_gpio->setFillLight(false);
}

// ========== GPIO ==========

void AppCoordinator::onPersonDetected()
{
    m_hasPerson = true;
}

void AppCoordinator::onPersonLeft()
{
    m_hasPerson = false;
}

// ========== Frame + Face ==========

void AppCoordinator::onFrameUpdate()
{
    QImage img = m_camera->getFrame();
    if (img.isNull()) return;

    // Async face detection every 10 frames
    if (++m_frameDetectCounter % 10 == 0) {
        m_faceDetector->detectAsync(img);
    }

    // Draw face rects + display
    QPainter painter(&img);
    painter.setPen(QPen(Qt::red, 3));
    for (const auto& face : m_currentFaces) {
        painter.drawRect(face.x, face.y, face.width, face.height);
    }
    painter.end();

    QSize labelSize(430, 475);
    QImage scaled = img.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
    QRect cropRect((scaled.width() - labelSize.width()) / 2,
                   (scaled.height() - labelSize.height()) / 2,
                   labelSize.width(), labelSize.height());
    QPixmap pix = QPixmap::fromImage(scaled.copy(cropRect));
    QLabel* labelcam = m_widget->findChild<QLabel*>("labelcam");
    if (labelcam) labelcam->setPixmap(pix);

    // Face recognition (when person detected)
    if (m_hasPerson && !m_currentFaces.empty()) {
        if (!m_faceCooldown.isValid() || m_faceCooldown.elapsed() > 2000) {
            auto maxFace = std::max_element(m_currentFaces.begin(), m_currentFaces.end(),
                [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
            if (maxFace != m_currentFaces.end()) {
                QRect qface(maxFace->x, maxFace->y, maxFace->width, maxFace->height);
                QImage faceImg = img.copy(qface).scaled(100, 100, Qt::KeepAspectRatio);
                m_faceManager->recognize(faceImg);
                m_faceCooldown.restart();
            }
        }
    } else if (m_hasPerson && m_currentFaces.empty()) {
        if (!m_faceCooldown.isValid() || m_faceCooldown.elapsed() > 2000) {
            m_audioManager->playAsync("/tmp/pcm/oncam.pcm");
            m_faceCooldown.restart();
            m_gpio->setFillLight(true);
        }
    }
}

void AppCoordinator::onFacesDetected(const QVector<cv::Rect>& faces, int imgW, int imgH)
{
    QImage img = m_camera->getFrame();
    if (img.isNull()) return;
    double scaleX = double(img.width()) / imgW;
    double scaleY = double(img.height()) / imgH;
    m_currentFaces.clear();
    for (const auto& f : faces) {
        m_currentFaces.append(cv::Rect(f.x * scaleX, f.y * scaleY,
                                        f.width * scaleX, f.height * scaleY));
    }
}

void AppCoordinator::onFaceRecognizeResult(bool success, double confidence)
{
    qDebug() << "[Face] recognize result:" << (success ? "OK" : "FAIL") << confidence;
    m_gpio->setFillLight(false);
    if (success)
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    else
        m_audioManager->playAsync("/tmp/pcm/faceerr.pcm");
}

// ========== Fingerprint ==========

void AppCoordinator::onFingerprintIdentify(bool success, int id, int score, const QString& msg)
{
    if (msg == QString::fromUtf8("请按下指纹进行识别")) return;
    if (success) {
        m_audioManager->playAsync("/tmp/pcm/fingerdoor.pcm");
    } else {
        m_audioManager->playAsync("/tmp/pcm/fingererr.pcm");
    }
    QTimer::singleShot(500, this, [this]() { m_fpManager->startIdentify(); });
}

void AppCoordinator::onFingerprintEnroll(bool success, int id, const QString& msg)
{
    if (msg.contains(QString::fromUtf8("请按下手指进行"))) {
        m_audioManager->playAsync("/tmp/pcm/caijifinger.pcm");
        return;
    }
    if (msg.contains(QString::fromUtf8("采集成功"))) {
        m_audioManager->playAsync("/tmp/pcm/fingerone.pcm");
        return;
    }
    if (success && msg == QString::fromUtf8("注册成功"))
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    else if (!success)
        m_audioManager->playAsync("/tmp/pcm/fingererr.pcm");
}

// ========== Password ==========

void AppCoordinator::onPasswordChecked(bool success)
{
    if (success) {
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
        m_passwdForm = nullptr; // Form_passwd self-deletes
    } else {
        m_audioManager->playAsync("/tmp/pcm/passwderr.pcm");
    }
}

// ========== TCP Signaling ==========

void AppCoordinator::onTcpMessage(const QString& msg)
{
    if (msg == "accept") {
        m_callCtrl->onAccept();
    } else if (msg == "recstop") {
        m_callCtrl->hangup();
    } else if (msg == "unlock") {
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    }
}

void AppCoordinator::onCallStateChanged(int state)
{
    Q_UNUSED(state);
}

```

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "feat: add AppCoordinator — central signal orchestrator for all modules"
```

---

### Task 12: 连接 main.cpp 与 CMakeLists.txt 最终适配

**Files:**
- Modify: `main.cpp` — 创建 AppCoordinator 并 initialize
- Modify: `CMakeLists.txt` — 更新 include 路径确保编译通过

- [ ] **Step 1: 更新 main.cpp**

```cpp
#include "src/core/widget.h"
#include "src/core/app_coordinator.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSurfaceFormat format;
    format.setVersion(2, 0);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    QSurfaceFormat::setDefaultFormat(format);

    Widget w;
    AppCoordinator coordinator(&w);
    w.setCoordinator(&coordinator);
    w.show();
    coordinator.initialize();

    return a.exec();
}
```

- [ ] **Step 2: 更新 CMakeLists.txt 的 include 路径**

在 `target_include_directories` 中确认根目录和 `src/` 都在路径中：

```cmake
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)
```

- [ ] **Step 3: 尝试编译**

```bash
cd build && cmake .. && make
```

修复编译错误（include 路径、缺少的 Qt 模块等）。

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "chore: wire main.cpp, finalize CMakeLists for new structure"
```

---

### Task 13: 清理旧文件

**Files:**
- Delete: `AccessTerminal.pro`（QMake 项目文件，CMake 替代）
- Delete: `README.en.md`, `README.md`（或不删，按需保留）

- [ ] **Step 1: 删除旧的 .pro 文件**

```bash
rm AccessTerminal.pro
```

- [ ] **Step 2: Commit**

```bash
git add -A
git commit -m "chore: remove old QMake .pro file, CMake is the build system"
```

---

### 实施顺序总结

```
Task 1  (目录结构)        ──┐
Task 2  (Widget瘦身)       ├─ 基础设施
Task 3  (GPIO)            ──┘
Task 4  (Audio)           ──┐
Task 5  (TCP信令)          ├─ 公用模块
Task 6  (Camera)          ──┘
Task 7  (Face)            ──┐
Task 8  (Fingerprint)      ├─ 业务模块（可并行）
Task 9  (Password)         │
Task 10 (Call)            ──┘
Task 11 (Coordinator)     ── 集成
Task 12 (构建适配)         ── 收尾
Task 13 (清理)            ── 收尾
```
