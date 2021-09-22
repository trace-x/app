#include "process_model.h"

#include <boost/chrono.hpp>
#include <boost/chrono/duration.hpp>

#include <QFileInfo>

#include "trace_x/trace_x.h"
#include "trace_x/impl/platform.h"

#include "trace_controller.h"
#include "data_parser.h"

ProcessModel::ProcessModel():
    EntityItem(),
    _trace_controller(0),
    _index_container(),
    _crash_received(false)
{
}

ProcessModel::ProcessModel(ColorPair color, pid_index_t index, TraceController *trace_controller, uint64_t pid, uint64_t timestamp, const QString &process_name, const QString &user_name, trace_x::filter_index index_container):
    EntityItem(ProcessIdEntity, 0, "", index, color.bg_color, color.fg_color),
    _trace_controller(trace_controller),
    _index(index),
    _connect_time(timestamp),
    _pid(pid),
    _full_path(process_name),
    _process_name(QFileInfo(process_name).baseName()),
    _user_name(user_name),
    _index_container(index_container)
{
    X_CALL;

    _descriptor.id = quint64(_pid);

    setToolTip(QObject::tr("Process ID: %1").arg(pid));
    setData(QString("%1] %2 %3").arg(_index + 1).arg(quint64(pid)).arg(QFileInfo(_full_path).baseName()), PidRole);
    setData(quint64(index), IndexRole);

    _time_delta = timestamp - _trace_controller->zero_time();

    _context_index_hash.insert(0, 0);
}

ProcessModel::~ProcessModel()
{
    X_CALL;
}

void ProcessModel::disconnected(uint64_t time)
{
    X_CALL;

    trace_message_t *trace_message = new trace_message_t;

    _trace_controller->register_message_type(trace_x::MESSAGE_DISCONNECTED);

    trace_message->type = trace_x::MESSAGE_DISCONNECTED;
    trace_message->timestamp = time - _connect_time;
    trace_message->process_index = _index;
    trace_message->call_level = 0;
    trace_message->module_index = 0;
    trace_message->tid_index = 0;
    trace_message->label_index = 0;
    trace_message->function_index = 0;
    trace_message->context_index = 0;
    trace_message->source_index = 0;
    trace_message->flags = 0;
    trace_message->message_text = QObject::tr("Process disconnected: \"%1\"[%2]").arg(_full_path).arg(_pid);

    _trace_controller->append(trace_message);
}

