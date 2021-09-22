#ifndef TRACE_MODEL_SERVICE_H
#define TRACE_MODEL_SERVICE_H

#include <QObject>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QListView>
#include <QtConcurrent/QtConcurrent>

#include <boost/atomic.hpp>

#include "entry_model.h"
#include "common_ui_tools.h"
#include "trace_filter_model.h"
#include "trace_data_model.h"
#include "tx_index.h"
#include "issues_list_model.h"

class TraceController;

struct FilterData
{
    FilterData() : is_up_to_date(true), is_complex_filter(false), model(0), index_item_model(0) {}

    bool is_complex_filter;
    bool is_up_to_date;

    void set_up_to_date()
    {
        is_up_to_date = true;

        model->emit_refiltered();
        image_model->emit_refiltered();
    }

    void emit_updated()
    {
        model->emit_updated();
        image_model->emit_updated();
    }

    void clear()
    {
        model->clear();
        image_model->clear();
    }

    void lock()
    {
        model->lock();
        image_model->lock();
    }

    void unlock()
    {
        model->unlock();
        image_model->unlock();
    }

    void remove_from_tail(index_t index)
    {
        model->remove_first(index);
        image_model->remove_first(index);
    }

    TraceDataModel *model;
    TraceEntityModel *index_item_model;
    TraceDataModel *image_model;
};

typedef QMap<FilterChain, FilterData> FilterTable;

//! Service for filtration, filter managment and search
class TraceModelService : public QObject
{
    Q_OBJECT

public:
    explicit TraceModelService(TraceController *trace_controller, QObject *parent = 0);

    ~TraceModelService();

    //! Search procedure
    uint64_t search(FilterItem search_filter, const QSet<int> &default_filters = QSet<int>());

    void lock();
    void unlock();

    void remove_from_tail(index_t index);
    void clear();
    void clear_data();

    TraceDataModel * request_trace_model(const FilterChain &filter_pair);
    TraceDataModel * request_image_model(const FilterChain &filter_pair);
    TraceEntityModel * request_index_item_model(const FilterChain &filter_pair);

    IssuesListModel * issue_model();
    QMutex * filter_mutex();

    //! Register new index and fill all filter flags on them
    void register_index(trace_index_t::iterator &it);

    void set_source_map_list(const QStringList &path_list);

    void update_all_data();
    void invalidate_all_filters();

public:
    inline QStandardItemModel *item_model() const;

    QByteArray save_state() const;
    void restore_state(const QByteArray &state);

    FilterListModel * trace_filter_models() const;
    FilterListModel * subtrace_filter_models() const;

    QString map_source_file(QString path) const;

signals:
    void clear_all_data();
    void update_search();

private:
    bool filter_message_list(size_t start, size_t end, const trace_index_t &trace_index, bool is_new_messages);

    void update_trace_filter(FilterModel *model);

    void filter_destroyed(QObject *filter);
    void subfilter_destroyed(QObject *filter);

    void register_trace_filter(FilterModel *model);
    void register_subtrace_filter(FilterModel *model);

    void remove_trace_filter(FilterModel *model);
    void remove_subtrace_filter(FilterModel *model);

    void filter_loop();

    void fill_descriptor(const trace_index_t::iterator &it, QHash<int, ItemDescriptor> &descriptor);
    void update_filter_pair(trace_index_t::iterator &it, const FilterChain &pair, const QHash<int, ItemDescriptor> &trace_message_desc);

    FilterData make_filter_data_model(const FilterModel *filter, const FilterModel *subfilter, QObject *parent);

private:
    friend class TraceController;

    TraceController *_trace_controller;

    QThread _filter_thread;

    TraceEntityModel *_entity_model;
    TraceDataModel *_main_image_model;
    FilterListModel *_trace_filter_model;
    FilterListModel *_subtrace_filter_model;

    FilterItem _search_filter;

    IssuesListModel _issue_model;

    boost::atomic<bool> _filter_updated;
    boost::atomic<bool> _filter_interrupt_flag;
    boost::atomic<bool> _full_update;

    //! Table, that bound all filters and message data`s
    FilterTable _filter_model_map;

    QMutex _filter_mutex;

    QStringList _source_map_list;

    size_t _last_index;
};

QStandardItemModel *TraceModelService::item_model() const
{
    return _entity_model;
}

#endif // TRACE_MODEL_SERVICE_H
