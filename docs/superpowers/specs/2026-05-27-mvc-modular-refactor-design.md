# SmartPro 终端重构设计：Manager 模式 + 功能模块化

## 目标

将 SmartPro（终端-T113）中已跑通的功能代码，基于模板（access_control_terminal）的 UI 骨架，重新组织为分层的模块化结构，取代当前的 God Object 模式。

## 当前问题

- `widget.cpp` 757 行承担所有逻辑，无分层
- 文件平铺在根目录，零分类
- 跨层调用：Form 通过 parentWidget() 穿透到 Widget 直接调 Manager
- 指纹注册逻辑在 FingerprintManager 和 Form_finger 各写一遍
- CameraUdpServer include 拼写错误 "wdiget.h"

## 目标目录结构

```
access_control_terminal/
├── AccessTerminal.pro
├── main.cpp
├── res.qrc
├── pic/                          # UI 图片资源
│
├── src/
│   ├── core/                     # 系统主控
│   │   ├── widget.h/cpp/ui        # 主窗口容器+路由（目标 <200行）
│   │   └── app_coordinator.h/cpp  # 信号编排协调器
│   │
│   ├── camera/                   # 摄像头模块
│   │   ├── v4l2_camera.h/cpp
│   │   ├── camera_udp_server.h/cpp
│   │   └── camera_push_thread.h/cpp
│   │
│   ├── face/                     # 人脸模块
│   │   ├── face_manager.h/cpp     # 采集/训练/识别
│   │   └── face_detector.h/cpp    # Haar 分类器封装
│   │
│   ├── fingerprint/              # 指纹模块
│   │   ├── hailin_fingerprint.h/cpp  # 串口驱动
│   │   ├── fingerprint_manager.h/cpp # 注册/识别编排
│   │   └── form_finger.h/cpp/ui      # 注册UI
│   │
│   ├── password/                 # 密码模块
│   │   ├── form_passwd.h/cpp/ui
│   │   └── password_manager.h/cpp
│   │
│   ├── call/                     # 对讲模块
│   │   ├── form_call.h/cpp/ui
│   │   ├── audio_sender.h/cpp     # ALSA 采集 + UDP 发送
│   │   ├── audio_receiver.h/cpp   # UDP 接收 + ALSA 播放
│   │   └── call_controller.h/cpp  # 呼叫流程状态机
│   │
│   ├── audio/                    # 本地音频播放（公用）
│   │   └── audio_manager.h/cpp
│   │
│   ├── network/                  # 通信（公用）
│   │   └── tcp_signaling.h/cpp   # TCP 信令客户端
│   │
│   └── hardware/                 # 硬件抽象（公用）
│       └── gpio_controller.h/cpp  # PE4/PE5 GPIO 封装
```

## 模块职责边界

### core/ — 系统主控

| 类 | 职责 | 来源 |
|---|---|---|
| `Widget` | 主窗口容器，加载背景和按钮，按钮点击转发到 Coordinator | 模板 Widget 为基础 |
| `AppCoordinator` | 模块初始化、跨模块信号编排、系统状态协调 | SmartPro widget 构造函数逻辑的归宿 |

### 其他模块对外接口

| 模块 | 关键出站信号 | 内部 |
|---|---|---|
| `face/` | `faceDetected(faces)`, `recognizeResult(success, confidence)`, `trainFinished(success)` | FaceDetector 管 Haar，FaceManager 管 LBPH |
| `fingerprint/` | `identifyResult(success, id)`, `enrollResult(success, id)` | HailinFingerprint 纯驱动，FingerprintManager 编排 |
| `password/` | `passwordChecked(success)` | 直接保留 |
| `call/` | `callStateChanged(state)` | CallController idle→calling→connected→hungup |
| `audio/` | `playbackFinished(file)` | 公用，被所有模块使用 |
| `network/` | `messageReceived(msg)` | connect/send/receive/auto-reconnect |
| `hardware/` | `personDetected()`, `personLeft()` | PE4 人体感应，PE5 补光灯 |

### 依赖规则

- 同层模块不直接依赖，通过 Coordinator 信号转发
- `audio/` 和 `network/` 是公用基础模块，可被多处引用
- `hardware/` 仅被 Coordinator 引用，再向下分发事件

## 关键流程信号编排

### 1. 指纹识别（后台常驻循环）

```
hardware → Coordinator → fingerprint::FingerprintManager::startIdentify()
                                    │ (500ms轮询)
                                    ↓
                              identifyResult(success, id)
                                    │
                              Coordinator → audio::playAsync(fingerdoor/fingererr)
                                    │ (500ms后自动重新 startIdentify)
```

### 2. 呼叫对讲

```
用户按呼叫键 → Widget → Coordinator::onCallRequested()
  → audio 播 "calling.pcm" → "conn.pcm"
  → network::TcpSignaling 发 "call\n"
  → call::FormCall 弹出

收到 accept → Coordinator::onCallAccepted()
  → camera::CameraPushThread 启动
  → call::AudioSender 启动
  → 400ms后 call::AudioReceiver 启动
  → FormCall 显示通话计时

收到 recstop / 挂断 → Coordinator::onCallHangup()
  → 停止所有 call/ 线程，关闭 FormCall，恢复 GpioController
```

### 3. 人脸识别

```
GpioController::personDetected()
  → Coordinator 拉取 V4L2Camera 帧
  → face::FaceDetector 异步 Haar 检测
  → faceDetected(faces)
  → 有脸: face::FaceManager.recognize(cropFace)
    → recognizeResult: 成功播 opendoor / 失败播 faceerr
  → 无人脸 + cooldown: audio 播 oncam.pcm
```

## 实施策略

1. **以模板为基础起点**（干净的 UI 骨架，无业务代码）
2. **从 SmartPro 逐个迁移模块**，每个模块完成后再做下一个
3. **优先顺序**：hardware → audio → network（公用基础模块）→ face → fingerprint → password → call（业务模块）→ core（集成串联）
4. **每完成一个模块进行编译验证**
5. **删除 SmartPro 中的重复代码**，如 fingerprint 注册逻辑合并为 FingerprintManager 唯一实现

## 不做的

- 不引入额外的抽象层/接口（过度设计）
- 不改动 OpenCV、ALSA、gpiod 底层驱动代码本身（只挪文件、改命名）
- 不修改协议格式（TCP/UDP 通信包保持兼容）
- 不新增功能、不删除 `zeng_sim.onnx` 预留
