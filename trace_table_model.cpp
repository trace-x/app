#include "trace_table_model.h"
#include "trace_model.h"
#include "trace_controller.h"
#include "settings.h"

#include "trace_x/trace_x.h"

#include <QFont>
#include <QFontMetrics>
#include <QStringBuilder>

TraceTableModel::TraceTableModel(TraceDataModel *trace_model, TraceController &trace_controller,
                                 const QList<TraceColumns> &columns, QObject *parent):
    QAbstractTableModel(parent),
    _trace_controller(trace_controller),
    _trace_model(0)
{
    X_CALL;

    _number_index       = -1;
    _timestamp_index    = -1;
    _process_index      = -1;
    _module_index       = -1;
    _thread_index       = -1;
    _context_index      = -1;
    _message_type_index = -1;
    _call_level_index   = -1;
    _message_index      = -1;
    _function_index     = -1;

    _header_model.resize(columns.size());

    int next_index = 0;

    foreach (TraceColumns column, columns)
    {
        switch(column)
        {
        case Number:
            _number_index = next_index;
            _header_model[next_index] = ColumnData(tr("   #"), tr("Message number"), "999999");
            break;
        case Timestamp:
            _timestamp_index = next_index;
            _header_model[next_index] = ColumnData(tr("Time"), tr("Time stamp"), "99.999999_");
            break;
        case Process:
            _process_index = next_index;
            _header_model[next_index] = ColumnData(tr("P"), tr("Process"), "99_");
            break;
        case Module:
            _module_index = next_index;
            _header_model[next_index] = ColumnData(tr("Module"), tr("Module"), "module_name__");
            break;
        case Thread:
            _thread_index = next_index;
            _header_model[next_index] = ColumnData(tr("T"), tr("Thread index"), "9_9_");
            break;
        case Level:
            _call_level_index = next_index;
            _header_model[next_index] = ColumnData(tr("L"), tr("Call level"), "99_");
            break;
        case Type:
            _message_type_index = next_index;
            _header_model[next_index] = ColumnData(tr("Type"), tr("Message type"), "IMPORTANT_");
            break;
        case Context:
            _context_index = next_index;
            _header_model[next_index] = ColumnData(tr("C"), tr("Object index"), "9_99_");
            break;
        case Message:
            _message_index = next_index;
            _header_model[next_index] = ColumnData(tr("Message"), tr("Message"), "");
            break;
        case Function:
            _function_index = next_index;
            _header_model[next_index] = ColumnData(tr("Function"), tr("Function"), "namespace::Class::function");
            break;
        }

        next_index++;
    }

    set_data(trace_model);
}

void TraceTableModel::set_data(TraceDataModel *data_model)
{
    X_CALL;

    X_INFO("current model: {}", (void*)_trace_model);
    X_INFO("new model: {}", (void*)data_model);

    if(_trace_model == data_model)
    {
        return;
    }

    if(_trace_model)
    {
        _trace_model->disconnect(this);
    }

    _trace_model = data_model;

    if(data_model)
    {
        connect(_trace_model, &TraceDataModel::destroyed, [this] { _trace_model = 0; });
        connect(_trace_model, &TraceDataModel::updated, this, &TraceTableModel::model_updated);
        connect(_trace_model, &TraceDataModel::cleaned, this, &TraceTableModel::cleaned);
        connect(_trace_model, &TraceDataModel::refiltered, this, &TraceTableModel::refiltered);
    }

    model_updated();

    emit cleaned();
}

int TraceTableModel::rowCount(const QModelIndex &parent) const
{
    if(!_trace_model)
        return 0;

    return parent.isValid() ? 0 : int(_trace_model->safe_size());
}

int TraceTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _header_model.size();
}

QString TraceTableModel::column_size_hint(int column) const
{
    return _header_model.at(column).width;
}

