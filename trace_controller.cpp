#include "trace_controller.h"

#include <QTimer>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

#include <boost/filesystem.hpp>

#include "image_lib/image.h"
#include "data_parser.h"
#include "settings.h"
#include "trace_x/trace_x.h"

namespace
{

QByteArray make_data_array(const char *data_buffer, QString &message)
{
    X_CALL_F;

    QByteArray array;

    QDataStream stream(&array, QIODevice::WriteOnly);

    size_t offset = 0;

    std::vector<quint64> size    = get_vector<quint64>(data_buffer, offset);
    quint64 elem_size            = get_value<uint64_t>(data_buffer, offset);
    qint32 channels              = get_value<uint32_t>(data_buffer, offset);
    qint8 unit_type              = get_value<int8_t>(data_buffer, offset);
    quint32 data_format          = get_value<uint32_t>(data_buffer, offset);
    std::vector<quint8> band_map = get_vector<quint8>(data_buffer, offset);
    std::vector<quint8> axes     = get_vector<quint8>(data_buffer, offset);
    quint32 message_size         = get_value<uint32_t>(data_buffer, offset);
    quint32 palette_size         = get_value<uint32_t>(data_buffer, offset);
    quint64 byte_size            = get_value<uint64_t>(data_buffer, offset);

    QString message_string = QString::fromLocal8Bit(data_buffer + offset, int(message_size));
    offset += message_size;

    stream << QVector<quint64>::fromStdVector(size) << elem_size << channels << unit_type <<
              data_format << QVector<quint8>::fromStdVector(band_map) << QVector<quint8>::fromStdVector(axes) <<
              palette_size << byte_size;

    if(palette_size)
    {
        stream.writeRawData(data_buffer + offset, palette_size);

        offset += palette_size;
    }

    QString unit_type_s = "user type";

    if(unit_type != trace_x::T_CUSTOM)
    {
        unit_type_s = trace_x::data_type_names[unit_type];
    }
    else if(data_format & trace_x::QT_IMAGE_FORMAT)
    {
        unit_type_s = trace_x::data_type_names[trace_x::T_8U];
    }

    stream.writeRawData(data_buffer + offset, byte_size);

    if(size.size() == 2)
    {
        message = QString(" [%1x%2 ; %3 ; %4 ch] ").arg(size[1]).arg(size[0]).arg(unit_type_s).arg(channels);
    }
    else if(size.empty())
    {
        message = " [empty]";
    }

    if(!message_string.isEmpty())
    {
        message += ": " + message_string;
    }

    return array;
}

}

TraceController::TraceController(QObject *parent):
    QObject(parent),
    _start_point(0),
    _zero_time(0),
    _pid_colorer(0),
    _tid_colorer(0),
    _context_colorer(0),
    _module_colorer(0),
    _model_updated(false),
    _index_updated(false),
    _index_counter(0),
    _message_limit(10000000),
    _file_data_limit(10 * 1024 * 1024)
{
    X_CALL;

    for(int i = 0; i < trace_x::_MESSAGE_END_; ++i)
    {
        _full_message_types.append(new MessageTypeItem(trace_x::MessageType(i)));
    }

    _items_hash[ProcessNameEntity]  = &_process_names;
    _items_hash[ProcessIdEntity]    = &_process_models;
    _items_hash[ProcessUserEntity]  = &_process_users;
    _items_hash[ModuleNameEntity]   = &_modules;
    _items_hash[ThreadIdEntity]     = &_threads;
    _items_hash[ContextIdEntity]    = &_contexts;
    _items_hash[ClassNameEntity]    = &_classes;
    _items_hash[FunctionNameEntity] = &_functions;
    _items_hash[SourceNameEntity]   = &_sources;
    _items_hash[MessageTypeEntity]  = &_full_message_types;
    _items_hash[LabelNameEntity]    = &_labels;

    initialize();

    //

    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &TraceController::check_updates);

    timer->start(100);

    //

    _tx_model_service = new TransmitterModelService(this);
    _trace_model_service = new TraceModelService(this);
}

TraceController::~TraceController()
{
    X_CALL;

    emit close_connections();

    delete _tx_model_service;
    delete _trace_model_service;

    qDeleteAll(_process_models);
    qDeleteAll(_modules);
    qDeleteAll(_classes);
    qDeleteAll(_functions);
    qDeleteAll(_sources);
    qDeleteAll(_process_names);
    qDeleteAll(_process_users);
    qDeleteAll(_contexts);
    qDeleteAll(_threads);
    qDeleteAll(_labels);
    qDeleteAll(_message_types);
}

