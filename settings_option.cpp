#include "settings_option.h"

#include "settings.h"

SettingsOption::SettingsOption(XSettings *settings, QCommandLineOption cl_option):
    QObject(settings),
    _settings(settings),
    _cl_option(cl_option)
{
}

SettingsOption::SettingsOption(XSettings *settings, const QString &name, const QString &description, const QString &value_name):
    QObject(settings),
    _settings(settings),
    _name(name),
    _cl_option(name, description, value_name)
{
}

bool SettingsOption::is_command_line_set() const
{
    return _settings->cl_parser.isSet(_cl_option);
}

QString SettingsOption::command_line_value() const
{
    return _settings->cl_parser.value(_cl_option);
}

void SettingsOption::set_value(const QVariant &value)
{
    if(_override_value.isNull())
    {
        _settings->config().setValue(_name, value);
    }
    else
    {
        _override_value = value;
    }

    emit value_changed();

    emit _settings->settings_changed();
}

void SettingsOption::override_value(const QVariant &value)
{
    _override_value = value;
}

void SettingsOption::init_value(const QVariant &value)
{
    if(this->value().isNull())
    {
        _settings->config().setValue(_name, value);
    }
}

QVariant SettingsOption::value(const QVariant &default_value) const
{
    if(!_override_value.isNull())
    {
        return _override_value;
    }

    return _settings->config().value(_name, default_value);
}

QString SettingsOption::string_value(const QString &default_value) const
{
    return value(default_value).toString();
}

bool SettingsOption::bool_value(bool default_value) const
{
    return value(default_value).toBool();
}

quint64 SettingsOption::uint_value(quint64 default_value) const
{
    return value(default_value).toULongLong();
}

void SettingsOption::set_bool_value(bool value)
{
    set_value(value);
}
