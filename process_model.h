#ifndef PROCESS_MODEL_H
#define PROCESS_MODEL_H

#include <QString>
#include <QList>
#include <QHash>
#include <QColor>
#include <QStandardItem>

#include <stdint.h>

#include <boost/atomic.hpp>

#include "color_generator.h"
#include "trace_model.h"
#include "trace_tools.h"

#include "trace_x/impl/types.h"

struct raw_message_t
{
    uint8_t  subtype;
    uint64_t timestamp;
    uint64_t extra_timestamp;
    uint64_t tid;
    uint64_t context;
    uint64_t module;
    uint64_t function;
    uint64_t source;
    uint32_t line;
    uint64_t label;
    char*    data;
};

class TraceController;

struct ThreadState
{
    ThreadState(): level(0), last_level_index(0) {}

    int level;
    uint64_t last_level_index;
};

class ProcessModel : public EntityItem
{
public:
    enum Roles
    {
        PidRole = Qt::UserRole + 1100,
        UserNameRole,
        IndexRole
    };

    ProcessModel();
    ProcessModel(ColorPair color, pid_index_t index, TraceController *trace_controller, uint64_t pid, uint64_t timestamp, const QString &process_name, const QString &user_name, trace_x::filter_index index_container);

    ~ProcessModel();

    void disconnected(uint64_t time);

    void append_message(raw_message_t *message);

    pid_index_t index() const { return _index; }
    pid_index_t name_index() const { return _name_index; }
    pid_index_t user_index() const { return _user_index; }

    const QString &user() const { return _user_name; }
    const QString &name() const { return _full_path; }
    uint64_t pid() const { return _pid; }

    QVariant item_data(int role, int flags) const;

    virtual bool operator<(const QStandardItem &other) const;

    virtual void read(QDataStream &in);
    virtual void write(QDataStream &out) const;

public:
    //! Регистрирует новый индекс, устанавливая в нём флаг для текущего фильтра передатчика
    void register_index(trace_x::filter_index_t::iterator &it);

    //! Обновляет все флаги индекса передатчика в соответствии с текущим фильтром передатчика
    void update_filter();

    inline quint64 time_delta() const { return _time_delta; }

public:
    TraceController *_trace_controller;

private:
    friend class TraceController;
    friend class LocalConnectionController;
    friend class TraceServer;

    //! Время подключения (UTC) (ns)
    quint64 _connect_time;

    //! Время между подключением этого процесса и подключением первого процесса (ns)
    quint64 _time_delta;

    quint64 _pid;

    QString _full_path;
    QString _process_name;
    QString _user_name;

    pid_index_t _index;
    pid_index_t _name_index;
    pid_index_t _user_index;

    QHash<uint64_t, ThreadState> _thread_level;

    trace_x::filter_index _index_container;

    bool _crash_received;

    QHash<uint64_t, source_index_t>   _source_index_hash;
    QHash<uint64_t, module_index_t>   _module_index_hash;
    QHash<uint64_t, function_index_t> _function_index_hash;
    QHash<uint64_t, tid_index_t>      _thread_index_hash;
    QHash<uint64_t, context_index_t>  _context_index_hash;
    QHash<uint64_t, label_index_t>    _label_index_hash;
};

#endif // PROCESS_MODEL_H
