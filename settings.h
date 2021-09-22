#ifndef SETTINGS_H
#define SETTINGS_H

#include <QApplication>

#include <QColor>
#include <QDataStream>
#include <QCommandLineParser>
#include <QMap>
#include <QDir>
#include <QVariant>
#include <QSettings>

#include "settings_option.h"
#include "trace_model.h"

class XSettings : public QObject
{
    Q_OBJECT

public:
    XSettings();
    ~XSettings();

    QVariant value(const QString &key, const QVariant &default_value = QVariant());

    void set_value(const QString &key, const QVariant &value);

    QSettings & config();

    void set_config(QSettings *config);

signals:
    void settings_changed();

private:
    SettingsOption * add_command_line_option(const QString &name, const QString &description, const QString &value_name = QString());

public:
    QCommandLineParser cl_parser;

    SettingsOption *help_option;
    SettingsOption *version_option;

    SettingsOption *no_gui_option;
    SettingsOption *no_show_option;
    SettingsOption *no_config_option;
    SettingsOption *debug_option; //! start server with self debugging
    SettingsOption *auto_exit_option;
    SettingsOption *auto_save_option;
    SettingsOption *auto_clean_option;
    SettingsOption *self_trace_option;
    SettingsOption *server_name_option;
    SettingsOption *config_name_option;
    SettingsOption *log_dir_option;
    SettingsOption *no_swap_option;
    SettingsOption *message_limit_option;
    SettingsOption *memory_limit_option;
    SettingsOption *swap_limit_option;
    SettingsOption *swap_path_option;
    SettingsOption *file_data_limit_option;

    SettingsOption *check_option;

    //

    QColor main_color;
    QColor search_highlight_color;
    QColor line_highlight_color;
    QColor checked_color;

    QList<EntityClass> entity_model_layout;
    QList<EntityClass> trace_completer_layout;
    QList<EntityClass> capture_layout;

    QSettings *_config;

private:
    friend class SettingsOption;

    QList<SettingsOption *> _options;
};

class Settings
{
public:
    static QString version() { return "1.0"; }

    static XSettings _xsettings;
};

inline XSettings & x_settings()
{
    return Settings::_xsettings;
}

//! DataStream with read_next_array method

class DataStream : public QDataStream
{
public:
    DataStream();
    explicit DataStream(QIODevice *);
    DataStream(QByteArray *, QIODevice::OpenMode);
    DataStream(const QByteArray &);

public:
    QByteArray read_next_array();
};

#endif // SETTINGS_H
