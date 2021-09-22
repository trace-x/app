#include "trace_model_service.h"

#include <QFileInfo>
#include <QThread>
#include <QFutureWatcher>

#include "settings.h"

#include "trace_controller.h"
#include "trace_table_model.h"

#include "trace_x/trace_x.h"

TraceModelService::TraceModelService(TraceController *trace_controller, QObject *parent):
    QObject(parent),
    _trace_controller(trace_controller),
    _filter_updated(false),
    _filter_interrupt_flag(false),
    _full_update(false),
    _last_index(0)
{
    X_CALL;

    _trace_filter_model = new FilterListModel(true, trace_controller, this);
    _subtrace_filter_model = new FilterListModel(false, trace_controller, this);
    _main_image_model = new TraceDataModel(this);

    _entity_model = new TraceEntityModel(trace_controller, x_settings().entity_model_layout, true, this);

    connect(&_filter_thread, &QThread::started, this, &TraceModelService::filter_loop, Qt::DirectConnection);
    connect(_trace_controller->tx_model_service().filter_model(), &FilterListModel::filter_changed, this, &TraceModelService::invalidate_all_filters);

    _filter_thread.start(QThread::HighestPriority);
}

TraceModelService::~TraceModelService()
{
    X_CALL;

    _filter_thread.requestInterruption();
    _filter_thread.quit();
    _filter_thread.wait();
}

QByteArray TraceModelService::save_state() const
{
    X_CALL;

    QByteArray state;

    QDataStream stream(&state, QIODevice::WriteOnly);

    stream << *_trace_filter_model;
    stream << *_subtrace_filter_model;

    return state;
}

void TraceModelService::restore_state(const QByteArray &state)
{
    X_CALL;

    _trace_filter_model->disconnect(this);
    _subtrace_filter_model->disconnect(this);

    if(!state.isEmpty())
    {
        QDataStream stream(state);

        stream >> *_trace_filter_model;
        stream >> *_subtrace_filter_model;
    }
    else
    {
        *_trace_filter_model = FilterListModel(true, _trace_controller, this);
        *_subtrace_filter_model = FilterListModel(false, _trace_controller, this);
    }

    connect(_trace_filter_model, &FilterListModel::filter_changed, this, &TraceModelService::update_trace_filter);
    connect(_trace_filter_model, &FilterListModel::filter_added, this, &TraceModelService::register_trace_filter);

    connect(_subtrace_filter_model, &FilterListModel::filter_changed, this, &TraceModelService::update_trace_filter);
    connect(_subtrace_filter_model, &FilterListModel::filter_added, this, &TraceModelService::register_subtrace_filter);

    for(int i = 0; i < _trace_filter_model->models().size(); ++i)
    {
        register_trace_filter(&_trace_filter_model->models()[i]);
    }

    for(int i = 0; i < _subtrace_filter_model->models().size(); ++i)
    {
        register_subtrace_filter(&_subtrace_filter_model->models()[i]);
    }
}

//! Обновляет весь индекс для фильтра model
void TraceModelService::update_trace_filter(FilterModel *model)
{
    X_CALL;

    trace_index_t &trace_index = _trace_controller->trace_index();

    QHash<int, ItemDescriptor> trace_message_desc;

    //first, update trace_index filter flags

    for(trace_index_t::iterator it = trace_index.begin(); it != trace_index.end(); ++it)
    {
        fill_descriptor(it, trace_message_desc);

        QMutexLocker locker(&_filter_mutex);

        QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

        while (map_iter.hasNext())
        {
            map_iter.next();

            const FilterChain &chain = map_iter.key();

            if((chain.first == model) || (chain.second == model))
            {
                update_filter_pair(it, chain, trace_message_desc);

                map_iter.value().is_up_to_date = false;
                map_iter.value().is_complex_filter = chain.first->has_class_in_filter(MessageTextEntity) || (chain.second && chain.second->has_class_in_filter(MessageTextEntity));
            }
        }
    }

    _filter_updated = true;
    _filter_interrupt_flag = true;
}

void TraceModelService::filter_destroyed(QObject *filter)
{
    //TODO фильтр уже уничтожен, а фильтрация по нему продолжается! при удалении Нужен lock

    remove_trace_filter(static_cast<FilterModel*>(filter));
}

void TraceModelService::subfilter_destroyed(QObject *filter)
{
    remove_subtrace_filter(static_cast<FilterModel*>(filter));
}

