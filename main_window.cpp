#include "main_window.h"
#include "ui_main_window.h"

#include <QStandardItemModel>
#include <QScreen>
#include <QShortcut>
#include <QToolButton>

#include "common_ui_tools.h"
#include "settings.h"

#include "trace_x/trace_x.h"

namespace
{
static QString TxFilterSettings = "tx_filters";
static QString RxFilterSettings = "rx_filters";
}

void MainWindow::crash()
{
    X_CALL;

    int *dead_ptr = (int *)(0xDEAD);

    delete dead_ptr;
}

void MainWindow::self_debug()
{
    QProcess::startDetached(QApplication::applicationFilePath(), QStringList() << "-d");
}

void MainWindow::show_trace()
{
    trace_x::show_trace_receiver();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _trace_service(),
    _settings_dialog(this),
    _about_dialog(this)
{
    X_CALL;

#ifdef Q_OS_UNIX
    setWindowIcon(QIcon(":/icons/callstak_logo"));
#endif

    // QTBUG: cause QObject: Cannot create children for a parent that is in a different thread.
    _file_dialog = new QFileDialog(0);

    _general_settings = new GeneralSettingWidget(&_trace_service.trace_controller(), this);

    {
        QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_C), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::crash); // :-D
    }

    {
        QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_D), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::self_debug);
    }

    {
        QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_S), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::show_trace);
    }

    //

    setObjectName("main_window");

    if(!x_settings().no_show_option->is_command_line_set())
    {
        connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::save_settings);
    }

    //

    qRegisterMetaType<FilterItem>("FilterItem");
    qRegisterMetaTypeStreamOperators<FilterItem>("FilterItem");
    qRegisterMetaType<FilterGroup>("FilterGroup");
    qRegisterMetaTypeStreamOperators<FilterGroup>("FilterGroup");

    //

    ui->setupUi(this);

    ui->action_save->setShortcut(QKeySequence::Save);
    ui->action_open->setShortcut(QKeySequence::Open);
    ui->action_exit->setShortcut(QKeySequence::Quit);

    //

    connect(&_source_mapping_settings, &SourceMappingWidget::map_list_changed, &_trace_service.trace_controller().trace_model_service(), &TraceModelService::set_source_map_list);

    _settings_dialog.add_settings_section(_general_settings);
    _settings_dialog.add_settings_section(&_source_mapping_settings);

    //

    _session_manager = new SessionManager(ui->menu_trace, this);

    ui->menu_trace->insertMenu(ui->action_session_manager, _session_manager->session_menu());

    connect(_session_manager, &SessionManager::request_session_settings, this, &MainWindow::save_session);
    connect(_session_manager, &SessionManager::session_changed, this, &MainWindow::switch_session);
    connect(_session_manager, &SessionManager::session_name_changed, this, &MainWindow::update_app_title);

    //

    connect(ui->action_clear, &QAction::triggered, &_trace_service.trace_controller(), &TraceController::clear);
    connect(ui->action_clean_on_new_connect, &QAction::toggled, x_settings().auto_clean_option, &SettingsOption::set_bool_value);
    connect(ui->action_capture, &QAction::triggered, &_trace_service, &TraceService::set_server_state);
    connect(ui->action_exit, &QAction::triggered, qApp, &QApplication::quit);
    connect(ui->action_session_manager, &QAction::triggered, _session_manager, &SessionManager::show);
    connect(ui->action_save, &QAction::triggered, this, &MainWindow::save_trace);
    connect(ui->action_open, &QAction::triggered, this, &MainWindow::load_trace);
    connect(ui->action_settings, &QAction::triggered, &_settings_dialog, &QDialog::exec);
    connect(ui->action_about, &QAction::triggered, &_about_dialog, &QDialog::show);

    connect(&_trace_service.trace_server(), &TraceServer::state_changed, this, &MainWindow::update_server_state);
    connect(&_trace_service.trace_controller(), &TraceController::trace_source_changed, this, &MainWindow::update_app_title);
    connect(&_trace_service.trace_controller(), &TraceController::show_gui, this, &MainWindow::show_gui);

    //

    _capture_filter_widget = new TxFilterWidget(_trace_service.trace_controller().tx_model_service(), this);
    _capture_filter_widget->setObjectName("capture_filter_widget");
    _capture_filter_widget->setWindowFlags(Qt::Dialog);

    //

    _file_dialog->setDefaultSuffix("trace_x");
    _file_dialog->setNameFilters(QStringList() << tr("Trace-X Storage Files (*.trace_x)"));
    _file_dialog->setFileMode(QFileDialog::AnyFile);
    _file_dialog->setViewMode(QFileDialog::Detail);
    _file_dialog->setOption(QFileDialog::DontUseNativeDialog, false);

    //

    ui->menu_bar->hide();

    ui->main_trace_view->set_common_actions(ui->action_capture, ui->action_capture_filter, ui->action_clear);
    ui->main_trace_view->set_menu(ui->menu_trace, ui->menu_help);

    //

    load_settings();

    //

    connect(ui->action_capture_filter, &QAction::triggered, _capture_filter_widget, &QWidget::show);

    //

    // TODO restore state

    if(x_settings().no_show_option->is_command_line_set())
    {
        _trace_service.set_server_state(true);

        qApp->setQuitOnLastWindowClosed(false);
    }
    else
    {
        show_gui();

        //

        if(QFileInfo(x_settings().cl_parser.positionalArguments().value(0, "")).exists())
        {
            _trace_service.trace_controller().load_trace(x_settings().cl_parser.positionalArguments().first());
        }
    }
}

MainWindow::~MainWindow()
{
    X_CALL;

    delete ui;
    delete _file_dialog;
}

