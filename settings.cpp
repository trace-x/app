#include "settings.h"

#include "trace_x/trace_x.h"

XSettings Settings::_xsettings;

XSettings::XSettings():
    QObject(),
    _config(0)
{
    main_color = QColor("#2B579A");
    search_highlight_color = QColor("#FFAE00");
    line_highlight_color = QColor("#C2D5F2");
    checked_color = QColor("#C2D5F2");

    //

    entity_model_layout << ProcessIdEntity      <<
                           ProcessNameEntity    <<
                           ProcessUserEntity    <<
                           ModuleNameEntity     <<
                           ThreadIdEntity       <<
                           MessageTypeEntity    <<
                           LabelNameEntity      <<
                           SourceNameEntity     <<
                           ClassNameEntity      <<
                           FunctionNameEntity   <<
                           ContextIdEntity;

    trace_completer_layout << ModuleNameEntity     <<
                              ProcessNameEntity    <<
                              ProcessIdEntity      <<
                              ProcessUserEntity    <<
                              ClassNameEntity      <<
                              SourceNameEntity     <<
                              FunctionNameEntity   <<
                              LabelNameEntity      <<
                              MessageTypeEntity    <<
                              ThreadIdEntity       <<
                              ContextIdEntity      <<
                              MessageTextEntity;

    capture_layout << ProcessIdEntity      <<
                      ProcessNameEntity    <<
                      ProcessUserEntity    <<
                      ModuleNameEntity     <<
                      ThreadIdEntity       <<
                      MessageTypeEntity    <<
                      SourceNameEntity     <<
                      ClassNameEntity      <<
                      FunctionNameEntity   <<
                      ContextIdEntity;

    //

    help_option = new SettingsOption(this, cl_parser.addHelpOption());
    version_option = new SettingsOption(this, cl_parser.addVersionOption());

    no_gui_option          = add_command_line_option("no_gui", "Start without gui.");
    no_show_option         = add_command_line_option("no_show", "Start without show(hidden).");
    no_config_option       = add_command_line_option("no_config", "Start without loading configuration file.");
    debug_option           = add_command_line_option("debug", "Start in self-debugging mode.");
    auto_exit_option       = add_command_line_option("auto_exit", "Auto exit on first disconnect.");
    auto_clean_option      = add_command_line_option("auto_clean", "Clean trace on new connection.");
    auto_save_option       = add_command_line_option("auto_save", "Auto save mode.", "[ALL] [CRASH]");
    self_trace_option      = add_command_line_option("x", "Self debugging trace server name.", "name");
    server_name_option     = add_command_line_option("n", "Server name.", "name");
    config_name_option     = add_command_line_option("config", "Config file name.", "name");
    log_dir_option         = add_command_line_option("dir", "Log directory.", "path");
    message_limit_option   = add_command_line_option("msg_limit", "Message count limit.", "number");
    memory_limit_option    = add_command_line_option("mem_limit", "Data memory limit, MB.", "size");
    swap_limit_option      = add_command_line_option("swap_limit", "Data swap file limit, MB.", "size");
    swap_path_option       = add_command_line_option("swap_path", "Data swap path.", "path");
    file_data_limit_option = add_command_line_option("file_limit", "Data size limit in .trace_x file, MB.", "size");
    no_swap_option         = add_command_line_option("no_swap", "Don`t use data swapping");

    check_option = add_command_line_option("check", "Returns 42. Because we can. ¯\\_(ツ)_/¯");
}

XSettings::~XSettings()
{
}

QVariant XSettings::value(const QString &key, const QVariant &default_value)
{
    return _config->value(key, default_value);
}

void XSettings::set_value(const QString &key, const QVariant &value)
{
    X_CALL;

    X_INFO("set config value {} : {}", key, value);

    _config->setValue(key, value);
}

QSettings & XSettings::config()
{
    return *_config;
}

void XSettings::set_config(QSettings *config)
{
    _config = config;

    foreach (SettingsOption *option, _options)
    {
        if(option->is_command_line_set() && !cl_parser.value(option->_name).isEmpty())
        {
            option->_override_value = cl_parser.value(option->_name);
        }
    }
}

SettingsOption *XSettings::add_command_line_option(const QString &name, const QString &description, const QString &value_name)
{
    SettingsOption *option = new SettingsOption(this, name, description, value_name);

    _options.append(option);

    cl_parser.addOption(option->_cl_option);

    return option;
}

DataStream::DataStream(): QDataStream() {}
DataStream::DataStream(QIODevice *device): QDataStream(device) {}
DataStream::DataStream(QByteArray *array, QIODevice::OpenMode flags): QDataStream(array, flags) {}
DataStream::DataStream(const QByteArray &array): QDataStream(array) {}

QByteArray DataStream::read_next_array()
{
    QByteArray array;

    *this >> array;

    return array;
}
