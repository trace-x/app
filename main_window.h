#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QLabel>
#include <QSplitter>
#include <QFileDialog>

#include "trace_service.h"
#include "tx_filter_widget.h"
#include "session_manager.h"
#include "settings_dialog.h"
#include "about_dialog.h"

#include "general_setting_widget.h"
#include "source_mapping_widget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    void crash();
    void self_debug();
    void show_trace();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    template<class T>
    void save_widget_state(T *widget);
    void save_widget_geometry(QWidget *widget);

    template<class T>
    void restore_widget_state(T *widget);
    void restore_widget_geometry(QWidget *widget);

    void save_session(Session &session);

private slots:
    void update_server_state(bool is_enabled);

    void save_settings();
    void load_settings();
    void switch_session(const Session &session);
    void update_app_title();

    void save_trace();
    void load_trace();
    void show_gui();

private:
    Ui::MainWindow *ui;
    TraceService _trace_service;
    SessionManager *_session_manager;

    AboutDialog _about_dialog;
    QFileDialog *_file_dialog;

    TxFilterWidget *_capture_filter_widget;
    SettingsDialog _settings_dialog;

    SourceMappingWidget _source_mapping_settings;
    GeneralSettingWidget *_general_settings;
};

#endif // MAIN_WINDOW_H