void ProcessModel::append_message(raw_message_t *message)
{
    X_CALL;

    trace_message_t *trace_message;

    size_t offset = 0; //mutable offset for data parsing

    const char *module; size_t module_size;
    const char *source; size_t source_size;
    const char *function; size_t function_size;
    const char *label; size_t label_size;

    try
    {
        trace_message = new trace_message_t;

        trace_message->process_index = _index;
        trace_message->type = message->subtype;
        trace_message->timestamp = message->timestamp;
        trace_message->extra_timestamp = message->extra_timestamp;
        trace_message->source_line = message->line;
        trace_message->flags = 0;
        trace_message->label_index = 0;
    }
    catch(std::exception &e)
    {
        X_EXCEPTION(e.what());
    }

    //parse literals positions

    parse_string(message->data, &module, module_size, offset);
    parse_string(message->data, &source, source_size, offset);
    parse_string(message->data, &function, function_size, offset);
    parse_string(message->data, &label, label_size, offset);

    _trace_controller->register_message_type(message->subtype);

    _index_container.mutex->lock();

    auto result = _index_container.index->insert(trace_x::message_filter_t(message->subtype, message->module, message->tid,
                                                                           message->context, message->function, message->source));

    _index_container.mutex->unlock();

    if(result.second)
    {
        //here we get some new index, but don`t know which

        //module_index:
        {
            QHash<uint64_t, uint16_t>::const_iterator it = _module_index_hash.find(message->module);

            if(it == _module_index_hash.end())
            {
                trace_message->module_index = _trace_controller->register_module(QString::fromLocal8Bit(module, int(module_size)), *this);

                _module_index_hash.insert(message->module, trace_message->module_index);

                X_INFO("new module id: #{} ; [{}][#{}]", message->module, _trace_controller->module_at(trace_message)->text(), trace_message->module_index);
            }
            else
            {
                trace_message->module_index = it.value();
            }
        }

        //source file path index:
        {
            QHash<uint64_t, uint16_t>::const_iterator it = _source_index_hash.find(message->source);

            if(it == _source_index_hash.end())
            {
                trace_message->source_index = _trace_controller->register_source_file(QString::fromLocal8Bit(source, int(source_size)));

                _source_index_hash.insert(message->source, trace_message->source_index);

                X_INFO("new source id: #{} ; [{}][#{}]", message->source, _trace_controller->source_at(trace_message)->descriptor().id.toString(), trace_message->source_index);
            }
            else
            {
                trace_message->source_index = it.value();
            }
        }

        //function_index:
        {
            QHash<uint64_t, function_index_t>::const_iterator it = _function_index_hash.find(message->function);

            if(it == _function_index_hash.end())
            {
                trace_message->function_index = _trace_controller->register_function(parse_function(function, function_size), trace_message->source_index, message->context != 0);

                _function_index_hash.insert(message->function, trace_message->function_index);

                X_INFO("new function id: #{} ; [{}][#{}]", message->function, _trace_controller->function_at(trace_message)->text(), trace_message->function_index);
            }
            else
            {
                trace_message->function_index = it.value();
            }
        }

        //thread_index:
        {
            QHash<uint64_t, tid_index_t>::const_iterator it = _thread_index_hash.find(message->tid);

            if(it == _thread_index_hash.end())
            {
                trace_message->tid_index = _trace_controller->register_thread(*this, message->tid);

                _thread_index_hash.insert(message->tid, trace_message->tid_index);

                X_INFO("new thread id: {} [#{}]", message->tid, trace_message->tid_index);
            }
            else
            {
                trace_message->tid_index = it.value();
            }
        }

        //context_index:
        {
            QHash<uint64_t, context_index_t>::const_iterator it = _context_index_hash.find(message->context);

            if(it == _context_index_hash.end())
            {
                trace_message->context_index = _trace_controller->register_context(*this, message->context, trace_message->function_index);

                _context_index_hash.insert(message->context, trace_message->context_index);

                X_INFO("new context id: {} [#{}]", message->context, trace_message->context_index);
            }
            else
            {
                trace_message->context_index = it.value();
            }
        }

        result.first->module_index   = trace_message->module_index;
        result.first->thread_index   = trace_message->tid_index;
        result.first->context_index  = trace_message->context_index;
        result.first->function_index = trace_message->function_index;
        result.first->source_index   = trace_message->source_index;

        register_index(result.first);
    }
    else
    {
        //already indexed

        trace_message->module_index   = result.first->module_index;
        trace_message->tid_index      = result.first->thread_index;
        trace_message->context_index  = result.first->context_index;
        trace_message->function_index = result.first->function_index;
        trace_message->source_index   = result.first->source_index;
    }

    //

    if((trace_message->type > trace_x::MESSAGE_RETURN) && (trace_message->type < trace_x::MESSAGE_DATA))
    {
        //packet with message string

        const char *message_str; size_t message_size;
        parse_string(message->data, &message_str, message_size, offset);

        trace_message->message_text = QString::fromLocal8Bit(message_str, int(message_size));
    }

    if(label_size > 0)
    {
        uint64_t label_id = message->label;

        QHash<uint64_t, label_index_t>::const_iterator it = _label_index_hash.find(label_id);

        if(it == _label_index_hash.end())
        {
            trace_message->label_index = _trace_controller->register_label(QString::fromLocal8Bit(label, int(label_size)));

            _label_index_hash.insert(label_id, trace_message->label_index);

            X_INFO("new label: #{} ; [{}][#{}]", label_id, _trace_controller->label_item_at(trace_message)->text(), trace_message->label_index);
        }
        else
        {
            trace_message->label_index = it.value();
        }

        QString current_text = trace_message->message_text;

        trace_message->message_text = _trace_controller->label_item_at(trace_message)->text();

        if(!current_text.isEmpty())
        {
            trace_message->message_text += " : " + current_text;
        }
    }

    if(trace_message->type == trace_x::MESSAGE_CRASH)
    {
        _crash_received = true;
    }

    //calculate current call level

    if(result.first->is_accepted)
    {
        ThreadState &state = _thread_level[message->tid];

        int &current_level = state.level;

        if(trace_message->type == trace_x::MESSAGE_RETURN)
        {
            if(current_level > 0)
            {
                current_level--;
            }
        }

        trace_message->call_level = current_level;
        //  trace_message->prev_index = state.last_level_index;

        if(trace_message->type == trace_x::MESSAGE_CALL)
        {
            current_level++;
        }
    }

    _trace_controller->append(trace_message, !result.first->is_accepted, message->data + offset);
}