void TraceController::append(trace_message_t *message, bool register_only, const char *data_buffer)
{
    X_CALL;

    //this function may be called from different threads

    QMutexLocker lock(&_append_mutex);

    //TODO: можно не вставлять каждый раз, проверив новый ли message->type и _model_updated ?

    auto result = _trace_index.insert(message_index_t(message->type, message->process_index, message->module_index,
                                                      message->tid_index, message->context_index,
                                                      message->function_index, message->source_index, message->label_index));

    if(result.second && (message->type < trace_x::_MESSAGE_END_))
    {
        //new index item

        _trace_model_service->register_index(result.first);

        _index_updated = true;
    }

    if(!register_only)
    {
        if(_main_trace._trace_list.size() > _message_limit)
        {
            emit truncated();

            int to_remove = _message_limit * 0.1; // 10 %

            QList<const trace_message_t*> erased = _main_trace._trace_list.mid(0, to_remove);

            _main_trace.lock();
            _trace_model_service->lock();

            if(_trace_model_service->_last_index < size_t(erased.size()))
            {
                _trace_model_service->_last_index = 0;
            }
            else
            {
                _trace_model_service->_last_index -= erased.size();
            }

            _main_trace._trace_list.erase(_main_trace._trace_list.begin(), _main_trace._trace_list.begin() + to_remove);

            X_INFO("erase {} messages", erased.size());

            _main_trace._safe_size = _main_trace.size();

            foreach (const trace_message_t *message, erased)
            {
                _data_storage.remove_from_tail(message->index);
                _trace_model_service->remove_from_tail(message->index);
            }

            qDeleteAll(erased);

            _trace_model_service->unlock();

            _main_trace.unlock();

            _trace_model_service->update_all_data();
        }

        //

        _main_trace.lock();

        message->index = _index_counter++;

        //

        if(message->type == trace_x::MESSAGE_IMAGE)
        {
            QString description;

            QByteArray data_array = ::make_data_array(data_buffer, description);

            message->message_text += description;

            _data_storage.append_data(message->index, data_array);
        }

        //

        _main_trace._trace_list.append(message);

        _main_trace._has_new_messages = true;

        _main_trace.unlock();
    }
}

EntityItem *TraceController::item_by_descriptor_id(EntityClass class_id, QVariant item_id) const
{
    X_CALL;

    foreach (EntityItem *item, items_by_class(class_id, true))
    {
        if(item->descriptor().id == item_id)
        {
            return item;
        }
    }

    return 0;
}

ProcessModel * TraceController::register_process(uint64_t pid, uint64_t timestamp, const QString &process_name, const QString &user_name, trace_x::filter_index index_container)
{
    X_CALL;

    QMutexLocker locker(&_index_mutex);

    ProcessModel *process_item;

    QHash<quint64, pid_index_t>::const_iterator it = _process_id_hash.find(pid);

    if(it == _process_id_hash.end())
    {
        if(!_start_point)
        {
            _start_point = QDateTime::currentMSecsSinceEpoch();

            _zero_time = timestamp;
        }

        //

        pid_index_t index = _process_models.size();

        _process_id_hash.insert(pid, index);

        process_item = new ProcessModel(_pid_colorer.next(), index, this, pid, timestamp, process_name, user_name, index_container);

        _process_models.append(process_item);

        //

        process_item->_name_index = register_process_name(process_name);
        process_item->_user_index = register_user_name(user_name);

        _model_updated = true;

        //

        register_message_type(trace_x::MESSAGE_CONNECTED);

        trace_message_t *trace_message = new trace_message_t;

        trace_message->type = trace_x::MESSAGE_CONNECTED;
        trace_message->timestamp = 0;
        trace_message->process_index = index;
        trace_message->call_level = 0;
        trace_message->module_index = 0;
        trace_message->tid_index = 0;
        trace_message->label_index = 0;
        trace_message->function_index = 0;
        trace_message->context_index = 0;
        trace_message->source_index = 0;
        trace_message->flags = 0;
        trace_message->message_text = QObject::tr("Process connected: \"%1\"[%2]").arg(process_name).arg(pid);

        append(trace_message);
    }
    else
    {
        process_item = static_cast<ProcessModel*>(_process_models.at(it.value()));

        process_item->_index_container = index_container;
    }

    return process_item;
}

