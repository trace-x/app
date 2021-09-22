#ifndef PANEL_LAYOUT_SETTINGS_H
#define PANEL_LAYOUT_SETTINGS_H

#include <QFrame>

namespace Ui {
class PanelLayoutSettings;
}

class PanelLayoutSettings : public QFrame
{
    Q_OBJECT

public:
    explicit PanelLayoutSettings(QWidget *parent = 0);
    ~PanelLayoutSettings();

private:
    Ui::PanelLayoutSettings *ui;
};

#endif // PANEL_LAYOUT_SETTINGS_H