bool TraceModelService::filter_message_list(size_t start, size_t end, const trace_index_t &trace_index, bool is_new_messages)
{
    X_CALL;

    X_INFO("start = {} ; end = {}; is_new_messages = {}", start, end, is_new_messages);

    // Trace filtration in range [start, end]
    // Fills TraceData for each filter
    // Started in separate thread (TraceModelService::filter_loop)
    // Run for re-filtration of any filter update, or for new messages in main trace

    TraceDataModel &trace_data = _trace_controller->trace_model();

    if(end > trace_data.size())
    {
        return false;
    }

    for(size_t i = start; i < end; ++i)
    {
        if(!is_new_messages && _filter_interrupt_flag)
        {
            _filter_interrupt_flag = false;

            return false;
        }

        const trace_message_t *message = trace_data.at(i);

        if(is_new_messages)
        {
            _issue_model.append(message);

            //

            if(message->type == trace_x::MESSAGE_IMAGE)
            {
                _main_image_model->append(message);
                _main_image_model->update_index(message);
            }
        }

        auto message_index = trace_index.get<message_index_t::ByKey>().find(boost::make_tuple(message->type, message->process_index, message->module_index,
                                                                                              message->tid_index, message->context_index,
                                                                                              message->function_index, message->source_index, message->label_index));

        X_ASSERT(message_index != trace_index.get<message_index_t::ByKey>().end());

        QMutexLocker locker(&_filter_mutex);

        QMapIterator<FilterChain, FilterData> it(_filter_model_map);

        while (it.hasNext())
        {
            it.next();

            const FilterData &filter_data = it.value();

            if(is_new_messages || !filter_data.is_up_to_date)
            {
                bool accepted = message_index->filters_map[it.key()];

                if(filter_data.is_complex_filter)
                {
                    const FilterChain &pair = it.key();

                    accepted = true;

                    if(_trace_controller->capture_filter()->is_enabled())
                    {
                        accepted = _trace_controller->capture_filter()->check_filter(message);
                    }

                    if(accepted && pair.first && pair.first->is_enabled())
                    {
                        accepted = pair.first->check_filter(message);
                    }

                    if(accepted && pair.second)
                    {
                        accepted = pair.second->check_filter(message);
                    }
                }

                if(accepted)
                {
                    filter_data.model->append(message);
                    filter_data.model->update_index(message);

                    if(message->type == trace_x::MESSAGE_IMAGE)
                    {
                        filter_data.image_model->append(message);
                        filter_data.image_model->update_index(message);
                    }
                }
            }
        }
    }

    return true;
}

void TraceModelService::filter_loop()
{
    X_CALL;

    // runs in separate thread

    TraceDataModel &trace_data = _trace_controller->trace_model();

    while(!QThread::currentThread()->isInterruptionRequested())
    {
        if(_filter_updated)
        {
            {
                QMutexLocker locker(&_filter_mutex);

                QMutableMapIterator<FilterChain, FilterData> it(_filter_model_map);

                while (it.hasNext())
                {
                    it.next();

                    FilterData &filter_data = it.value();

                    if(!filter_data.is_up_to_date)
                    {
                        X_INFO("filter with data {} was updated", static_cast<void*>(filter_data.model));

                        filter_data.clear();
                    }
                }
            }

            // we need refilter all updated filters

            trace_data.lock();

            bool is_complete = filter_message_list(0, _last_index, _trace_controller->trace_index(), _full_update);

            trace_data.unlock();

            if(is_complete)
            {
                _full_update = false;
                _filter_updated = false;

                // set all filter data is up to date

                QMutexLocker locker(&_filter_mutex);

                QMutableMapIterator<FilterChain, FilterData> it(_filter_model_map);

                while (it.hasNext())
                {
                    it.next();

                    if(!it.value().is_up_to_date)
                    {
                        it.value().set_up_to_date();
                    }
                }
            }
        }

        //

        size_t size = 0;

        trace_data.lock();

        if(trace_data.size())
        {
            size = trace_data.size();

            if(_last_index != size)
            {
                filter_message_list(_last_index, size, _trace_controller->trace_index(), true);

                _last_index = size;
            }
        }

        trace_data.unlock();

        QThread::msleep(100);
    }
}

