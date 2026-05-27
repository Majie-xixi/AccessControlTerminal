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
core/widget.cpp
core/app_coordinator.cpp
)

set(CAMERA_SOURCES
camera/v4l2_camera.cpp
camera/camera_udp_server.cpp
camera/camera_push_thread.cpp
)

set(FACE_SOURCES
face/face_manager.cpp
face/face_detector.cpp
)

set(FINGERPRINT_SOURCES
fingerprint/hailin_fingerprint.cpp
fingerprint/fingerprint_manager.cpp
fingerprint/form_finger.cpp
)

set(PASSWORD_SOURCES
password/form_passwd.cpp
password/password_manager.cpp
)

set(CALL_SOURCES
call/form_call.cpp
call/audio_sender.cpp
call/audio_receiver.cpp
call/call_controller.cpp
)

set(AUDIO_SOURCES
audio/audio_manager.cpp
)

set(NETWORK_SOURCES
network/tcp_signaling.cpp
)

set(HARDWARE_SOURCES
hardware/gpio_controller.cpp
)

set(ALL_SOURCES
    main.cpp
    ${CORE_SOURCES}
    ${CAMERA_SOURCES}
    ${FACE_SOURCES}
    ${FINGERPRINT_SOURCES}
    ${PASSWORD_SOURCES}
    ${CALL_SOURCES}
    ${AUDIO_SOURCES}
    ${NETWORK_SOURCES}
    ${HARDWARE_SOURCES}
)

set(ALL_HEADERS
core/widget.h
core/app_coordinator.h
camera/v4l2_camera.h
camera/camera_udp_server.h
camera/camera_push_thread.h
face/face_manager.h
face/face_detector.h
fingerprint/hailin_fingerprint.h
fingerprint/fingerprint_manager.h
fingerprint/form_finger.h
password/form_passwd.h
password/password_manager.h
call/form_call.h
call/audio_sender.h
call/audio_receiver.h
call/call_controller.h
audio/audio_manager.h
network/tcp_signaling.h
hardware/gpio_controller.h
)

set(UI_FILES
ui/widget.ui
ui/form_finger.ui
ui/form_passwd.ui
ui/form_call.ui
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