QVariant TraceTableModel::data(const QModelIndex &index, int role) const
{
    //X_CALL;

    //X_INFO("data index: row={};col={}", index.row(), index.column());

    QVariant result;

    bool process_role = false;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::BackgroundRole:
    case Qt::DecorationRole:
    case Qt::ForegroundRole:
    case FilterDataRole:
    case SearchHighlightDataRole:
    case SearchIndexesDataRole:
    case FunctionLevelDataRole:
        process_role = true;
        break;
    default:
        process_role = false;
    }

    if(!_trace_model || !process_role)
        return result;

    _trace_model->mutex()->lock();

    if(index.row() < int(_trace_model->size()))
    {
        trace_message_t message_k = *_trace_model->at(index.row());

        const trace_message_t *message = &message_k;

        _trace_model->mutex()->unlock();

        //TODO можно сделать проще - перенести эту часть на EnityItem

        // message->module_data(role, flags);

        if(index.column() == _number_index) return data_number(message, index, role);
        else if(index.column() == _timestamp_index) return data_timestamp(message, index, role);
        else if(index.column() == _process_index) return data_process(message, index, role);
        else if(index.column() == _module_index) return data_module(message, index, role);
        else if(index.column() == _thread_index) return data_thread(message, index, role);
        else if(index.column() == _call_level_index) return data_call_level(message, index, role);
        else if(index.column() == _message_type_index) return data_message_type(message, index, role);
        else if(index.column() == _context_index) return data_context(message, index, role);
        else if(index.column() == _message_index) return data_message(message, index, role);
        else return data_function(message, index, role);
    }

    _trace_model->mutex()->unlock();

    return result;
}

QVariant TraceTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::TextAlignmentRole)
    {
        return Qt::AlignLeft;
    }

    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            if(_trace_model && (section == this->columnCount() - 1) && _trace_model->size())
            {
                return QString(tr("%1 %2 displayed: %3")).arg(_header_model.at(section).text).arg(QChar(UnicodeBullet)).arg(_trace_model->size());
            }

            return _header_model.at(section).text;
        }
        else if(role == Qt::ToolTipRole)
        {
            return _header_model.at(section).tool_tip;
        }
    }

    return QVariant();
}

Qt::ItemFlags TraceTableModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsDropEnabled;
}

QMimeData *TraceTableModel::mimeData(const QModelIndexList &indexes) const
{
    return entity_mime_data(indexes);
}

QStringList TraceTableModel::mimeTypes() const
{
    return QStringList() << "application/trace.items.list";
}

void TraceTableModel::model_updated()
{
    X_CALL;

    //  emit dataChanged(index(_size + 1, 0), index(_trace_model.size(), 0));
    //    beginInsertRows(QModelIndex(), _size + 1, _trace_model.size());

    //    _size = _trace_model.size();

    //    endInsertRows();

    emit layoutChanged();
    // emit rowsInserted(QModelIndex(), _trace_model.size(), _trace_model.size());
    //emit dataChanged(index(_trace_model.size()));
}

QVariant TraceTableModel::data_number(const trace_message_t *, const QModelIndex &index, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        result = index.row() + 1;
        break;
    }

    return result;
}

QVariant TraceTableModel::data_timestamp(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        result = QString::number((message->timestamp + _trace_controller.process_at(message).time_delta()) / 1000000000.0, 'f', 6);
        break;
    }

    return result;
}

QVariant TraceTableModel::data_process(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole:
        result = message->process_index + 1;
        break;
    case Qt::ToolTipRole:
        result = QString("#%1: %2 [%3]").arg(message->process_index + 1).arg(_trace_controller.process_at(message).pid()).arg(_trace_controller.process_at(message).pid());
        break;
    case Qt::BackgroundRole:
        result = _trace_controller.process_at(message).data(ColorRole);
        break;
    case Qt::ForegroundRole:
        result = _trace_controller.process_at(message).data(TextColorRole);
        break;
    case FilterDataRole:
        result = _trace_controller.process_at(message).data(FilterDataRole);
        break;
    }

    return result;
}

QVariant TraceTableModel::data_module(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    if(message->type >= trace_x::_MESSAGE_END_) return result;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        result = _trace_controller.module_at(message)->text();
        break;
    case Qt::DecorationRole:
        result = _trace_controller.module_at(message)->data(ColorRole);
        break;
    case FilterDataRole:
        result = _trace_controller.module_at(message)->data(FilterDataRole);
        break;
    }

    return result;
}