void TraceModelService::register_trace_filter(FilterModel *filter)
{
    X_CALL;

    connect(filter, &QObject::destroyed, this, &TraceModelService::filter_destroyed);

    _filter_mutex.lock();

    _filter_model_map[FilterChain(filter, 0)] = make_filter_data_model(filter, 0, filter);

    // create FilterData for all subfilters (for each combination filter+subfilter)

    foreach (const FilterModel &subfilter, _subtrace_filter_model->models())
    {
        _filter_model_map[FilterChain(filter, &subfilter)] = make_filter_data_model(filter, &subfilter, filter);
    }

    _filter_mutex.unlock();

    update_trace_filter(filter);
}

void TraceModelService::register_subtrace_filter(FilterModel *subfilter)
{
    X_CALL;

    connect(subfilter, &QObject::destroyed, this, &TraceModelService::subfilter_destroyed);

    // create FilterData for all subfilters (for each combination filter+subfilter)

    _filter_mutex.lock();

    foreach (const FilterModel &filter, _trace_filter_model->models())
    {
        _filter_model_map[FilterChain(&filter, subfilter)] = make_filter_data_model(&filter, subfilter, subfilter);
    }

    _filter_mutex.unlock();

    update_trace_filter(subfilter);
}

void TraceModelService::remove_trace_filter(FilterModel *filter)
{
    X_CALL;

    // QMutexLocker locker(&_filter_mutex);

    _filter_model_map.remove(FilterChain(filter, 0));

    foreach (const FilterModel &subfilter, _subtrace_filter_model->models())
    {
        _filter_model_map.remove(FilterChain(filter, &subfilter));
    }
}

void TraceModelService::remove_subtrace_filter(FilterModel *subfilter)
{
    X_CALL;

    // QMutexLocker locker(&_filter_mutex);

    _filter_model_map.remove(FilterChain(0, subfilter));

    foreach (const FilterModel &filter, _trace_filter_model->models())
    {
        _filter_model_map.remove(FilterChain(&filter, subfilter));
    }
}

TraceDataModel * TraceModelService::request_trace_model(const FilterChain &filter_pair)
{
    X_CALL;

    QMutexLocker locker(&_filter_mutex);

    if(!_trace_controller->capture_filter()->is_enabled() && !filter_pair.second && !filter_pair.first->is_enabled())
    {
        // Request trace without filter - main trace

        return &_trace_controller->trace_model();
    }

    return _filter_model_map[filter_pair].model;
}

TraceDataModel *TraceModelService::request_image_model(const FilterChain &filter_pair)
{
    QMutexLocker locker(&_filter_mutex);

    if(!filter_pair.second && !filter_pair.first->is_enabled())
    {
        // Request trace without filter -

        return _main_image_model;
    }

    return _filter_model_map[filter_pair].image_model;
}

TraceEntityModel *TraceModelService::request_index_item_model(const FilterChain &filter_pair)
{
    X_CALL;

    QMutexLocker locker(&_filter_mutex);

    if(!filter_pair.second && !filter_pair.first->is_enabled())
    {
        return _entity_model;
    }

    return _filter_model_map[filter_pair].index_item_model;
}

IssuesListModel * TraceModelService::issue_model()
{
    return &_issue_model;
}

void TraceModelService::fill_descriptor(const trace_index_t::iterator &it, QHash<int, ItemDescriptor> &descriptor)
{
    // Заполняет дескриптор по данным из элемента индексного контейнера

    const ProcessModel &process = _trace_controller->process_at(it->process_index);

    descriptor[ProcessIdEntity]    = process.descriptor();
    descriptor[ProcessNameEntity]  = _trace_controller->process_name_at(process.name_index())->descriptor();
    descriptor[ProcessUserEntity]  = _trace_controller->process_user_at(process.user_index())->descriptor();
    descriptor[ModuleNameEntity]   = _trace_controller->module_at(it->module_index)->descriptor();
    descriptor[FunctionNameEntity] = _trace_controller->function_at(it->function_index)->descriptor();
    descriptor[ClassNameEntity]    = _trace_controller->class_at(it->function_index)->descriptor();
    descriptor[SourceNameEntity]   = _trace_controller->source_at(it->source_index)->descriptor();
    descriptor[ThreadIdEntity]     = _trace_controller->thread_item_at(it->tid_index)->descriptor();
    descriptor[ContextIdEntity]    = _trace_controller->context_item_at(it->context_index)->descriptor();
    descriptor[MessageTypeEntity]  = _trace_controller->message_type_at(it->type)->descriptor();
    descriptor[LabelNameEntity]    = _trace_controller->label_item_at(it->label_index)->descriptor();
}