pid_index_t TraceController::register_process_name(const QString &process_name)
{
    X_CALL;

    pid_index_t index = 0;

    QHash<QString, pid_index_t>::const_iterator it = _process_name_hash.find(process_name);

    if(it == _process_name_hash.end())
    {
        index = _process_names.size();

        _process_name_hash.insert(process_name, index);

        EntityItem *process_name_item = new EntityItem(ProcessNameEntity, process_name, QFileInfo(process_name).baseName(), index);

        process_name_item->setToolTip(tr("Process path: ") + QDir::fromNativeSeparators(process_name));

        _process_names.append(process_name_item);
    }
    else
    {
        index = it.value();
    }

    return index;
}

pid_index_t TraceController::register_user_name(const QString &user_name)
{
    X_CALL;

    pid_index_t index;

    QHash<QString, pid_index_t>::const_iterator it = _process_user_hash.find(user_name);

    if(it == _process_user_hash.end())
    {
        index = _process_users.size();

        _process_user_hash.insert(user_name, index);

        EntityItem *user_item = new EntityItem(ProcessUserEntity, user_name, user_name, index);
        user_item->setToolTip(tr("Process user: ") + user_name);

        _process_users.append(user_item);
    }
    else
    {
        index = it.value();
    }

    return index;
}

void TraceController::register_message_type(uint8_t type)
{
    X_CALL;

    if(!_message_type_set.contains(type))
    {
        _message_type_set.insert(type);

        _message_types.append(new MessageTypeItem(trace_x::MessageType(type)));
    }
}

void TraceController::clear()
{
    clear_trace(true);
}

void TraceController::clear_trace(bool disconnect)
{
    X_CALL;

    emit cleaned();

    _loaded_file_name.clear();

    emit trace_source_changed();

    if(disconnect)
    {
        emit close_connections();
    }

    //TODO где локать и анлокать?

    _main_trace.lock();

    //

    _trace_model_service->clear();
    _tx_model_service->clear();

    //

    qDeleteAll(_main_trace._trace_list);

    _main_trace._safe_size = 0;
    _main_trace._trace_list = QList<const trace_message_t*>();

    _data_storage.clear();

    //

    _start_point = 0;
    _zero_time = 0;

    clear_indexes();

    _main_trace.unlock();

    emit _main_trace.cleaned();
    emit _main_trace.updated();
}

void TraceController::set_message_limit(uint64_t limit)
{
    X_CALL;

    _message_limit = limit;

    X_VALUE(_message_limit);
}

uint64_t TraceController::message_limit() const
{
    return _message_limit;
}

void TraceController::check_updates()
{
    //TODO наверное можно убрать
    if(_model_updated)
    {
        _model_updated = false;

        emit model_updated();
    }

    if(_index_updated)
    {
        _index_updated = false;

        emit index_updated();
    }
}

void TraceController::clear_indexes()
{
    X_CALL;

    _trace_index = trace_index_t();

    qDeleteAll(_process_models);
    qDeleteAll(_modules);
    qDeleteAll(_classes);
    qDeleteAll(_functions);
    qDeleteAll(_sources);
    qDeleteAll(_process_names);
    qDeleteAll(_process_users);
    qDeleteAll(_contexts);
    qDeleteAll(_threads);
    qDeleteAll(_labels);
    qDeleteAll(_message_types);

    _process_models = QList<EntityItem*>();
    _modules = QList<EntityItem*>();
    _classes = QList<EntityItem*>();
    _functions = QList<EntityItem*>();
    _sources = QList<EntityItem*>();
    _process_names = QList<EntityItem*>();
    _process_users = QList<EntityItem*>();
    _contexts = QList<EntityItem*>();
    _threads = QList<EntityItem*>();
    _labels = QList<EntityItem*>();
    _message_types = QList<EntityItem*>();

    _process_id_hash = QHash<quint64, pid_index_t>();
    _process_name_hash = QHash<QString, pid_index_t>();
    _process_user_hash = QHash<QString, pid_index_t>();
    _modules_hash = QHash<QString, module_index_t>();
    _sources_hash = QHash<QString, source_index_t>();
    _functions_hash = QHash<FunctionID, function_index_t>();
    _class_hash = QHash<QString, class_index_t>();
    _variable_hash = QHash<QString, label_index_t>();
    _message_type_set = QSet<uint8_t>();

    _pid_colorer.reset();
    _tid_colorer.reset();
    _context_colorer.reset();
    _module_colorer.reset();

    initialize();
}

