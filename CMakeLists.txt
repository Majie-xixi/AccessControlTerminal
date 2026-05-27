cmake_minimum_required(VERSION 3.16)
project(AccessTerminal VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)



# ==================== Qt5 ====================
set(CMAKE_PREFIX_PATH "$ENV{QT_HOME}" ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.12.2/5.12.2/mingw73_32")
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network

    SerialPort
    Concurrent
)

# ==================== OpenCV4 ====================
# T113 交叉编译时替换为: /home/meetyoo/opencv_arm_build/_install
set(OpenCV_DIR "" CACHE PATH "OpenCV install directory")
find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    objdetect
    face
)

# ==================== ALSA ====================
find_library(ASOUND_LIB asound REQUIRED)

# ==================== GPIO ====================
find_library(GPIOD_LIB gpiod)

# ==================== 源文件 ====================
set(CORE_SOURCES
app/widget.cpp
app/app_coordinator.cpp
)

set(CAMERA_SOURCES
videoManager/v4l2_camera.cpp
videoManager/camera_udp_server.cpp
videoManager/camera_push_thread.cpp
)

set(FACE_SOURCES
faceManager/face_manager.cpp
faceManager/face_detector.cpp
)

set(FINGERPRINT_SOURCES
fingerprintManager/hailin_fingerprint.cpp
fingerprintManager/fingerprint_manager.cpp
fingerprintManager/form_finger.cpp
)

set(PASSWORD_SOURCES
passwordManager/form_password.cpp
passwordManager/password_manager.cpp
)

set(CALL_SOURCES
audioManager/form_intercom.cpp
audioManager/audio_sender.cpp
audioManager/audio_receiver.cpp
audioManager/call_controller.cpp
    audioManager/audio_manager.cpp
)


set(NETWORK_SOURCES
networkManager/tcp_signaling.cpp
)

set(HARDWARE_SOURCES
halManager/gpio_controller.cpp
)

set(ALL_SOURCES
    main.cpp
    ${CORE_SOURCES}
    ${CAMERA_SOURCES}
    ${FACE_SOURCES}
    ${FINGERPRINT_SOURCES}
    ${PASSWORD_SOURCES}
    ${CALL_SOURCES}
    
    ${NETWORK_SOURCES}
    ${HARDWARE_SOURCES}
)

set(ALL_HEADERS
app/widget.h
app/app_coordinator.h
videoManager/v4l2_camera.h
videoManager/camera_udp_server.h
videoManager/camera_push_thread.h
faceManager/face_manager.h
faceManager/face_detector.h
fingerprintManager/hailin_fingerprint.h
fingerprintManager/fingerprint_manager.h
fingerprintManager/form_finger.h
passwordManager/form_password.h
passwordManager/password_manager.h
audioManager/form_intercom.h
audioManager/audio_sender.h
audioManager/audio_receiver.h
audioManager/call_controller.h
audioManager/audio_manager.h
networkManager/tcp_signaling.h
halManager/gpio_controller.h
)

set(UI_FILES
ui/widget.ui
ui/form_finger.ui
ui/form_password.ui
ui/form_intercom.ui
)

# ==================== 可执行文件 ====================
add_executable(${PROJECT_NAME}
    ${ALL_SOURCES}
    ${ALL_HEADERS}
    ${UI_FILES}
    res.qrc
)

# ==================== Include 路径 ====================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

# ==================== 链接库 ====================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::SerialPort
    Qt5::Concurrent
    ${OpenCV_LIBS}
    ${ASOUND_LIB}
    pthread
)

# GPIO 为可选依赖（桌面开发可能没有 libgpiod）
if(GPIOD_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GPIOD_LIB})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_LIBGPIOD)
endif()

# ==================== 编译选项 ====================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DEPRECATED_WARNINGS
)

# T113 交叉编译时取消注释:
# set(CMAKE_C_COMPILER   arm-openwrt-linux-gcc)
# set(CMAKE_CXX_COMPILER arm-openwrt-linux-g++)
# set(CMAKE_SYSROOT      /home/meetyoo/t113/Tina-Linux/out/t113-bingpi_m2/staging_dir/target)
