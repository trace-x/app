#include "profile_model.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include <QHeaderView>
#include <QMenu>
#include <QScrollBar>

#include "trace_controller.h"

#include "trace_x/trace_x.h"

using boost::multi_index_container;
using namespace boost::multi_index;

namespace
{
enum
{
    SortRole = Qt::UserRole + 1700
};

inline QString nanoseconds_to_string(uint64_t time)
{
    return time ? QString::number(time / 1000000000.0, 'f', 6) : "-";
}

}

struct function_stat
{
    function_stat():
        total_time(0),
        avg_time(0),
        max_time(0),
        min_time(0),
        process_rate(0),
        thread_rate(0)
    {
    }

    void update_stat(uint64_t duration, size_t call_counter)
    {
        if((duration < min_time) || !min_time)
        {
            min_time = duration;
        }

        if(duration > max_time)
        {
            max_time = duration;
        }

        total_time += duration;

        avg_time = total_time / call_counter;
    }

    void update_global_stat(uint64_t process_time, uint64_t thread_time)
    {
        if(process_time)
        {
            process_rate = total_time / double(process_time);
        }

        if(thread_time)
        {
            thread_rate = total_time / double(thread_time);
        }
    }

    uint64_t total_time;
    uint64_t avg_time;
    uint64_t min_time;
    uint64_t max_time;

    double process_rate;
    double thread_rate;
};

struct function_profile_t
{
    function_profile_t() {}

    function_profile_t(function_index_t function, pid_index_t process, tid_index_t thread,
                       module_index_t module, source_index_t source, label_index_t label, uint64_t time):
        function_index(function), process_index(process), thread_index(thread),
        module_index(module), source_index(source), label_index(label), last_start_time(time),
        call_counter(1), level(0)
    {}

    int64_t          function_index;
    pid_index_t      process_index;
    int32_t          thread_index;

    module_index_t   module_index;
    source_index_t   source_index;
    label_index_t    label_index;

    mutable uint64_t last_start_time;

    mutable size_t call_counter;

    mutable function_stat exclusive_stat;
    mutable function_stat inclusive_stat;

    mutable int level;

    struct ByKey {};
};

//! Multi-index container for trace model storing
typedef boost::multi_index_container<function_profile_t,
indexed_by<
random_access<>,
hashed_unique<
tag<function_profile_t::ByKey>, composite_key<
function_profile_t,
member<function_profile_t, int64_t, &function_profile_t::function_index>,
member<function_profile_t, pid_index_t, &function_profile_t::process_index>,
member<function_profile_t, int32_t, &function_profile_t::thread_index>
>
>
>
> profile_index_t;

struct level_profile_t
{
    level_profile_t(pid_index_t process, int32_t thread, int32_t level):
        process_index(process),
        thread_index(thread),
        level_index(level)
    {}

    pid_index_t process_index;
    int32_t     thread_index;
    int32_t     level_index;

    mutable uint64_t level_total_time;

    struct ByKey {};
};

//! Multi-index container for trace model storing
typedef boost::multi_index_container<level_profile_t,
indexed_by<
hashed_unique<
tag<level_profile_t::ByKey>, composite_key<
level_profile_t,
member<level_profile_t, pid_index_t, &level_profile_t::process_index>,
member<level_profile_t, int32_t, &level_profile_t::thread_index>,
member<level_profile_t, int32_t, &level_profile_t::level_index>
>
>
>
> level_index_t;

struct ProcessStat
{
    ProcessStat() : total_duration(0) {}

    uint64_t total_duration;

    QHash<pid_index_t, uint64_t> threads_duration;
};

struct ProfileModelPrivate
{
    profile_index_t data;

    QHash<pid_index_t, ProcessStat> process_stats;
};

