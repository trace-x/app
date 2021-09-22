#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    void add_settings_section(QWidget *section_widget);
    void update_section();

signals:
    void save();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGS_DIALOG_H
