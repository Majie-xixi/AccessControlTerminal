QT       += core gui widgets network serialport concurrent

CONFIG   += c++11

# ==================== ALSA ====================
# T113 交叉编译时启用:
# LIBS += -lasound
# LIBS += -lpthread

# ==================== OpenCV4 ====================
# T113 交叉编译时取消注释:
# INCLUDEPATH += /home/meetyoo/opencv_arm_build/_install/include/opencv4
# LIBS += -L/home/meetyoo/opencv_arm_build/_install/lib
# LIBS += -lopencv_core -lopencv_imgproc -lopencv_objdetect -lopencv_face
# LIBS += -lopencv_highgui -lopencv_imgcodecs

# ==================== GPIO ====================
# LIBS += -lgpiod
# DEFINES += HAS_LIBGPIOD

# ==================== ALSA 桌面开发（可选） ====================
# mac: LIBS += -framework CoreAudio
# win32: LIBS += -lwinmm

SOURCES += \
    main.cpp \
    app/widget.cpp \
    app/app_coordinator.cpp \
    cameraManager/v4l2_camera.cpp \
    cameraManager/camera_udp_server.cpp \
    cameraManager/camera_push_thread.cpp \
    cameraManager/face_detector.cpp \
    cameraManager/face_manager.cpp \
    fingerprintManager/hailin_fingerprint.cpp \
    fingerprintManager/fingerprint_manager.cpp \
    fingerprintManager/form_finger.cpp \
    passwordManager/form_password.cpp \
    passwordManager/password_manager.cpp \
    audioManager/audio_manager.cpp \
    audioManager/audio_sender.cpp \
    audioManager/audio_receiver.cpp \
    audioManager/call_controller.cpp \
    audioManager/form_intercom.cpp \
    networkManager/tcp_signaling.cpp \
    halManager/gpio_controller.cpp

HEADERS += \
    app/widget.h \
    app/app_coordinator.h \
    cameraManager/v4l2_camera.h \
    cameraManager/camera_udp_server.h \
    cameraManager/camera_push_thread.h \
    cameraManager/face_detector.h \
    cameraManager/face_manager.h \
    fingerprintManager/hailin_fingerprint.h \
    fingerprintManager/fingerprint_manager.h \
    fingerprintManager/form_finger.h \
    passwordManager/form_password.h \
    passwordManager/password_manager.h \
    audioManager/audio_manager.h \
    audioManager/audio_sender.h \
    audioManager/audio_receiver.h \
    audioManager/call_controller.h \
    audioManager/form_intercom.h \
    networkManager/tcp_signaling.h \
    halManager/gpio_controller.h

FORMS += \
    ui/widget.ui \
    ui/form_finger.ui \
    ui/form_password.ui \
    ui/form_intercom.ui

RESOURCES += \
    res.qrc

INCLUDEPATH += $$PWD

TARGET = AccessTerminal
