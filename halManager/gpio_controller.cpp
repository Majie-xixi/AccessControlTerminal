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
