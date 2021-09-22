#include "settings_dialog.h"
#include "ui_settings_dialog.h"

#include "common_ui_tools.h"
#include "trace_x/trace_x.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    X_CALL;

    ui->setupUi(this);

    ui->section_list_widget->setItemDelegate(new TreeItemDelegate(this));
    connect(ui->section_list_widget, &QListWidget::currentRowChanged, this, &SettingsDialog::update_section);

    ui->stacked_widget->setContentsMargins(QMargins());

    connect(ui->button_box, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(ui->button_box->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::save);
}

SettingsDialog::~SettingsDialog()
{
    X_CALL;

    delete ui;
}

void SettingsDialog::add_settings_section(QWidget *section_widget)
{
    X_CALL;

    ui->stacked_widget->addWidget(section_widget);
    ui->section_list_widget->addItem(new QListWidgetItem(section_widget->windowIcon(), section_widget->windowTitle()));

    ui->section_list_widget->setMinimumWidth(ui->section_list_widget->sizeHintForColumn(0) + 30);

    connect(this, SIGNAL(save()), section_widget, SLOT(save()));
    connect(this, SIGNAL(rejected()), section_widget, SLOT(restore()));
}

void SettingsDialog::update_section()
{
    X_CALL;

    ui->stacked_widget->setCurrentIndex(ui->section_list_widget->currentRow());
}