ProfileModel::ProfileModel(TraceController *controller, TraceDataModel *trace_data, QObject *parent):
    QAbstractTableModel(parent),
    _controller(controller),
    _p(new ProfileModelPrivate),
    _exclusive_mode(true)
{
    X_CALL;

    _header_model.resize(LastColumn);

    _header_model[Process]     = ColumnData(tr("P"), tr("Process"), "99_");
    _header_model[Module]      = ColumnData(tr("Module"), tr("Module"), "module_name__");
    _header_model[Thread]      = ColumnData(tr("T"), tr("Thread index"), "9_9___");
    _header_model[Function]    = ColumnData(tr("Function"), tr("Function"), "namespace::Class::function_____________");
    _header_model[Calls]       = ColumnData(tr("Calls"), tr("Number of calls"), "999999_");
    _header_model[TotalTime]   = ColumnData(tr("Total"), tr("Total time, sec."), "00.00000000_");
    _header_model[MinTime]     = ColumnData(tr("Min"), tr("Minimum time, sec."), "00.00000000_");
    _header_model[MaxTime]     = ColumnData(tr("Max"), tr("Maximum time, sec."), "00.00000000_");
    _header_model[AvgTime]     = ColumnData(tr("Avg"), tr("Average time, sec."), "00.00000000_");
    _header_model[ProcessRate] = ColumnData(tr("Proc. %"), tr("Process rate, %"), "00.000000000");
    _header_model[ThreadRate]  = ColumnData(tr("Thread, %"), tr("Thread rate, %"), "00.000000000");

    QMutexLocker(trace_data->mutex());

    level_index_t level_index;

    /// Build profile

    for(size_t i = 0; i < trace_data->size(); ++i)
    {
        const trace_message_t *message = trace_data->at(i);

        if((message->type == trace_x::MESSAGE_CALL) || (message->type == trace_x::MESSAGE_RETURN))
        {
            profile_index_t::index<function_profile_t::ByKey>::type::iterator function =
                    _p->data.get<function_profile_t::ByKey>().find
                    (boost::make_tuple(message->function_index, message->process_index, message->tid_index));

            if(message->type == trace_x::MESSAGE_CALL)
            {
                level_index_t::index<level_profile_t::ByKey>::type::iterator next_level =
                        level_index.get<level_profile_t::ByKey>().insert
                        (level_profile_t(message->process_index, message->tid_index, message->call_level + 1)).first;

                next_level->level_total_time = 0;
            }

            if((function == _p->data.get<function_profile_t::ByKey>().end()) && (message->type == trace_x::MESSAGE_CALL))
            {
                // new function

                _p->data.push_back(function_profile_t(message->function_index, message->process_index, message->tid_index,
                                                      message->module_index, message->source_index, message->label_index,
                                                      message->timestamp));
            }
            else
            {
                if(message->type == trace_x::MESSAGE_RETURN)
                {
                    if(function->last_start_time)
                    {
                        if(!function->level) //recursion protection
                        {
                            uint64_t duration = message->timestamp - message->extra_timestamp;
                            //uint64_t duration = message->timestamp - function->last_start_time;

                            level_index_t::index<level_profile_t::ByKey>::type::iterator current_level =
                                    level_index.get<level_profile_t::ByKey>().insert
                                    (level_profile_t(message->process_index, message->tid_index, message->call_level)).first;

                            current_level->level_total_time += duration;

                            level_index_t::index<level_profile_t::ByKey>::type::iterator next_level =
                                    level_index.get<level_profile_t::ByKey>().find
                                    (boost::make_tuple(message->process_index, message->tid_index, message->call_level + 1));

                            uint64_t level_duration = 0;

                            if(next_level != level_index.get<level_profile_t::ByKey>().end())
                            {
                                level_duration = next_level->level_total_time;

                                next_level->level_total_time = 0;
                            }


                            uint64_t exlusive_duration = duration - level_duration;

                            //

                            uint64_t &process_duration = _p->process_stats[message->process_index].total_duration;
                            uint64_t &thread_duration = _p->process_stats[message->process_index].threads_duration[message->tid_index];

                            process_duration += exlusive_duration;
                            thread_duration += exlusive_duration;

                            //

                            function->inclusive_stat.update_stat(duration, function->call_counter);
                            function->exclusive_stat.update_stat(exlusive_duration, function->call_counter);
                        }
                        else
                        {
                            function->level--;
                        }
                    }

                    function->last_start_time = 0;
                }
                else if(message->type == trace_x::MESSAGE_CALL)
                {
                    function->call_counter++;

                    if(!function->last_start_time)
                    {
                        function->last_start_time = message->timestamp;
                    }
                    else
                    {
                        function->level++;
                    }
                }
            }
        }
    }

    for(auto it = _p->data.begin(); it != _p->data.end(); ++it)
    {
        uint64_t process_duration = _p->process_stats[it->process_index].total_duration;
        uint64_t thread_duration = _p->process_stats[it->process_index].threads_duration[it->thread_index];

        it->exclusive_stat.update_global_stat(process_duration, thread_duration);
        it->inclusive_stat.update_global_stat(process_duration, thread_duration);
    }
}