void TraceController::initialize()
{
    X_CALL;

    //initialize with reserved zero-indexes

    ColorPair cp = _context_colorer.next();

    IDItem *global_context_item = new ContextEntityItem(0, "<global>", 0, 0, 0, cp.bg_color, cp.fg_color);

    global_context_item->setToolTip(tr("outside of the context"));

    _contexts.append(global_context_item);

    //

    _threads.append(new IDItem(ThreadIdEntity, "", 0, 0, 0, 0, QColor(), QColor()));

    //

    _classes.append(new EntityItem(ClassNameEntity, 0, "<global>", 0, QColor(), QColor(), tr("outside of the class")));
    _class_hash.insert("<global>", 0);

    //

    _labels.append(new EntityItem(LabelNameEntity, 0, "", 0, QColor(), QColor(), ""));
    _variable_hash.insert("", 0);

    //

    _modules.append(new EntityItem(ModuleNameEntity, 0, "", 0, QColor(), QColor(), ""));
    _modules_hash.insert("", 0);

    //

    _sources.append(new EntityItem(SourceNameEntity, "", "", 0, QColor(), QColor(), ""));
    _sources_hash.insert("", 0);

    //

    _functions.append(new FunctionEntityItem(function_t(), 0, 0));
    _functions_hash.insert(FunctionID(), 0);
}

uint64_t TraceController::file_data_limit() const
{
    return _file_data_limit;
}

void TraceController::set_file_data_limit(uint64_t file_data_limit)
{
    X_VALUE(_file_data_limit);

    _file_data_limit = file_data_limit;
}

module_index_t TraceController::register_module(const QString &module, const ProcessModel &process)
{
    X_CALL;

    QMutexLocker locker(&_index_mutex);

    module_index_t index = 0;

    QHash<QString, module_index_t>::const_iterator it = _modules_hash.find(module);

    if(it == _modules_hash.end())
    {
        index = _modules.size();

        _modules_hash.insert(module, index);

        ColorPair cp = _module_colorer.next();

        _modules.append(new EntityItem(ModuleNameEntity, module, module, index, cp.bg_color, cp.fg_color));

        _model_updated = true;
    }
    else
    {
        index = it.value();
    }

    return index;
}

source_index_t TraceController::register_source_file(const QString &path)
{
    X_CALL;

    QMutexLocker locker(&_index_mutex);

    source_index_t index = 0;

    QHash<QString, source_index_t>::const_iterator it = _sources_hash.find(path);

    if(it == _sources_hash.end())
    {
        index = _sources.size();

        _sources_hash.insert(path, index);

        _sources.append(new EntityItem(SourceNameEntity, QDir::fromNativeSeparators(path), QFileInfo(path).fileName(), index));

        _model_updated = true;
    }
    else
    {
        index = it.value();
    }

    return index;
}

function_index_t TraceController::register_function(const function_t &function, source_index_t source_index, bool has_context)
{
    X_CALL;

    //Регистрируем сигнатуру функции и имя класса одновременно

    //Т.к сигнатура функции может быть одинаковая для разных единиц трансляции,
    //то дополнительным параметром считаем полный путь к файлу(TODO: но может произойти и их коллизия, если объекты собирались на разных машинах)
    //TODO: добавить имя модуля для верности

    //Вообще говоря, с именем класса та же самая проблема, однако реализация методов класса может быть разделена,
    //поэтому на данный момент имя класса является уникальным в одиночку

    QMutexLocker locker(&_index_mutex);

    class_index_t class_index = 0;

    //Пока только таким образом мы можем отличить имя класса от namespace и понять, что эта функция - метод класса
    if(has_context/* && (function.spec == "__thiscall")*/)
    {
        QString class_name = function.namespace_string;

        QHash<QString, class_index_t>::const_iterator it = _class_hash.find(class_name);

        if(it == _class_hash.end())
        {
            class_index = _classes.size();

            _class_hash.insert(class_name, class_index);

            _classes.append(new EntityItem(ClassNameEntity, function.namespace_string, function.namespace_list.last(), class_index));

            X_INFO("new class: {} [#{}]", class_name, class_index);

            _model_updated = true;
        }
        else
        {
            class_index = it.value();
        }
    }

    //

    function_index_t fun_index = 0;

    FunctionID function_id(function.full_name, source_index);

    QHash<FunctionID, function_index_t>::const_iterator it = _functions_hash.find(function_id);

    if(it == _functions_hash.end())
    {
        fun_index = _functions.size();

        _functions_hash.insert(function_id, fun_index);

        _functions.append(new FunctionEntityItem(function, fun_index, class_index));

        _model_updated = true;
    }
    else
    {
        fun_index = it.value();
    }

    return fun_index;
}

