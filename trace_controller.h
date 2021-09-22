#ifndef TRACE_CONTROLLER_H
#define TRACE_CONTROLLER_H

#include <QObject>
#include <QMutex>

#include <boost/chrono.hpp>
#include <boost/chrono/duration.hpp>

#include "trace_model.h"
#include "trace_data_model.h"

#include "process_model.h"
#include "tx_index.h"
#include "color_generator.h"

#include "local_connection_controller.h"

#include "tx_model_service.h"
#include "trace_model_service.h"

#include "data_storage.h"

struct FunctionID
{
    FunctionID(): full_name(), source_id(0) {}
    FunctionID(const QString &name, source_index_t id): full_name(name), source_id(id) {}

    QString full_name;
    source_index_t source_id;
};

inline bool operator==(const FunctionID &f1, const FunctionID &f2)
{
    return (f1.source_id == f2.source_id) && (f1.full_name == f1.full_name);
}

inline uint qHash(const FunctionID &f, uint seed)
{
    //TODO: think about that

    size_t seed_s = seed;

    boost::hash_combine(seed_s, f.full_name.toStdString());
    boost::hash_combine(seed_s, f.source_id);

    return uint(seed_s);
}

//! Класс управления трассой.
//! Содержит единый список всех сообщений и элементы модели трассы.
//! Отвечает за идексацию элементов модели трассы.
//! Включает вспомогательный сервисы CaptureModelService и TraceModelService.
class TraceController : public QObject
{
    Q_OBJECT

public:
    explicit TraceController(QObject *parent = 0);
    ~TraceController();

    void append(trace_message_t *message, bool register_only = false, const char* data_buffer = 0);

    QMutex * index_mutex() { return &_index_mutex; }

    //

    ProcessModel * register_process(uint64_t pid, uint64_t timestamp, const QString &process_name, const QString &user_name, trace_x::filter_index index_container);

    pid_index_t register_process_name(const QString &process_name);
    pid_index_t register_user_name(const QString &user_name);

    inline EntityItem * process_item_at(pid_index_t index) const;
    inline EntityItem * process_item_at(const trace_message_t *message) const;
    inline const ProcessModel & process_at(const trace_message_t *message) const;
    inline const ProcessModel & process_at(pid_index_t index) const;

    void register_message_type(uint8_t type);

    inline EntityItem * message_type_at(int index) const;

    inline EntityItem * process_name_at(const trace_message_t *message) const;
    inline EntityItem * process_name_at(pid_index_t index) const;

    inline EntityItem * process_user_at(const trace_message_t *message) const;
    inline EntityItem * process_user_at(pid_index_t index) const;

    module_index_t register_module(const QString &module, const ProcessModel &process);

    inline EntityItem * module_at(module_index_t index) const;
    inline EntityItem * module_at(const trace_message_t *message) const;

    source_index_t register_source_file(const QString &path);
    inline EntityItem * source_at(const trace_message_t *message) const;
    inline EntityItem * source_at(source_index_t index) const;

    function_index_t register_function(const function_t &function, source_index_t source_index, bool has_context);
    inline EntityItem * function_at(const trace_message_t *message) const;
    inline EntityItem * function_at(function_index_t index) const;

    inline EntityItem * class_at(const trace_message_t *message) const;
    inline EntityItem * class_at(function_index_t index) const;
    inline EntityItem * class_at(class_index_t index) const;

    tid_index_t register_thread(const ProcessModel &process, quint64 tid);
    inline EntityItem * thread_item_at(const trace_message_t *message) const;
    inline EntityItem * thread_item_at(tid_index_t index) const;

    context_index_t register_context(const ProcessModel &process, quint64 context, function_index_t function_index);
    inline EntityItem * context_item_at(const trace_message_t *message) const;
    inline EntityItem * context_item_at(context_index_t index) const;

    label_index_t register_label(const QString &var_name);
    inline EntityItem * label_item_at(const trace_message_t *message) const;
    inline EntityItem * label_item_at(label_index_t index) const;

    QString message_text_at(const trace_message_t *message) const;

    QByteArray data_at(const trace_message_t *message);

    //

    QList<EntityItem*> * process_models() { return &_process_models; }
    QList<EntityItem*> * process_names() { return &_process_names; }
    QList<EntityItem*> * process_users() { return &_process_users; }
    QList<EntityItem*> * modules() { return &_modules; }
    QList<EntityItem*> * sources() { return &_sources; }
    QList<EntityItem*> * functions() { return &_functions; }
    QList<EntityItem*> * classes() { return &_classes; }
    QList<EntityItem*> * variables() { return &_labels; }
    QList<EntityItem*> * threads() { return &_threads; }
    QList<EntityItem*> * contexts() { return &_contexts; }
    QList<EntityItem*> * message_types() { return &_message_types; }
    QList<EntityItem*> * full_message_types() { return &_full_message_types; }