ProfileModel::~ProfileModel()
{
    X_CALL;

    delete _p;
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(_p->data.size());
}

int ProfileModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _header_model.size();
}

QVariant ProfileModel::data(const QModelIndex &index, int role_) const
{
    QVariant result;

    int role = role_;

    bool process_role = false;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::BackgroundRole:
    case Qt::DecorationRole:
    case Qt::ForegroundRole:
    case FilterDataRole:
    case ::SortRole:
        process_role = true;
        break;
    default:
        process_role = false;
    }

    if(process_role)
    {
        if(role_ == ::SortRole)
        {
            role = Qt::DisplayRole;
        }

        switch (index.column())
        {
        case Process:  return _controller->process_item_at(_p->data[index.row()].process_index)->item_data(role, EntityItem::OnlyIndex | EntityItem::WithBackColor);
        case Thread:   return _controller->thread_item_at(_p->data[index.row()].thread_index)->item_data(role, EntityItem::OnlyIndex | EntityItem::WithBackColor);
        case Module:   return _controller->module_at(_p->data[index.row()].module_index)->item_data(role, EntityItem::WithDecorator);
        case Function: return _controller->function_at(_p->data[index.row()].function_index)->data(role);
        }

        if((role == Qt::DisplayRole) || (role == Qt::ToolTipRole))
        {
            const function_stat *stats = &_p->data[index.row()].exclusive_stat;

            if(!_exclusive_mode)
            {
                stats = &_p->data[index.row()].inclusive_stat;
            }

            if(index.column() == Calls)
            {
                return quint64(_p->data[index.row()].call_counter);
            }
            else
            {
                uint64_t time_value;

                bool rate = false;

                switch (index.column())
                {
                case TotalTime: time_value = stats->total_time; break;
                case MinTime: time_value = stats->min_time; break;
                case MaxTime: time_value = stats->max_time; break;
                case AvgTime: time_value = stats->avg_time; break;
                default: rate = true;
                }

                if(!rate)
                {
                    if(role_ == Qt::DisplayRole)
                    {
                        return ::nanoseconds_to_string(time_value);
                    }
                    else if(role_ == ::SortRole)
                    {
                        return quint64(time_value);
                    }
                    else
                    {
                        return QString::number(time_value) + " ns";
                    }
                }
                else
                {
                    double rate;

                    switch (index.column())
                    {
                    case ProcessRate: rate = stats->process_rate; break;
                    case ThreadRate: rate = stats->thread_rate; break;
                    }

                    if(role_ == ::SortRole)
                    {
                        return rate;
                    }
                    else
                    {
                        return rate ? QString::number(rate * 100.0, 'f', 3) : "-";
                    }
                }
            }
        }
    }

    return result;
}

QVariant ProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::TextAlignmentRole)
    {
        return Qt::AlignLeft;
    }

    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return _header_model.at(section).text;
        }
        else if(role == Qt::ToolTipRole)
        {
            return _header_model.at(section).tool_tip;
        }
    }

    return QVariant();
}

