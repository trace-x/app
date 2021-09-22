#include "trace_data_model.h"

#include <QTimer>

#include "trace_x/trace_x.h"

template<class T>
T abs_diff(T x, T y)
{
    return x > y ? x - y : y - x;
}

TraceDataModel::TraceDataModel(QObject *parent):
    QObject(parent),
    _has_new_messages(false),
    _has_new_indexes(false),
    _emit_refiltered(false),
    _trace_mutex(QMutex::NonRecursive),
    _index_mutex(QMutex::NonRecursive),
    _safe_size(0)
{
    X_CALL;

    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &TraceDataModel::check_updates);

    timer->start(100);
}

bool TraceDataModel::find_relative_index(index_t trace_index, index_t &relative_index) const
{
    X_CALL;

    QMutexLocker lock(&_trace_mutex);

    int i = 0;

    for(; i < _trace_list.size(); ++i)
    {
        if(_trace_list[i]->index == trace_index)
        {
            relative_index = i;

            return true;
        }
        else if(_trace_list[i]->index > trace_index)
        {
            if(i)
            {
                relative_index = abs_diff(_trace_list[i]->index, trace_index) < abs_diff(_trace_list[i - 1]->index, trace_index) ? i : i - 1;
            }
            else
            {
                relative_index = 0;
            }

            return false;
        }
    }

    relative_index = _trace_list.size() - 1;

    return false;
}

void TraceDataModel::append(const trace_message_t *message)
{
    //X_CALL;

    // this function may be called from different threads

    _trace_mutex.lock();

    _trace_list.append(message);

    _has_new_messages = true;

    _trace_mutex.unlock();
}

void TraceDataModel::insert(const trace_message_t *message, int index)
{
    //X_CALL;

    // this function may be called from different threads

    _trace_mutex.lock();

    _trace_list.insert(index, message);

    _has_new_messages = true;

    _trace_mutex.unlock();
}

void TraceDataModel::update_index(const trace_message_t *message)
{
    X_CALL;

    _index_mutex.lock();

    auto result = _model_index.insert(message_index_t(message->type, message->process_index, message->module_index,
                                                      message->tid_index, message->context_index,
                                                      message->function_index, message->source_index, message->label_index));

    _index_mutex.unlock();

    if(result.second)
    {
        _has_new_indexes = true;
    }
}

void TraceDataModel::emit_refiltered()
{
    X_CALL;

    _emit_refiltered = true;
}

void TraceDataModel::emit_updated()
{
    _trace_mutex.lock();

    _safe_size = _trace_list.size();

    _trace_mutex.unlock();

    emit updated();
}

void TraceDataModel::set_message_list(const QList<const trace_message_t *> &list)
{
    X_CALL;

    _trace_list = list;

    _safe_size = _trace_list.size();

    emit updated();
    emit cleaned();
}

void TraceDataModel::clear()
{
    X_CALL;

    _trace_mutex.lock();
    _index_mutex.lock();

    _safe_size = 0;
    _trace_list = QList<const trace_message_t*>();
    _model_index = trace_index_t();

    _trace_mutex.unlock();
    _index_mutex.unlock();

    emit updated();
    emit cleaned();
    emit model_changed();
}

const QList<const trace_message_t *> &TraceDataModel::trace_list() const
{
    return _trace_list;
}

bool TraceDataModel::get_nearest_by_type(index_t current, uint8_t type, index_t &index) const
{
    X_CALL;

    index_t prev_index, next_index;

    bool has_next = get_next_by_type(current, type, next_index);
    bool has_prev = get_prev_by_type(current, type, prev_index);

    if(has_next && has_prev)
    {
        index = qAbs(next_index - index) < qAbs(prev_index - index) ? next_index : prev_index;
    }
    else if(has_next)
    {
        index = next_index;
    }
    else if(has_prev)
    {
        index = prev_index;
    }
    else
    {
        return false;
    }

    return true;
}

bool TraceDataModel::get_next_by_type(index_t current, uint8_t type, index_t &index) const
{
    X_CALL;

    QMutexLocker lock(&_trace_mutex);

    current = relative_index(current);

    for(int i = current; i < _trace_list.size(); ++i)
    {
        if(_trace_list[i]->type == type)
        {
            index = trace_index(i);

            return true;
        }
    }

    return false;
}

bool TraceDataModel::get_prev_by_type(index_t current, uint8_t type, index_t &index) const
{
    X_CALL;

    QMutexLocker lock(&_trace_mutex);

    current = relative_index(current);

    for(intptr_t i = current - 1; i >= 0; --i)
    {
        if(_trace_list[i]->type == type)
        {
            index = trace_index(i);

            return true;
        }
    }

    return false;
}

void TraceDataModel::check_updates()
{
    //X_CALL;

    if(_has_new_messages)
    {
        _has_new_messages = false;

        _trace_mutex.lock();

        _safe_size = _trace_list.size();

        _trace_mutex.unlock();

        emit updated();
    }

    if(_emit_refiltered)
    {
        _emit_refiltered = false;

        emit refiltered();
    }

    if(_has_new_indexes)
    {
        _has_new_indexes = false;

        emit model_changed();
    }
}