void TraceModelService::update_filter_pair(trace_index_t::iterator &it, const FilterChain &pair, const QHash<int, ItemDescriptor> &trace_message_desc)
{
    X_CALL;

    // Обновляет флаг фильтрации в индексном контейнере для заданной пары "фильтр-субфильтр" и дескриптора сообщения
    // Нулевой субфильтр означает то, что обновляется только первый фильтр

    const FilterModel *filter = pair.first;
    const FilterModel *subfilter = pair.second;

    bool accepted = true;

    if(_trace_controller->capture_filter()->is_enabled())
    {
        accepted = _trace_controller->capture_filter()->check_filter(trace_message_desc);
    }

    if(accepted && filter && filter->is_enabled())
    {
        accepted = filter->check_filter(trace_message_desc);
    }

    if(accepted && subfilter && subfilter->is_enabled())
    {
        accepted = subfilter->check_filter(trace_message_desc);
    }

    it->filters_map[pair] = accepted;
}

FilterData TraceModelService::make_filter_data_model(const FilterModel *filter, const FilterModel *subfilter, QObject *parent)
{
    TraceDataModel * model = new TraceDataModel(parent);
    TraceDataModel * image_model = new TraceDataModel(parent);

    FilterData data;

    data.model = model;
    data.image_model = image_model;

    data.is_complex_filter = (filter && filter->has_class_in_filter(MessageTextEntity)) || (subfilter && subfilter->has_class_in_filter(MessageTextEntity));

    data.index_item_model = new TraceEntityModel(_trace_controller, x_settings().entity_model_layout, false, model);

    connect(this, &TraceModelService::clear_all_data, model, &TraceDataModel::clear);
    connect(this, &TraceModelService::update_search, model, &TraceDataModel::updated);

    auto update_index_item = [model, data]
    {
        X_CALL_F;

        model->index_mutex()->lock();

        data.index_item_model->set_index_model(data.model->index_model());

        model->index_mutex()->unlock();
    };

    connect(model, &TraceDataModel::model_changed, this, update_index_item, Qt::QueuedConnection);

    return data;
}

void TraceModelService::register_index(trace_index_t::iterator &it)
{
    X_CALL;

    // Register new item in index container

    QHash<int, ItemDescriptor> trace_message_desc;

    fill_descriptor(it, trace_message_desc);

    // Fill all filter flags in new item (for each pair filter-subfilter)

    QMutexLocker locker(&_filter_mutex);

    QMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

    while (map_iter.hasNext())
    {
        map_iter.next();

        update_filter_pair(it, map_iter.key(), trace_message_desc);
    }
}

void TraceModelService::clear()
{
    X_CALL;

    for(int i = 0; i < _trace_filter_model->models().size(); ++i)
    {
        _trace_filter_model->models()[i].reset_indexes();
    }

    for(int i = 0; i < _subtrace_filter_model->models().size(); ++i)
    {
        _subtrace_filter_model->models()[i].reset_indexes();
    }

    _entity_model->clear();

    clear_data();
}

void TraceModelService::clear_data()
{
    X_CALL;

    _main_image_model->clear();

    _issue_model.clear();

    QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

    while (map_iter.hasNext())
    {
        map_iter.next();

        map_iter.value().clear();
    }
}

uint64_t TraceModelService::search(FilterItem search_filter, const QSet<int> &default_filters)
{
    X_CALL;

    // TODO make search as filter model

    uint64_t found_counter = 0;

    _search_filter = search_filter;

    TraceDataModel &trace_data = _trace_controller->trace_model();

    trace_data.lock();

    // parallel for!

    if(search_filter.is_empty())
    {
        for(size_t i = 0; i < trace_data.size(); ++i)
        {
            trace_data.at(i)->flags = 0;
            trace_data.at(i)->search_indexes.clear();
        }
    }
    else
    {
        int class_id = search_filter.class_id();

        QSet<int> filters;

        if(class_id == -1)
        {
            filters = default_filters;
        }
        else
        {
            filters << class_id;
        }

        for(size_t i = 0; i < trace_data.size(); ++i)
        {
            trace_message_t *message = trace_data.at(i);

            message->flags = 0;
            message->search_indexes.clear();

            QVector<QPair<int, int>> indexes;

            foreach(int filter_class, filters)
            {
                if(filter_class == MessageTextEntity)
                {
                    if(search_filter.contains(_trace_controller->message_text_at(message), indexes))
                    {
                        message->flags = SearchHighlighted;
                        message->search_indexes = indexes;

                        found_counter++;

                        break;
                    }
                }
                else
                {
                    if(search_filter.contains(
                                _trace_controller->items_by_class(EntityClass(filter_class), true).at(
                                    _trace_controller->index_by_class(message, filter_class))->descriptor(), indexes))
                    {
                        message->flags = SearchHighlighted;

                        found_counter++;

                        break;
                    }
                }
            }
        }
    }

    trace_data.unlock();

    emit update_search();

    return found_counter;
}

