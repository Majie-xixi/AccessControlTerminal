# AccessControlTerminal

A smart access control terminal based on Allwinner T113 + Qt5.

## Features

| Module | Description |
|--------|-------------|
| Face Recognition | V4L2 camera capture + OpenCV face detection + LBPH recognition algorithm, supports registration and real-time matching |
| Fingerprint Recognition | Hailin fingerprint module via UART serial, supports 1:N auto-identify and auto-enroll |
| Password | Local password verification with configurable passcode |
| Video Intercom | TCP signaling + ALSA audio capture/playback + UDP camera streaming, supports two-way calls between terminal and indoor unit |
| Hardware Control | GPIO-controlled PIR sensor (PE4), fill light (PE5), and relay door lock |

## Hardware Platform

- **SoC**: Allwinner T113 (ARM Cortex-A7)
- **OS**: Tina Linux (OpenWrt-based)
- **Peripherals**:
  - USB camera (V4L2)
  - Hailin fingerprint module (UART, `/dev/ttyS4`)
  - PIR motion sensor (PE4)
  - Fill light (PE5)
  - Relay / electromagnetic lock
  - Microphone + speaker (ALSA)

## Architecture

```
main.cpp
├── app/                      # Application layer
│   ├── Widget (main UI)
│   └── AppCoordinator (module coordinator)
├── cameraManager/            # Camera & face recognition
│   ├── V4L2Camera (V4L2 capture)
│   ├── FaceDetector (face detection)
│   ├── FaceManager (LBPH training & recognition)
│   ├── CameraUdpServer (UDP video streaming)
│   └── CameraPushThread (encoding & push thread)
├── fingerprintManager/       # Fingerprint
│   ├── HailinFingerprint (serial protocol)
│   ├── FingerprintManager (business logic)
│   └── Form_finger (enrollment UI)
├── passwordManager/          # Password
│   ├── PasswordManager (verification)
│   └── FormPassword (input UI)
├── audioManager/             # Audio & intercom
│   ├── AudioManager (ALSA capture/playback)
│   ├── AudioSenderThread (audio TX)
│   ├── AudioReceiverThread (audio RX)
│   ├── CallController (call state machine)
│   └── FormIntercom (intercom UI)
├── networkManager/           # Networking
│   └── TcpSignaling (TCP signaling)
└── halManager/               # Hardware abstraction
    └── GpioController (GPIO control)
```

## Dependencies

| Dependency | Notes |
|------------|-------|
| Qt 5.12+ | Core, Gui, Widgets, Network, SerialPort, Concurrent |
| OpenCV 4.x | core, imgproc, objdetect, face |
| ALSA | libasound (audio) |
| libgpiod | GPIO control |
| pthread | Multithreading |

## Build

Both qmake (`.pro`) and CMake are supported.

### qmake

```bash
mkdir build && cd build
qmake ../AccessTerminal.pro
make -j$(nproc)
```

### CMake

```bash
mkdir build && cd build
cmake .. -DOpenCV_DIR=/path/to/opencv
make -j$(nproc)
```

### T113 Cross-Compilation

Edit `CMakeLists.txt` to set the cross-compiler paths and uncomment:

```cmake
set(CMAKE_C_COMPILER   arm-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER arm-openwrt-linux-g++)
set(CMAKE_SYSROOT      /path/to/tina-sdk/staging_dir/target)
```

## Running

```bash
./AccessTerminal
```

The terminal stays in standby mode and detects approaching persons via PIR sensor. It supports three access methods (face / fingerprint / password) and one-touch video intercom calls to the indoor unit.