void MainWindow::update_server_state(bool is_enabled)
{
    X_CALL;

    ui->action_capture->setChecked(is_enabled);
}

void MainWindow::save_settings()
{
    X_CALL;

    save_widget_state(this);
    save_widget_geometry(_capture_filter_widget);

    //TODO main_trace_view_geometry - non command line config

    x_settings().set_value("main_trace_view_geometry", ui->main_trace_view->save_state());
    x_settings().set_value("session_state", _session_manager->save_state());
    x_settings().set_value("file_dialog_state", _file_dialog->saveState());
    x_settings().set_value("file_dialog_directory", _file_dialog->directory().absolutePath());
    x_settings().set_value("source_mapping", _source_mapping_settings.save_state());
}

void MainWindow::load_settings()
{
    X_CALL;

    _session_manager->restore_state(x_settings().value("session_state").toByteArray());

    _trace_service.trace_controller().tx_model_service().restore_state(_session_manager->current_session().capture_filter);
    _trace_service.trace_controller().trace_model_service().restore_state(_session_manager->current_session().trace_filter);

    ui->main_trace_view->initialize(&_trace_service, _session_manager->current_session().main_view_filter);

    ui->main_trace_view->restore_state(x_settings().value("main_trace_view_geometry").toByteArray());

    _source_mapping_settings.restore_state(x_settings().value("source_mapping").toByteArray());

    _trace_service.trace_controller().trace_model_service().set_source_map_list(_source_mapping_settings.path_list());

    // ui settings

    if(!x_settings().config().allKeys().isEmpty())
    {
        restore_widget_state(this);

        restore_widget_geometry(_capture_filter_widget);

        _capture_filter_widget->restore();

        _file_dialog->restoreState(x_settings().value("file_dialog_state").toByteArray());
        _file_dialog->setDirectory(x_settings().value("file_dialog_directory").toString());
    }
    else
    {
        if(QScreen *screen = qApp->primaryScreen())
        {
            QRect geometry;

            X_VALUE(screen->physicalSize());
            X_VALUE(screen->physicalDotsPerInch());

            geometry.setSize(QSizeF(screen->logicalDotsPerInchX() * 6.0, screen->logicalDotsPerInchY() * 3.0).toSize());
            geometry.moveCenter(screen->geometry().center());

            _capture_filter_widget->setGeometry(geometry);
        }
    }

    ui->action_clean_on_new_connect->setChecked(x_settings().auto_clean_option->bool_value());

    update_app_title();
}

void MainWindow::switch_session(const Session &session)
{
    X_CALL;

    _trace_service.trace_controller().tx_model_service().restore_state(session.capture_filter);
    _trace_service.trace_controller().trace_model_service().restore_state(session.trace_filter);

    ui->main_trace_view->restore_filter_state(session.main_view_filter);

    ui->main_trace_view->update_trace_filter();

    update_app_title();
}

void MainWindow::update_app_title()
{
    X_CALL;

    QString window_title = QApplication::applicationDisplayName();

    if(_trace_service.trace_server().is_started())
    {
        window_title += QString(" [%1]").arg(_trace_service.trace_server().server_name());
    }

    if(_session_manager->current_session().name != tr("Default"))
    {
        window_title += QString(" - %1").arg(_session_manager->current_session().name);
    }

    if(!_trace_service.trace_controller().loaded_file().isEmpty())
    {
        window_title += QString(" - %1").arg(QFileInfo(_trace_service.trace_controller().loaded_file()).fileName());
    }

    setWindowTitle(window_title);
}

void MainWindow::save_trace()
{
    X_CALL;

    _file_dialog->setAcceptMode(QFileDialog::AcceptSave);
    _file_dialog->setWindowTitle(tr("Save Trace"));
    _file_dialog->setNameFilters(QStringList() << tr("Trace-X Storage Files (*.trace_x)") << tr("Text Files (*.txt)"));

    if(_file_dialog->exec() == QDialog::Accepted)
    {
        _trace_service.trace_controller().save_trace(_file_dialog->selectedFiles().first());
    }
}

void MainWindow::load_trace()
{
    X_CALL;

    _file_dialog->setAcceptMode(QFileDialog::AcceptOpen);
    _file_dialog->setWindowTitle(tr("Load Trace"));

    if(_file_dialog->exec() == QDialog::Accepted)
    {
        _trace_service.trace_controller().load_trace(_file_dialog->selectedFiles().first());
    }
}

void MainWindow::show_gui()
{
    X_CALL;

    if(x_settings().config().contains("main_window_geometry"))
    {
        show();
    }
    else
    {
        showMaximized();
    }

    ui->main_trace_view->show();
}

void MainWindow::save_widget_geometry(QWidget *widget)
{
    x_settings().set_value(widget->objectName() + "_geometry", widget->saveGeometry());
}

void MainWindow::restore_widget_geometry(QWidget *widget)
{
    widget->restoreGeometry(x_settings().value(widget->objectName() + "_geometry").toByteArray());
}

void MainWindow::save_session(Session &session)
{
    X_CALL;

    session.capture_filter = _trace_service.trace_controller().tx_model_service().save_state();
    session.trace_filter = _trace_service.trace_controller().trace_model_service().save_state();
    session.main_view_filter = ui->main_trace_view->save_filter_state();
}

template<class T>
void MainWindow::save_widget_state(T *widget)
{
    x_settings().set_value(widget->objectName() + "_geometry", widget->saveGeometry());
    x_settings().set_value(widget->objectName() + "_state", widget->saveState());
}

template<class T>
void MainWindow::restore_widget_state(T *widget)
{
    widget->restoreGeometry(x_settings().value(widget->objectName() + "_geometry").toByteArray());
    widget->restoreState(x_settings().value(widget->objectName() + "_state").toByteArray());
}