QVariant ProcessModel::item_data(int role, int flags) const
{
    if(role == Qt::DisplayRole)
    {
        QString text;

        if(flags & OnlyIndex)
        {
            text = QString("%1").arg(_index + 1);
        }
        else
        {
            text = QString("%1] %2").arg(_index + 1).arg(quint64(_pid));

            if(flags & FullText)
            {
                if(_trace_controller->_process_names.size() > 1)
                {
                    text += " " % _process_name;
                }

                if(_trace_controller->_process_users.size() > 1)
                {
                    text += " " % _user_name;
                }
            }
        }

        return text;
    }

    return EntityItem::item_data(role, flags);
}

bool ProcessModel::operator<(const QStandardItem &other) const
{
    return _index < other.data(IndexRole).toULongLong();
}

void ProcessModel::read(QDataStream &in)
{
    in >> _time_delta;
    in >> _full_path;
    in >> _pid;
    in >> _process_name;
    in >> _user_name;
    in >> _index;
    in >> _name_index;
    in >> _user_index;

    EntityItem::read(in);
}

void ProcessModel::write(QDataStream &out) const
{
    out << _time_delta;
    out << _full_path;
    out << _pid;
    out << _process_name;
    out << _user_name;
    out << _index;
    out << _name_index;
    out << _user_index;

    EntityItem::write(out);
}

void ProcessModel::register_index(trace_x::filter_index_t::iterator &it)
{
    X_CALL;

    //TODO const? mutex?

    FilterModel *capture_filter = _trace_controller->capture_filter();

    if(capture_filter->is_enabled())
    {
        QHash<int, ItemDescriptor> descriptor;

        descriptor[ProcessIdEntity] = this->descriptor();

        descriptor[ProcessNameEntity] = _trace_controller->process_name_at(this->name_index())->descriptor();
        descriptor[ProcessUserEntity] = _trace_controller->process_user_at(this->user_index())->descriptor();
        descriptor[ModuleNameEntity] = _trace_controller->module_at(it->module_index)->descriptor();
        descriptor[FunctionNameEntity] = _trace_controller->function_at(it->function_index)->descriptor();
        descriptor[ClassNameEntity] = _trace_controller->class_at(it->function_index)->descriptor();
        descriptor[SourceNameEntity] = _trace_controller->source_at(it->source_index)->descriptor();
        descriptor[ThreadIdEntity] = _trace_controller->thread_item_at(it->thread_index)->descriptor();
        descriptor[ContextIdEntity] = _trace_controller->context_item_at(it->context_index)->descriptor();
        descriptor[MessageTypeEntity] = _trace_controller->message_type_at(it->type)->descriptor();

        it->is_accepted = capture_filter->check_filter(descriptor);
    }
    else
    {
        it->is_accepted = true;
    }
}

void ProcessModel::update_filter()
{
    X_CALL;

    if(_index_container.index)
    {
        _index_container.mutex->lock();

        for(trace_x::filter_index_t::iterator it = _index_container.index->begin(); it != _index_container.index->end(); ++it)
        {
            register_index(it);
        }

        _index_container.mutex->unlock();
    }
}
