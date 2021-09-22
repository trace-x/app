#include "general_setting_widget.h"
#include "ui_general_setting_widget.h"

#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>

#include "settings.h"
#include "trace_x/trace_x.h"

GeneralSettingWidget::GeneralSettingWidget(TraceController *controller, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::GeneralSettingWidget),
    _controller(controller)
{
    X_CALL;

    ui->setupUi(this);

    restore();
}

GeneralSettingWidget::~GeneralSettingWidget()
{
    X_CALL;

    delete ui;
}

void GeneralSettingWidget::save()
{
    X_CALL;

    x_settings().message_limit_option->set_value(ui->message_limit_spinbox->value());
    x_settings().memory_limit_option->set_value(ui->memory_limit_spinbox->value());
    x_settings().swap_limit_option->set_value(ui->swap_limit_spinbox->value());
    x_settings().file_data_limit_option->set_value(ui->file_limit_spinbox->value());
    x_settings().no_swap_option->set_value(!ui->use_swap_checkbox->isChecked());
    x_settings().swap_path_option->init_value(ui->swap_path_line_edit->text());
}

void GeneralSettingWidget::restore()
{
    X_CALL;

    ui->message_limit_spinbox->setValue(x_settings().message_limit_option->uint_value());
    ui->memory_limit_spinbox->setValue(x_settings().memory_limit_option->uint_value());
    ui->swap_limit_spinbox->setValue(x_settings().swap_limit_option->uint_value());
    ui->file_limit_spinbox->setValue(x_settings().file_data_limit_option->uint_value());
    ui->use_swap_checkbox->setChecked(!x_settings().no_swap_option->bool_value());
    ui->swap_path_line_edit->setText(x_settings().swap_path_option->string_value());
}