QString ProfileModel::column_size_hint(int column) const
{
    return _header_model.at(column).width;
}

void ProfileModel::clear()
{
    X_CALL;

    emit layoutAboutToBeChanged();

    _p->data.clear();

    emit layoutChanged();
}

void ProfileModel::set_inclusive_mode(bool is_inclusive)
{
    X_CALL;

    emit layoutAboutToBeChanged();

    _exclusive_mode = !is_inclusive;

    emit layoutChanged();
}

//////////////////////

ProfilerTable::ProfilerTable(TraceController *controller, TraceDataModel *trace_data, QWidget *parent):
    TableView(parent)
{
    X_CALL;

    setWindowTitle(tr("Profiler"));

    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setStretchLastSection(true);
    this->horizontalHeader()->setSectionsMovable(true);

    ProfileModel *profile_model = new ProfileModel(controller, trace_data, this);

    _model = profile_model;

    this->setItemDelegateForColumn(ProfileModel::Module, new FancyItemDelegate(this));

    QSortFilterProxyModel *sort_model = new QSortFilterProxyModel(this);

    sort_model->setSortRole(::SortRole);
    sort_model->setSourceModel(profile_model);

    this->setSortingEnabled(true);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);

    this->setModel(sort_model);

    //

    QFontMetrics font_metrics(this->font());

    for(int i = 0; i < profile_model->columnCount(); ++i)
    {
        this->setColumnWidth(i, font_metrics.width(profile_model->column_size_hint(i)));
    }

    int font_h = QFontMetrics(this->font()).height() + 5;

    this->verticalHeader()->setDefaultSectionSize(font_h);

    //

    setColumnHidden(ProfileModel::Process, controller->items_by_class(ProcessIdEntity).size() <= 1);
    setColumnHidden(ProfileModel::Thread, controller->items_by_class(ThreadIdEntity).size() <= 2);
    setColumnHidden(ProfileModel::Module, controller->items_by_class(ModuleNameEntity).size() <= 1);

    //

    _menu_button = new QToolButton(this);

    _menu_button->setIcon(QIcon(":/icons/dots_dark"));
    _menu_button->setPopupMode(QToolButton::InstantPopup);

    QMenu *table_menu = new QMenu();

    _menu_button->setMenu(table_menu);

    QActionGroup *action_group = new QActionGroup(this);
    action_group->setExclusive(true);

    QAction *exclusive_action = new QAction(tr("Exclusive"), action_group);
    QAction *inclusive_action = new QAction(tr("Inclusive"), action_group);

    connect(inclusive_action, &QAction::toggled, _model, &ProfileModel::set_inclusive_mode);

    exclusive_action->setCheckable(true);
    inclusive_action->setCheckable(true);

    exclusive_action->setChecked(true);

    table_menu->addAction(exclusive_action);
    table_menu->addAction(inclusive_action);

    //

    this->sortByColumn(ProfileModel::TotalTime);
}

ProfilerTable::~ProfilerTable()
{
    X_CALL;
}

ProfileModel *ProfilerTable::model()
{
    return _model;
}

void ProfilerTable::resizeEvent(QResizeEvent *event)
{
    TableView::resizeEvent(event);

    update_layout();
}

void ProfilerTable::showEvent(QShowEvent *event)
{
    update_layout();

    TableView::showEvent(event);
}

void ProfilerTable::update_layout()
{
    QRect geometry(0, 0, _menu_button->sizeHint().width(), horizontalHeader()->height());

    if(!verticalScrollBar()->isVisible())
    {
        geometry.moveTopRight(QPoint(this->width() - 2, 0));
    }
    else
    {
        geometry.moveTopRight(QPoint(this->width() - this->verticalScrollBar()->width() - 2, 0));
    }

    _menu_button->setGeometry(geometry);
}
