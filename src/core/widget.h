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
