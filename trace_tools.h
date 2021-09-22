#ifndef TRACE_TOOLS_H
#define TRACE_TOOLS_H

#include <QString>
#include <QStringList>
#include <QHash>

template <class T, class U>
class QHashBuilder
{
public:
    QHashBuilder(const T &key, const U &value)
    {
        _hash.insert(key, value);
    }

    QHashBuilder & operator() (const T &key, const U &value)
    {
        _hash.insert(key, value);

        return *this;
    }

    operator QHash<T,U>()
    {
        return _hash;
    }

private:
    QHash<T,U> _hash;
};

struct function_t
{
    QString signature;
    QString function_name;
    QString full_name;
    QStringList namespace_list;
    QString namespace_string;
    QString arg;
    QString return_type;
    QString spec;
};

function_t parse_function(const char *string, size_t len);

#endif
