#include "panel_layout_settings.h"
#include "ui_panel_layout_settings.h"

PanelLayoutSettings::PanelLayoutSettings(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::PanelLayoutSettings)
{
    ui->setupUi(this);
}

PanelLayoutSettings::~PanelLayoutSettings()
{
    delete ui;
}
