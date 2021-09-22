#include "main_window.h"

#include <csignal>

#include <QApplication>
#include <QStyleFactory>

#include "trace_x/trace_x.h"
#include "trace_x/tools/qt_trace_handler.h"

#include "settings.h"
#include "trace_service.h"

class TraceXRegister
{
public:
    TraceXRegister()
    {
        trace_x::x_disable_auto_connect = true;
    }
};

static TraceXRegister register_trace_x;

#ifdef QT_IS_STATIC

#include <QtPlugin>

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

#endif

namespace
{

void apply_variable(QString &style_sheet, const QString &key, const QString &value)
{
    style_sheet.replace(QRegExp(QString("\\$%1\\$").arg(key)), value);
}

void exit_qt(int)
{
    QCoreApplication::exit();
}

void init_config()
{
    QSettings *config;

    if(x_settings().config_name_option->command_line_value().isEmpty())
    {
        config = new QSettings(QSettings::IniFormat, QSettings::UserScope, "trace_x");
    }
    else
    {
        config = new QSettings(x_settings().config_name_option->command_line_value(), QSettings::IniFormat);
    }

    x_settings().set_config(config);

    //

    if(x_settings().debug_option->is_command_line_set())
    {
        trace_x::start_trace_receiver(trace_x::MODE_SAVE_CRASH);
    }
    else
    {
        if(!x_settings().self_trace_option->string_value().isEmpty())
        {
            trace_x::trace_connect(x_settings().self_trace_option->string_value().toLocal8Bit().data());
        }
        else
        {
            trace_x::trace_connect(0);
        }
    }

    X_VALUE_F("config_file", config->fileName());

    // init default values

    if(!qgetenv(TraceServerNameEnvID).isEmpty())
    {
        x_settings().server_name_option->override_value(qgetenv(TraceServerNameEnvID));
    }
    else
    {
        x_settings().server_name_option->init_value(TraceIDDefault);
    }

    x_settings().message_limit_option->init_value(1000000);
    x_settings().memory_limit_option->init_value(1024);
    x_settings().swap_limit_option->init_value(2048);
    x_settings().file_data_limit_option->init_value(10);
    x_settings().no_swap_option->init_value(false);
    x_settings().swap_path_option->init_value(QDir::tempPath());
}

}

class Free
{
public:
    ~Free()
    {
        delete x_settings()._config;
    }
};

struct chain_t
{
    std::vector<int> v;

    bool operator < (const chain_t &other) const
    {
        return v < other.v;
    }
};

int main(int argc, char *argv[])
{
    chain_t c1;
    chain_t c2;

    QMap<chain_t, int> maponika;

    maponika[c1] = 0;
    maponika[c2] = 0;

//    QHash<QVector<int>, int> mapok;

//    QVector<int> key;

//    mapok[key] = 0;

//    chain_t c1;

//    c1.v = { 0, 1, 2 };

//    chain_t c2;

   // bool cc = (c1.v < c2.v);

    //

    Free free_guard;

    signal(SIGINT, &::exit_qt);
    signal(SIGTERM, &::exit_qt);

    QStringList arguments;

    for(int i = 0; i < argc; ++i)
    {
        arguments.append(argv[i]);
    }

    x_settings().cl_parser.parse(arguments);

    if(x_settings().check_option->is_command_line_set())
    {
        return 42;
    }

    if(x_settings().help_option->is_command_line_set())
    {
        QCoreApplication app(argc, argv);

        Q_UNUSED(app);

        x_settings().cl_parser.showHelp();
    }

    if(x_settings().version_option->is_command_line_set())
    {
        QTextStream(stdout) << "Trace-X version: " << Settings::version() << endl;

        return 0;
    }

    if(!x_settings().no_gui_option->is_command_line_set())
    {
        QApplication app(argc, argv);
        app.setApplicationVersion(Settings::version());
        app.setApplicationName("Trace-X");
        app.setOrganizationName("Trace-X");

        ::init_config();

        X_CALL_F;
        X_INFO_F("comman line arguments: {}", arguments);

        QFile qss_file("://app.qss");

        qss_file.open(QIODevice::ReadOnly | QIODevice::Text);

        QString style_sheet = qss_file.readAll();

        ::apply_variable(style_sheet, "main_color",          x_settings().main_color.name());
        ::apply_variable(style_sheet, "selection_color",     "#D5E1F2");
        ::apply_variable(style_sheet, "highlight_color",     "#5392EF");
        ::apply_variable(style_sheet, "pressed_hover_color", "#A3BDE3");
        ::apply_variable(style_sheet, "checked_color",       x_settings().checked_color.name());
        ::apply_variable(style_sheet, "search_color",        x_settings().search_highlight_color.name());

#ifdef Q_OS_WIN
        ::apply_variable(style_sheet, "view_font",           "Consolas");
#elif defined Q_OS_LINUX
        ::apply_variable(style_sheet, "view_font",           "Monospace");
#endif

        app.setStyle(QStyleFactory::create("Fusion"));

        MainWindow main_window;

        main_window.setStyleSheet(style_sheet);

        return app.exec();
    }
    else
    {
        QCoreApplication app(argc, argv);
        app.setApplicationVersion(Settings::version());
        app.setApplicationName("Trace-X");
        app.setOrganizationName("Trace-X");

        init_config();

        X_CALL_F;
        X_INFO_F("comman line arguments: {}", arguments);

        TraceService service;

        service.set_server_state(true);

        if(service.trace_server().is_started())
        {
            QTextStream(stdout) << "\nTrace server [" << service.trace_server().server_name() << "] started at : " << QTime::currentTime().toString() << endl;

            int result = app.exec();

//            QTextStream(stdout) << "\n";

//            if(service.trace_controller().trace_model().safe_size())
//            {
//                QString trace_file_name = QString("%1.trace_x").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));

//                QTextStream(stdout) << "Saving trace: " << service.trace_controller().trace_model().safe_size() << " messages" << endl;

//                service.trace_controller().save_trace(trace_file_name);

//                QTextStream(stdout) << "Trace stored on:" << QFileInfo(trace_file_name).absoluteFilePath() << endl;
//            }

            return result;
        }
        else
        {
            QTextStream(stderr) << "service is not started" << endl;
        }
    }

    return 0;
}