tid_index_t TraceController::register_thread(const ProcessModel &process, quint64 tid)
{
    X_CALL;

    QMutexLocker locker(&_index_mutex);

    tid_index_t index = _threads.size();

    ColorPair cp = _tid_colorer.next();

    IDItem *thread_item = new IDItem(ThreadIdEntity, QString("%1").arg(tid), process.index() + 1,
                                     process._thread_index_hash.size() + 1, tid, index, cp.bg_color, cp.fg_color);

    thread_item->setToolTip(thread_item->text());

    _threads.append(thread_item);

    _model_updated = true;

    return index;
}

context_index_t TraceController::register_context(const ProcessModel &process, quint64 context, function_index_t function_index)
{
    X_CALL;

    QMutexLocker locker(&_index_mutex);

    context_index_t index = _contexts.size();

    ColorPair cp = _context_colorer.next();

    IDItem *context_item = new ContextEntityItem(context, class_at(function_index)->text(), process.index() + 1,
                                                 process._context_index_hash.size(), index, cp.bg_color, cp.fg_color);

    _contexts.append(context_item);

    _model_updated = true;

    return index;
}

label_index_t TraceController::register_label(const QString &var_name)
{
    X_CALL;

    QMutexLocker locker(&_index_mutex);

    label_index_t index = 0;

    QHash<QString, label_index_t>::const_iterator it = _variable_hash.find(var_name);

    if(it == _variable_hash.end())
    {
        index = _labels.size();

        _variable_hash.insert(var_name, index);

        _labels.append(new EntityItem(LabelNameEntity, var_name, var_name, index));

        _model_updated = true;
    }
    else
    {
        index = it.value();
    }

    return index;
}

QString TraceController::message_text_at(const trace_message_t *message) const
{
    if(message->type > trace_x::MESSAGE_RETURN)
    {
        return message->message_text;
    }

    return function_at(message)->toolTip();
}

