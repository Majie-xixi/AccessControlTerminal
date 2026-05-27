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
