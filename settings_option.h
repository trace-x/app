#ifndef SETTING_SOPTION_H
#define SETTING_SOPTION_H

#include <QObject>
#include <QVariant>
#include <QCommandLineOption>

class XSettings;

class SettingsOption : public QObject
{
    Q_OBJECT

public:
    SettingsOption(XSettings *settings, QCommandLineOption cl_option);
    SettingsOption(XSettings *settings, const QString &name, const QString &description, const QString &value_name);

    bool is_command_line_set() const;

    QString command_line_value() const;
    QVariant value(const QVariant &default_value = QVariant()) const;
    QString string_value(const QString &default_value = QString()) const;
    bool bool_value(bool default_value = false) const;
    quint64 uint_value(quint64 default_value = 0) const;

public slots:
    void set_bool_value(bool value);
    void set_value(const QVariant &value);
    void override_value(const QVariant &value);
    void init_value(const QVariant &value);

signals:
    void value_changed();

private:
    friend class XSettings;

    XSettings *_settings;
    QString _name;
    QCommandLineOption _cl_option;
    QVariant _override_value;
};

#endif // SETTING_SOPTION_H
