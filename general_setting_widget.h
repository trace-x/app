#ifndef GENERAL_SETTING_WIDGET_H
#define GENERAL_SETTING_WIDGET_H

#include <QFrame>

#include "trace_controller.h"

namespace Ui {
class GeneralSettingWidget;
}

class GeneralSettingWidget : public QFrame
{
    Q_OBJECT

public:
    explicit GeneralSettingWidget(TraceController *controller, QWidget *parent = 0);
    ~GeneralSettingWidget();

public slots:
    void save();
    void restore();

private:
    Ui::GeneralSettingWidget *ui;

    TraceController *_controller;
};

#endif // GENERAL_SETTING_WIDGET_H