QList<const trace_message_t *> TraceController::get_callers(const trace_message_t *message) const
{
    X_CALL;

    // Парсим трассу вверх от сообщения message

    // Проверяем только функции :
    // - с типом MESSAGE_CALL
    // - от такого же процесса, что и message
    // - из того же потока, что и message

    // Уменьшаем счётчик next_level, пока не достигнем нуля

    QList<const trace_message_t *> call_stack;

    if(message)
    {
        if(message->type == trace_x::MESSAGE_CALL)
        {
            call_stack.prepend(message);
        }

        int16_t next_level = message->call_level;

        if(message->type != trace_x::MESSAGE_RETURN)
        {
            next_level--;
        }

        if(next_level >= 0)
        {
            QMutexLocker lock(_main_trace.mutex());

            index_t start_index = _main_trace._trace_list.first()->index;

            //TODO omp
            for(int i = message->index - 1 - start_index; i >= 0; --i)
            {
                const trace_message_t *next_message = _main_trace.at(i);

                if((next_message->type == trace_x::MESSAGE_CALL) &&
                        next_message->in_same_thread(message) &&
                        (next_message->call_level == next_level))
                {
                    call_stack.prepend(next_message);

                    next_level--;

                    if(next_message->call_level == 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    return call_stack;
}

QList<const trace_message_t *> TraceController::get_callees(const trace_message_t *message, trace_message_t &ret_message) const
{
    X_CALL;

    // Парсим трассу вниз от сообщения message

    // Проверяем только функции :
    // - с типом MESSAGE_CALL
    // - от такого же процесса, что и message
    // - из того же потока, что и message
    // - с уровнем на единицу больше, чем уровень функции message

    // Выходим из цикла поиска, когда найдём RETURN из функции message-а

    QList<const trace_message_t *> call_stack;

    ret_message.source_line = 0;

    if(message && (message->type != trace_x::MESSAGE_RETURN))
    {
        int16_t next_level = message->call_level;

        if(message->type == trace_x::MESSAGE_CALL)
        {
            next_level++;
        }

        QMutexLocker lock(_main_trace.mutex());

        index_t start_index = _main_trace._trace_list.first()->index;

        for(size_t i = message->index + 1 - start_index; i < _main_trace.size(); ++i)
        {
            const trace_message_t *next_message = _main_trace.at(i);

            if(message->in_same_thread(next_message))
            {
                ret_message = *next_message;

                if((next_message->type == trace_x::MESSAGE_CALL) && (next_message->call_level == next_level))
                {
                    call_stack.append(next_message);
                }
                else if((next_message->type == trace_x::MESSAGE_RETURN) && (next_message->call_level == next_level - 1))
                {
                    break;
                }
            }
        }
    }

    if(!ret_message.source_line)
    {
        ret_message = *message;
    }

    return call_stack;
}

QList<const trace_message_t *> TraceController::get_callstack(const trace_message_t *message, int &call_index, uint64_t &duration)
{
    X_CALL;

    QList<const trace_message_t *> call_stack = get_callers(message);

    call_index = call_stack.size() - 1;

    trace_message_t ret_message;

    QList<const trace_message_t *> callees = get_callees(message, ret_message);

    call_stack.append(callees);

    if(!call_stack.isEmpty() && (call_index != -1))
    {
        duration = ret_message.timestamp - call_stack.at(call_index)->timestamp;
    }

    call_index = qMax(call_index, 0);

    return call_stack;
}

index_t TraceController::get_call_index(index_t current, bool &finded)
{
    QMutexLocker lock(_main_trace.mutex());

    if(!_main_trace.size())
    {
        finded = false;
        return 0;
    }

    current = _main_trace.relative_index(current);

    index_t result = current;

    const trace_message_t * message = _main_trace.at(current);

    int16_t this_level = message->call_level;

    if(message->type == trace_x::MESSAGE_CALL)
    {
        finded = true;
    }
    else
    {
        if(message->type != trace_x::MESSAGE_RETURN)
        {
            this_level--;
        }

        for(size_t i = current; i >= 0; --i)
        {
            const trace_message_t *next_message = _main_trace.at(i);

            if(message->in_same_thread(next_message))
            {
                if(next_message->call_level < this_level) //we lost CALL ?
                {
                    result = i;
                    finded = false;

                    break;
                }
                else if((next_message->call_level == this_level) && (next_message->type == trace_x::MESSAGE_CALL))
                {
                    result = i;
                    finded = true;

                    break;
                }
            }
        }
    }

    return _main_trace.trace_index(result);
}

index_t TraceController::get_return_index(index_t current, bool &finded)
{
    X_CALL;

    QMutexLocker lock(_main_trace.mutex());

    if(!_main_trace.size())
    {
        finded = false;
        return 0;
    }

    current = _main_trace.relative_index(current);

    index_t result = current;

    const trace_message_t * message = _main_trace.at(current);

    int16_t this_level = message->call_level;

    if(message->type == trace_x::MESSAGE_RETURN)
    {
        finded = true;
    }
    else
    {
        if(message->type != trace_x::MESSAGE_CALL)
        {
            this_level--;
        }

        for(size_t i = current; i < _main_trace.size(); ++i)
        {
            const trace_message_t *next_message = _main_trace.at(i);

            if(message->in_same_thread(next_message))
            {
                if(next_message->call_level < this_level) //?
                {
                    result = i;
                    finded = false;

                    break;
                }
                else if((next_message->call_level == this_level) && (next_message->type == trace_x::MESSAGE_RETURN))
                {
                    result = i;
                    finded = true;

                    break;
                }
            }
        }
    }

    return _main_trace.trace_index(result);
}

index_t TraceController::get_next_call(index_t current, bool &finded)
{
    X_CALL;

    QMutexLocker lock(_main_trace.mutex());

    if(!_main_trace.size())
    {
        finded = false;
        return 0;
    }

    current = _main_trace.relative_index(current);

    index_t result = current;

    const trace_message_t * message = _main_trace.at(current);

    for(size_t i = current + 1; i < _main_trace.size(); ++i)
    {
        const trace_message_t *next_message = _main_trace.at(i);

        if(message->in_same_thread(next_message))
        {
            if(next_message->type == trace_x::MESSAGE_CALL)
            {
                result = i;
                finded = true;

                break;
            }
        }
    }

    return _main_trace.trace_index(result);
}

index_t TraceController::get_prev_call(index_t current, bool &finded)
{
    X_CALL;

    QMutexLocker lock(_main_trace.mutex());

    if(!_main_trace.size())
    {
        finded = false;
        return 0;
    }

    current = _main_trace.relative_index(current);

    index_t result = current;

    const trace_message_t * message = _main_trace.at(current);

    for(intptr_t i = current - 1; i >= 0; --i)
    {
        const trace_message_t *next_message = _main_trace.at(i);

        if(message->in_same_thread(next_message))
        {
            if(next_message->type == trace_x::MESSAGE_CALL)
            {
                result = i;
                finded = true;

                break;
            }
        }
    }

    return _main_trace.trace_index(result);
}

trace_index_t &TraceController::trace_index()
{
    return _trace_index;
}

const trace_index_t &TraceController::trace_index() const
{
    return _trace_index;
}

QString TraceController::filter_class_name(int class_id) const
{
    return TraceEntityDescription::instance().entity_name(EntityClass(class_id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream & operator << (QDataStream &out, const trace_index_t &value)
{
    out << quint64(value.size());

    for(trace_index_t::iterator it = value.begin(); it != value.end(); ++it)
    {
        out << it->type;
        out << it->process_index;
        out << it->module_index;
        out << it->tid_index;
        out << it->context_index;
        out << it->function_index;
        out << it->source_index;
        out << it->label_index;
    }

    return out;
}

QDataStream & operator >> (QDataStream &in, trace_index_t &value)
{
    quint64 size;

    in >> size;

    for(quint64 i = 0; i < size; ++i)
    {
        message_index_t index;

        in >> index.type;
        in >> index.process_index;
        in >> index.module_index;
        in >> index.tid_index;
        in >> index.context_index;
        in >> index.function_index;
        in >> index.source_index;
        in >> index.label_index;

        value.insert(index);
    }

    return in;
}

QDataStream & operator << (QDataStream &out, const FunctionID &value)
{
    out << value.full_name;
    out << value.source_id;

    return out;
}

QDataStream & operator >> (QDataStream &in, FunctionID &value)
{
    in >> value.full_name;
    in >> value.source_id;

    return in;
}

QDataStream & operator << (QDataStream &out, QList<EntityItem*> &value)
{
    out << quint32(value.size());

    for(int i = 0; i < value.size(); ++i)
    {
        out << *value[i];
    }

    return out;
}

template<class T>
void read_entity_list(QDataStream &in, QList<EntityItem*> &value)
{
    qDeleteAll(value);

    value.clear();

    quint32 size = 0;

    in >> size;

    for(quint32 i = 0; i < size; ++i)
    {
        T *item = new T;

        in >> *item;

        value.push_back(item);
    }
}

QDataStream & operator << (QDataStream &out, const trace_message_t &value)
{
    out << quint64(value.index);
    out << value.type;
    out << quint64(value.timestamp);
    out << quint64(value.extra_timestamp);
    out << value.process_index;
    out << value.tid_index;
    out << value.context_index;
    out << value.module_index;
    out << value.function_index;
    out << value.label_index;
    out << value.source_index;
    out << value.source_line;
    out << value.call_level;
    out << value.message_text;

    return out;
}

QDataStream & operator >> (QDataStream &in, trace_message_t &value)
{
    quint64 index; in >> index; value.index = index;
    in >> value.type;
    quint64 timestamp; in >> timestamp; value.timestamp = timestamp;
    quint64 extra_timestamp; in >> extra_timestamp; value.extra_timestamp = extra_timestamp;
    in >> value.process_index;
    in >> value.tid_index;
    in >> value.context_index;
    in >> value.module_index;
    in >> value.function_index;
    in >> value.label_index;
    in >> value.source_index;
    in >> value.source_line;
    in >> value.call_level;
    in >> value.message_text;

    value.flags = 0;

    return in;
}

QDataStream & operator << (QDataStream &out, const QList<const trace_message_t*> &value)
{
    out << quint32(value.size());

    for(int i = 0; i < value.size(); ++i)
    {
        out << *value[i];
    }

    return out;
}

QDataStream & operator >> (QDataStream &in, QList<const trace_message_t*> &value)
{
    quint32 size = 0;

    in >> size;

    value.reserve(size);

    for(quint32 i = 0; i < size; ++i)
    {
        trace_message_t *message = new trace_message_t;

        in >> *message;

        value << message;
    }

    return in;
}

void TraceController::save_trace(const QString &name)
{
    X_CALL;

    X_INFO("save to {}", name);

    try
    {
        boost::filesystem::path boost_path = boost::filesystem::absolute(name.toStdString());

        boost::filesystem::create_directories(boost_path.parent_path());

        QFile trace_file(name);

        if (trace_file.open(QFile::WriteOnly | QFile::Truncate))
        {
            if(QFileInfo(trace_file).suffix() == "txt")
            {
                // План таков - для каждого столбца узнаём максимальную ширину

                //#    | Time     | Module          | T   | Type    | C   | Message
                //102  | 0.829066 | tracer_module_1 | 1_2 | INFO    | 1_0 | • • • • on test_call_stack_3() 1
                //103  | 0.829067 | tracer_module_1 | 1_2 | INFO    | 1_0 | • • • • on test_call_stack_3() 2

                // Не выводим колонку процессов, если процесс один
                // Также можно выводить состав индексов

                // for()
                // trace_list

                // можно также выводить в csv
            }
            else
            {
                // TODO write annotation

                QDataStream file_stream(&trace_file);

                quint16 version = 1;

                file_stream << version;
                file_stream << quint32(file_stream.version());

                QByteArray data_array;

                QDataStream stream(&data_array, QIODevice::WriteOnly);

                stream << _zero_time;
                stream << _start_point;
                stream << _trace_index;

                stream << _process_models;
                stream << _process_names;
                stream << _process_users;
                stream << _modules;
                stream << _sources;
                stream << _functions;
                stream << _classes;
                stream << _labels;
                stream << _threads;
                stream << _contexts;
                stream << _message_types;

                stream << _main_trace.trace_list();

                QByteArray compressed_data = qCompress(data_array);

                file_stream << compressed_data;

                if(_data_storage.total_data_size() <= _file_data_limit)
                {
                    // save data into trace file

                    _data_storage.copy_data_to(&trace_file);
                }
                else
                {
                    // save data into separate file

                    QFile data_file(name + ".data");

                    if(data_file.open(QFile::WriteOnly | QFile::Truncate))
                    {
                        _data_storage.copy_data_to(&data_file);
                    }
                }
            }
        }
    }
    catch(std::exception &e)
    {
        X_ERROR("can`t create directories for {} : {}", name, e.what());
    }
}

bool TraceController::load_trace(const QString &file_name)
{
    X_CALL;

    X_INFO("load trace: {}", file_name);

    QFile trace_file(file_name);

    if(trace_file.open(QFile::ReadOnly))
    {
        emit stop_server();

        clear();

        QDataStream file_stream(&trace_file);

        quint16 version;
        quint32 stream_verion;

        file_stream >> version;
        file_stream >> stream_verion;
        file_stream.setVersion(stream_verion);

        QByteArray data_array;

        file_stream >> data_array;

        QDataStream stream(qUncompress(data_array));

        stream.setVersion(stream_verion);

        stream >> _zero_time;
        stream >> _start_point;
        stream >> _trace_index;

        read_entity_list<ProcessModel>(stream, _process_models);

        foreach (EntityItem* item, _process_models)
        {
            static_cast<ProcessModel*>(item)->_trace_controller = this;
        }

        read_entity_list<EntityItem>(stream, _process_names);
        read_entity_list<EntityItem>(stream, _process_users);
        read_entity_list<EntityItem>(stream, _modules);
        read_entity_list<EntityItem>(stream, _sources);
        read_entity_list<FunctionEntityItem>(stream, _functions);
        read_entity_list<EntityItem>(stream, _classes);
        read_entity_list<EntityItem>(stream, _labels);
        read_entity_list<IDItem>(stream, _threads);
        read_entity_list<ContextEntityItem>(stream, _contexts);
        read_entity_list<MessageTypeItem>(stream, _message_types);

        QList<const trace_message_t*> trace_list;

        stream >> trace_list;

        QString data_file = file_name + ".data";

        qint64 data_offset = 0;

        if(!QFileInfo(data_file).exists())
        {
            data_file = file_name;

            data_offset = trace_file.pos();
        }

        _data_storage.set_swap_file(data_file, data_offset);

        _model_updated = true;

        _main_trace.set_message_list(trace_list);

        _loaded_file_name = file_name;

        emit trace_source_changed();

        _trace_model_service->invalidate_all_filters();

        return true;
    }
    else
    {
        X_ERROR("can`t load file {}", file_name);
    }

    return false;
}

QString TraceController::loaded_file() const
{
    return _loaded_file_name;
}

QByteArray TraceController::data_at(const trace_message_t *message)
{
    return _data_storage.request_data(message->index);
}
