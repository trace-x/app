#include "extra_message_model.h"

#include <QFileInfo>
#include <QDateTime>

#include "process_model.h"
#include "trace_controller.h"

#include "trace_x/trace_x.h"

class ModelItem : public QStandardItem
{
public:
    void set_model(ExtraMessageModel *model)
    {
        _model = model;
    }

    const trace_message_t *message() const
    {
        return _model->_ref_message;
    }

    const TraceController &trace_controller() const
    {
        return _model->_trace_controller;
    }

protected:
    ExtraMessageModel *_model;
};

namespace _detail
{

class TitleItem : public QStandardItem
{
public:
    TitleItem(const QString &text):
        QStandardItem(text)
    {
    }

    QVariant data(int role) const
    {
        if(role == FilterDataRole)
        {
            return model()->index(this->row(), 1, this->index().parent()).data(role);
        }

        return QStandardItem::data(role);
    }
};

class TimestampItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message()) return QVariant();

        if(role == Qt::DisplayRole) return QDateTime::fromMSecsSinceEpoch(trace_controller().start_point() + (message()->timestamp + trace_controller().process_at(message()).time_delta()) / 1000000).toString("hh:mm:ss.zzz");

        return QVariant();
    }
};

class ProcessNameItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message()) return QVariant();

        return trace_controller().process_name_at(message())->data(role);
    }
};

class ProcessIDItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message()) return QVariant();

        return trace_controller().process_at(message()).item_data(role, EntityItem::WithDecorator | EntityItem::ShortText);
    }
};

class ModuleItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        return trace_controller().module_at(message())->item_data(role, EntityItem::WithDecorator | EntityItem::ShortText);
    }
};

class Contexttem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        return trace_controller().context_item_at(message())->item_data(role, EntityItem::WithDecorator | EntityItem::ShortText);
    }
};

class ClassItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        return trace_controller().class_at(message())->data(role);
    }
};

class FunctionItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        return trace_controller().function_at(message())->data(role);
    }
};

class SourceFileItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        return trace_controller().source_at(message())->data(role);
    }
};

class SourceLineItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        if(role == Qt::DisplayRole)
        {
            return message()->source_line;
        }

        return QVariant();
    }
};

class ThreadItem : public ModelItem
{
    QVariant data(int role) const
    {
        if(!message() || (message()->type >= trace_x::_MESSAGE_END_)) return QVariant();

        return trace_controller().thread_item_at(message())->item_data(role, EntityItem::WithDecorator | EntityItem::ShortText);
    }
};

}

ExtraMessageModel::ExtraMessageModel(const TraceController &trace_controller, QObject *parent) :
    QStandardItemModel(parent),
    _ref_message(0),
    _trace_controller(trace_controller)
{
    X_CALL;

    setColumnCount(2);

    append_row(tr("Timestamp"), new _detail::TimestampItem);
    append_row(tr("Process name"), new _detail::ProcessNameItem);
    append_row(tr("Process ID"), new _detail::ProcessIDItem);
    append_row(tr("Module"), new _detail::ModuleItem);
    append_row(tr("Thread"), new _detail::ThreadItem);
    append_row(tr("Object"), new _detail::Contexttem);
    append_row(tr("Class"), new _detail::ClassItem);
    append_row(tr("Function"), new _detail::FunctionItem);
    append_row(tr("Source"), new _detail::SourceFileItem);
    append_row(tr("Source line"), new _detail::SourceLineItem);
}

void ExtraMessageModel::set_message(const trace_message_t *message)
{
    X_CALL;

    X_INFO("new message: {}", message ? message->index : 0);

    _ref_message = message;

    if(_ref_message)
    {
        _message = *message;
        _ref_message = &_message;
    }

    emit dataChanged(index(0, 1), index(rowCount() - 1, 1));
}

void ExtraMessageModel::clear()
{
    _ref_message = 0;
}

void ExtraMessageModel::append_row(const QString &name, ModelItem *item)
{
    item->set_model(this);

    appendRow(QList<QStandardItem*>() << new _detail::TitleItem(name) << item);
}

QMimeData *ExtraMessageModel::mimeData(const QModelIndexList &indexes) const
{
    X_CALL;

    return entity_mime_data(indexes);
}