QVariant TraceTableModel::data_thread(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    if(message->type >= trace_x::_MESSAGE_END_) return result;

    switch(role)
    {
    case Qt::DisplayRole:
        result = QString("%1_%2").arg(message->process_index + 1).arg(_trace_controller.thread_item_at(message)->data(IDItem::Idx2Role).toInt());
        break;
    case Qt::ToolTipRole:
        result = _trace_controller.thread_item_at(message)->text();
        break;
    case Qt::BackgroundRole:
        result = _trace_controller.thread_item_at(message)->data(ColorRole);
        break;
    case Qt::ForegroundRole:
        result = _trace_controller.thread_item_at(message)->data(TextColorRole);
        break;
    case FilterDataRole:
        result = _trace_controller.thread_item_at(message)->data(FilterDataRole);
        break;
    }

    return result;
}

QVariant TraceTableModel::data_context(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    if(message->type >= trace_x::_MESSAGE_END_) return result;

    switch(role)
    {
    case Qt::DisplayRole:
        result = QString("%1_%2").arg(message->process_index + 1).arg(_trace_controller.context_item_at(message)->data(IDItem::Idx2Role).toInt());
        break;
    case Qt::ToolTipRole:
        result = _trace_controller.context_item_at(message)->text();
        break;
    case Qt::BackgroundRole:
        result = _trace_controller.context_item_at(message)->data(ColorRole);
        break;
    case Qt::ForegroundRole:
        result = _trace_controller.context_item_at(message)->data(TextColorRole);
        break;
    case FilterDataRole:
        result = _trace_controller.context_item_at(message)->data(FilterDataRole);
        break;
    }

    return result;
}

QVariant TraceTableModel::data_message_type(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        result = MessageTypeItem::message_type_name(message->type);
        break;
    case Qt::ForegroundRole:
        result = MessageTypeItem::message_type_color(message->type);
        break;
    case FilterDataRole:
        result = _trace_controller.message_type_at(message->type)->data(FilterDataRole);
        break;
    }

    return result;
}

QVariant TraceTableModel::data_call_level(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    if(message->type >= trace_x::_MESSAGE_END_) return result;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        result = message->call_level;
        break;
    }

    return result;
}

QVariant TraceTableModel::data_message(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        QString message_string = _trace_controller.message_text_at(message);

        if(role == Qt::DisplayRole)
        {
            message_string.replace('\n', ' ');
        }

        if(role == Qt::DisplayRole)
        {
            result = QString("%1 ").arg(QChar(UnicodeBullet)).repeated(message->call_level) + message_string;
        }
        else
        {
            result = message_string;
        }

        break;
    }
    case Qt::ForegroundRole:
    {
        result = MessageTypeItem::message_type_color(message->type);
        break;
    }
    case FilterDataRole:
    {
        QString message_text = _trace_controller.message_text_at(message);

        result = QVariant::fromValue(FilterItem(message_text, message_text, MessageTextEntity,
                                                FilterItem::ID, -1, QVariant(), QVariant()));

//        if(message->type > trace_x::MESSAGE_RETURN)
//        {
////            if((message->type == trace_x::MESSAGE_VALUE) || (message->type == trace_x::MESSAGE_IMAGE) || (message->type == trace_x::MESSAGE_DATA))
////            {
////                result = _trace_controller.label_item_at(message)->data(FilterDataRole);
////            }
////            else
//            {
//                result = QVariant::fromValue(FilterItem(message->message_text, message->message_text, MessageTextEntity,
//                                                        FilterItem::ID, -1, QVariant(), QVariant()));
//            }
//        }
//        else
//        {
//            QString message_text = _trace_controller.function_at(message)->text();

//            result = QVariant::fromValue(FilterItem(message_text, message_text, MessageTextEntity,
//                                                    FilterItem::ID, -1, QVariant(), QVariant()));

//           // result = _trace_controller.function_at(message)->data(FilterDataRole);
//        }

        break;
    }
    case SearchHighlightDataRole:
    {
        result = (message->flags == SearchHighlighted) ? x_settings().search_highlight_color : QVariant();

        break;
    }
    case SearchIndexesDataRole:
    {
        result = (message->flags == SearchHighlighted) ? QVariant::fromValue(message->search_indexes) : QVariant();

        break;
    }
    case FunctionLevelDataRole:
    {
        result = message->call_level;

        break;
    }
    }

    return result;
}

QVariant TraceTableModel::data_function(const trace_message_t *message, const QModelIndex &, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole: return _trace_controller.function_at(message)->text();
    case Qt::ToolTipRole: return _trace_controller.function_at(message)->toolTip();
    }

    return result;
}