    //

    inline const TraceDataModel &trace_model() const;
    inline TraceDataModel &trace_model();

    inline DataStorage &data_storage();

    inline TransmitterModelService &tx_model_service();
    inline TraceModelService &trace_model_service();

    inline quint64 zero_time() const;
    inline quint64 start_point() const;

    inline FilterModel *capture_filter() const;

    //

    inline const QList<EntityItem *> &items_by_class(EntityClass class_id, bool full_types = false) const;

    inline size_t index_by_class(const trace_message_t *message, int class_id) const;

    EntityItem * item_by_descriptor_id(EntityClass class_id, QVariant item_id) const;

    inline const QHash<int, QList<EntityItem *> *> &items_hash() const;

    //

    //! Получить список функций с типом CALL, из которых была вызвана функция с сообщением message
    QList<const trace_message_t*> get_callers(const trace_message_t *message) const;

    //! Получить список функций с типом CALL, которые вызваны из функции с сообщением message
    QList<const trace_message_t*> get_callees(const trace_message_t *message, trace_message_t &ret_message) const;

    QList<const trace_message_t*> get_callstack(const trace_message_t *message, int &stack_index, uint64_t &duration);

    //TODO: move to TraceDataModel ?

    index_t get_call_index(index_t current, bool &finded);
    index_t get_return_index(index_t current, bool &finded);
    index_t get_next_call(index_t current, bool &finded);
    index_t get_prev_call(index_t current, bool &finded);

    trace_index_t & trace_index();
    const trace_index_t & trace_index() const;

    //

    QString filter_class_name(int class_id) const;

    void save_trace(const QString &file_name);
    bool load_trace(const QString &file_name);

    QString loaded_file() const;

public:
    void set_message_limit(uint64_t limit);
    uint64_t message_limit() const;

    uint64_t file_data_limit() const;
    void set_file_data_limit(uint64_t file_data_limit);

signals:
    void model_updated();
    void index_updated();

    void close_connections();
    void trace_source_changed();
    void cleaned();
    void truncated();

    //commands
signals:
    void stop_server();
    void show_gui();

public slots:
    void clear();

private:
    void check_updates();
    void clear_indexes();
    void initialize();
    void clear_trace(bool disconnect);

private:
    friend class ProcessModel;
    friend class TransmitterModelService;
    friend class TraceModelService;
    friend class LocalConnectionController;
    friend class GeneralSettingWidget;

private:
    TransmitterModelService *_tx_model_service;
    TraceModelService *_trace_model_service;

    QList<EntityClass> _entity_list;

    //! Таблица со списками элементов модели трассы, сгруппированных по классу
    QHash<int, QList<EntityItem*>*> _items_hash;

    //

    mutable TraceDataModel _main_trace;

    //! Индекс трассы, используется для фильтрации
    trace_index_t _trace_index;

    quint64 _zero_time;
    quint64 _start_point;

    //

    QHash<QString, pid_index_t> _process_name_hash;
    QHash<quint64, pid_index_t> _process_id_hash;
    QHash<QString, pid_index_t> _process_user_hash;
    QHash<QString, module_index_t> _modules_hash;
    QHash<QString, source_index_t> _sources_hash;
    QHash<FunctionID, function_index_t> _functions_hash;
    QHash<QString, class_index_t> _class_hash;
    QHash<QString, label_index_t> _variable_hash;
    QSet<uint8_t> _message_type_set;

    //

    QList<EntityItem*> _process_models;
    QList<EntityItem*> _process_names;
    QList<EntityItem*> _process_users;
    QList<EntityItem*> _modules;
    QList<EntityItem*> _sources;
    QList<EntityItem*> _functions;
    QList<EntityItem*> _classes;
    QList<EntityItem*> _labels;
    QList<EntityItem*> _threads;
    QList<EntityItem*> _contexts;
    QList<EntityItem*> _message_types;

    QList<EntityItem*> _full_message_types;
    QList<EntityItem*> _empty_list;

    //

    QMutex _index_mutex;
    QMutex _append_mutex;

    boost::atomic<bool> _model_updated;
    boost::atomic<bool> _index_updated;

    ColorGenerator _pid_colorer;
    ColorGenerator _tid_colorer;
    ColorGenerator _context_colorer;
    ColorGenerator _module_colorer;

    QString _loaded_file_name;

    uint64_t _index_counter;
    uint64_t _message_limit;
    uint64_t _file_data_limit;

    DataStorage _data_storage;
};

const TraceDataModel &TraceController::trace_model() const
{
    return _main_trace;
}