void TraceModelService::lock()
{
    X_CALL;

    _filter_mutex.lock();

    _issue_model.data_model()->lock();

    _main_image_model->lock();

    QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

    while (map_iter.hasNext())
    {
        map_iter.next();

        map_iter.value().lock();
    }
}

void TraceModelService::unlock()
{
    X_CALL;

    _issue_model.data_model()->unlock();

    _main_image_model->unlock();

    QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

    while (map_iter.hasNext())
    {
        map_iter.next();

        map_iter.value().unlock();
    }

    _filter_mutex.unlock();
}

void TraceModelService::remove_from_tail(index_t index)
{
    X_CALL;

    _issue_model.remove_from_tail(index);

    _main_image_model->remove_first(index);

    QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

    while (map_iter.hasNext())
    {
        map_iter.next();

        map_iter.value().remove_from_tail(index);
    }
}

QMutex * TraceModelService::filter_mutex()
{
    return &_filter_mutex;
}

FilterListModel * TraceModelService::trace_filter_models() const
{
    return _trace_filter_model;
}

FilterListModel *TraceModelService::subtrace_filter_models() const
{
    return _subtrace_filter_model;
}

QString TraceModelService::map_source_file(QString path) const
{
    X_CALL;

    QString result_path;

    if(!QFile(path).exists())
    {
        path = QDir::fromNativeSeparators(path);

        foreach (const QString &map_path, _source_map_list)
        {
            QDir map_dir(map_path);

            QString path_section = path;

            do
            {
                path_section = path_section.section('/', 1);

                if(!path_section.isEmpty() && QFile(map_dir.absoluteFilePath(path_section)).exists())
                {
                    result_path = map_dir.absoluteFilePath(path_section);

                    break;
                }
            }
            while (!path_section.isEmpty());

            if(!result_path.isEmpty())
            {
                break;
            }
        }

        X_INFO("{} mapped to {}", path, result_path);
    }
    else
    {
        result_path = path;
    }

    return result_path;
}

void TraceModelService::set_source_map_list(const QStringList &path_list)
{
    X_CALL;

    _source_map_list = path_list;

    X_VALUE(_source_map_list);
}

void TraceModelService::update_all_data()
{
    X_CALL;

    QMutexLocker locker(&_filter_mutex);

    _issue_model.data_model()->emit_updated();

    _main_image_model->emit_updated();

    QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

    while (map_iter.hasNext())
    {
        map_iter.next();

        map_iter.value().emit_updated();
    }
}

void TraceModelService::invalidate_all_filters()
{
    X_CALL;

    trace_index_t &trace_index = _trace_controller->trace_index();

    QHash<int, ItemDescriptor> trace_message_desc;

    //first, update trace_index filter flags

    for(trace_index_t::iterator it = trace_index.begin(); it != trace_index.end(); ++it)
    {
        fill_descriptor(it, trace_message_desc);

        QMutexLocker locker(&_filter_mutex);

        QMutableMapIterator<FilterChain, FilterData> map_iter(_filter_model_map);

        while (map_iter.hasNext())
        {
            map_iter.next();

            const FilterChain &pair = map_iter.key();

            update_filter_pair(it, pair, trace_message_desc);

            map_iter.value().is_up_to_date = false;
            map_iter.value().is_complex_filter = pair.first->has_class_in_filter(MessageTextEntity) || (pair.second && pair.second->has_class_in_filter(MessageTextEntity));
        }
    }

    _filter_updated = true;
    _filter_interrupt_flag = true;
}
