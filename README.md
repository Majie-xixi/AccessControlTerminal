# AccessControlTerminal

基于全志 T113 + Qt5 的智能门禁终端系统。

## 功能

| 模块 | 说明 |
|------|------|
| 人脸识别 | V4L2 摄像头采集 + OpenCV 人脸检测 + LBPH 识别算法，支持注册与实时比对 |
| 指纹识别 | 海凌指纹模块，UART 串口通信，支持 1:N 自动识别与自动注册 |
| 密码识别 | 本地密码验证，可自定义密码 |
| 可视对讲 | 基于 TCP 信令 + ALSA 音频采集播放 + UDP 摄像头推流，支持终端呼叫室内机双向通话 |
| 硬件控制 | GPIO 控制人体红外检测（PE4）、补光灯（PE5）、继电器开门 |

## 硬件平台

- **主控**: 全志 T113 (ARM Cortex-A7)
- **系统**: Tina Linux (OpenWrt 分支)
- **外设**:
  - USB 摄像头 (V4L2)
  - 海凌指纹模块 (UART, `/dev/ttyS4`)
  - 人体红外传感器 (PE4)
  - 补光灯 (PE5)
  - 继电器/电磁锁
  - 麦克风 + 扬声器 (ALSA)

## 软件架构

```
main.cpp
├── app/                      # 应用层
│   ├── Widget (主界面)
│   └── AppCoordinator (模块协调器)
├── cameraManager/            # 摄像头与人脸模块
│   ├── V4L2Camera (V4L2 采集)
│   ├── FaceDetector (人脸检测)
│   ├── FaceManager (LBPH 训练与识别)
│   ├── CameraUdpServer (UDP 视频推流)
│   └── CameraPushThread (编码推流线程)
├── fingerprintManager/       # 指纹模块
│   ├── HailinFingerprint (串口协议)
│   ├── FingerprintManager (指纹业务逻辑)
│   └── Form_finger (指纹录入界面)
├── passwordManager/          # 密码模块
│   ├── PasswordManager (密码验证)
│   └── FormPassword (密码输入界面)
├── audioManager/             # 音频与对讲模块
│   ├── AudioManager (ALSA 采集/播放)
│   ├── AudioSenderThread (音频发送)
│   ├── AudioReceiverThread (音频接收)
│   ├── CallController (通话状态机)
│   └── FormIntercom (对讲界面)
├── networkManager/           # 网络通信
│   └── TcpSignaling (TCP 信令)
└── halManager/               # 硬件抽象层
    └── GpioController (GPIO 控制)
```

## 依赖

| 依赖 | 说明 |
|------|------|
| Qt 5.12+ | Core, Gui, Widgets, Network, SerialPort, Concurrent |
| OpenCV 4.x | core, imgproc, objdetect, face |
| ALSA | libasound (音频) |
| libgpiod | GPIO 控制 |
| pthread | 多线程 |

## 编译

项目同时提供了 qmake (`.pro`) 和 CMake 两种构建方式。

### qmake 构建

```bash
mkdir build && cd build
qmake ../AccessTerminal.pro
make -j$(nproc)
```

### CMake 构建

```bash
mkdir build && cd build
cmake .. -DOpenCV_DIR=/path/to/opencv
make -j$(nproc)
```

### T113 交叉编译

修改 `CMakeLists.txt` 中交叉编译器路径并取消注释：

```cmake
set(CMAKE_C_COMPILER   arm-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER arm-openwrt-linux-g++)
set(CMAKE_SYSROOT      /path/to/tina-sdk/staging_dir/target)
```

## 运行

```bash
./AccessTerminal
```

启动后终端进入待机状态，通过人体红外自动检测来人，支持人脸/指纹/密码三种开门方式，以及一键呼叫室内机进行可视对讲。