TraceDataModel &TraceController::trace_model()
{
    return _main_trace;
}

DataStorage &TraceController::data_storage()
{
    return _data_storage;
}

const QHash<int, QList<EntityItem *> *> &TraceController::items_hash() const
{
    return _items_hash;
}

EntityItem * TraceController::process_item_at(pid_index_t index) const
{
    return _process_models.at(index);
}

EntityItem * TraceController::process_item_at(const trace_message_t *message) const
{
    return _process_models.at(message->process_index);
}

const ProcessModel &TraceController::process_at(const trace_message_t *message) const
{
    return process_at(message->process_index);
}

const ProcessModel &TraceController::process_at(pid_index_t index) const
{
    return *static_cast<ProcessModel*>(_process_models.at(index));
}

EntityItem * TraceController::message_type_at(int index) const
{
    return _full_message_types.at(index);
}

EntityItem * TraceController::process_name_at(const trace_message_t *message) const
{
    return process_name_at(process_at(message).name_index());
}

EntityItem *TraceController::process_name_at(pid_index_t index) const
{
    return _process_names.at(index);
}

EntityItem * TraceController::process_user_at(const trace_message_t *message) const
{
    return process_user_at(process_at(message).user_index());
}

EntityItem *TraceController::process_user_at(pid_index_t index) const
{
    return _process_users.at(index);
}

EntityItem * TraceController::module_at(module_index_t index) const
{
    return _modules.at(index);
}

EntityItem * TraceController::module_at(const trace_message_t *message) const
{
    return module_at(message->module_index);
}

EntityItem * TraceController::source_at(const trace_message_t *message) const
{
    return source_at(message->source_index);
}

EntityItem * TraceController::source_at(source_index_t index) const
{
    return _sources.at(index);
}

EntityItem * TraceController::function_at(const trace_message_t *message) const
{
    return function_at(message->function_index);
}

EntityItem * TraceController::function_at(function_index_t index) const
{
    return _functions.at(index);
}

EntityItem * TraceController::class_at(const trace_message_t *message) const
{
    return class_at(static_cast<FunctionEntityItem*>(function_at(message))->_class_index);
}

EntityItem *TraceController::class_at(function_index_t index) const
{
    return class_at(static_cast<FunctionEntityItem*>(function_at(index))->_class_index);
}

EntityItem *TraceController::class_at(class_index_t index) const
{
    return _classes.at(index);
}

EntityItem * TraceController::thread_item_at(const trace_message_t *message) const
{
    return _threads.at(message->tid_index);
}

EntityItem *TraceController::thread_item_at(tid_index_t index) const
{
    return _threads.at(index);
}

EntityItem * TraceController::context_item_at(const trace_message_t *message) const
{
    return _contexts.at(message->context_index);
}

EntityItem *TraceController::context_item_at(context_index_t index) const
{
    return _contexts.at(index);
}

EntityItem *TraceController::label_item_at(const trace_message_t *message) const
{
    return _labels.at(message->label_index);
}

EntityItem *TraceController::label_item_at(label_index_t index) const
{
    return _labels.at(index);
}

TransmitterModelService &TraceController::tx_model_service()
{
    return *_tx_model_service;
}

TraceModelService &TraceController::trace_model_service()
{
    return *_trace_model_service;
}

quint64 TraceController::zero_time() const
{
    return _zero_time;
}

quint64 TraceController::start_point() const
{
    return _start_point;
}

FilterModel * TraceController::capture_filter() const
{
    return _tx_model_service->active_filter();
}

const QList<EntityItem *> & TraceController::items_by_class(EntityClass class_id, bool full_types) const
{
    if(!full_types && (class_id == MessageTypeEntity))
    {
        return _message_types;
    }

    if(_items_hash.contains(class_id))
    {
        return *_items_hash.value(int(class_id));
    }

    return _empty_list;
}

size_t TraceController::index_by_class(const trace_message_t *message, int class_id) const
{
    switch (class_id)
    {
    case ProcessNameEntity:  return process_at(message).name_index();
    case ProcessIdEntity:    return message->process_index;
    case ProcessUserEntity:  return process_at(message).user_index();
    case ModuleNameEntity:   return message->module_index;
    case ThreadIdEntity:     return message->tid_index;
    case ContextIdEntity:    return message->context_index;
    case ClassNameEntity:    return (static_cast<FunctionEntityItem*>(function_at(message)))->_class_index;
    case FunctionNameEntity: return message->function_index;
    case SourceNameEntity:   return message->source_index;
    case MessageTypeEntity:  return message->type;
    case LabelNameEntity:    return message->label_index;
    case MessageTextEntity:  return 0;
    }

    return 0;
}

#endif // TRACE_CONTROLLER_H
