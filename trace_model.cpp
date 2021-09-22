#include "trace_model.h"

#include <QTimer>
#include <QMimeData>
#include <QDir>

#include "filter_model.h"
#include "trace_controller.h"
#include "trace_x/trace_x.h"

namespace
{

QByteArray entity_mime_array(const QModelIndexList &indexes)
{
    X_CALL_F;

    QVariantList entity_list;

    foreach (const QModelIndex &index, indexes)
    {
        if (index.isValid())
        {
            entity_list << index.data(FilterDataRole);
        }
    }

    QByteArray encoded_data;

    QDataStream stream(&encoded_data, QIODevice::WriteOnly);

    stream << entity_list;

    return encoded_data;
}

}

QMimeData* entity_mime_data(const QModelIndexList &indexes)
{
    QMimeData *mime_data = new QMimeData();

    mime_data->setData("application/trace.items.list", entity_mime_array(indexes));

    return mime_data;
}

/* ===================================================================================== */

MessageTypeItem::MessageTypeView::MessageTypeView()
{
    message_type_view.resize(trace_x::_MESSAGE_END_);

    message_type_view[trace_x::MESSAGE_CALL]       = type_view_t("CALL",       Qt::blue);
    message_type_view[trace_x::MESSAGE_RETURN]     = type_view_t("RET",        Qt::darkBlue);
    message_type_view[trace_x::MESSAGE_INFO]       = type_view_t("INFO",       Qt::darkGray);
    message_type_view[trace_x::MESSAGE_DEBUG]      = type_view_t("DEBUG",      Qt::darkGray);
    message_type_view[trace_x::MESSAGE_IMPORTANT]  = type_view_t("IMPORTANT",  Qt::darkGreen);
    message_type_view[trace_x::MESSAGE_EVENT]      = type_view_t("EVENT",      Qt::darkBlue);
    message_type_view[trace_x::MESSAGE_WARNING]    = type_view_t("WARNING",    Qt::darkYellow);
    message_type_view[trace_x::MESSAGE_ERROR]      = type_view_t("ERROR",      Qt::red);
    message_type_view[trace_x::MESSAGE_EXCEPTION]  = type_view_t("EXCEPTION",  Qt::black);
    message_type_view[trace_x::MESSAGE_ASSERT]     = type_view_t("ASSERT",     Qt::darkRed);
    message_type_view[trace_x::MESSAGE_PARAMETERS] = type_view_t("PARAMETERS", Qt::magenta);
    message_type_view[trace_x::MESSAGE_VALUE]      = type_view_t("VALUE",      Qt::magenta);
    message_type_view[trace_x::MESSAGE_SUSPEND]    = type_view_t("SUSPEND",    Qt::darkBlue);
    message_type_view[trace_x::MESSAGE_RESUME]     = type_view_t("RESUME",     Qt::darkBlue);
    message_type_view[trace_x::MESSAGE_SIGNAL]     = type_view_t("SIGNAL",     Qt::darkBlue);
    message_type_view[trace_x::MESSAGE_IMAGE]      = type_view_t("IMAGE",      Qt::darkCyan);

    message_type_view[trace_x::MESSAGE_CONNECTED]    = type_view_t("CONNECTED",    Qt::darkMagenta);
    message_type_view[trace_x::MESSAGE_DISCONNECTED] = type_view_t("DISCONNECTED", Qt::darkMagenta);
    message_type_view[trace_x::MESSAGE_CRASH]        = type_view_t("CRASH",        Qt::darkMagenta);
}

/* ===================================================================================== */

TraceEntityDescription::TraceEntityDescription()
{
    enum_map[ProcessNameEntity]  = QObject::tr("process name");
    enum_map[ProcessIdEntity]    = QObject::tr("process id");
    enum_map[ProcessUserEntity]  = QObject::tr("process user");
    enum_map[ModuleNameEntity]   = QObject::tr("module");
    enum_map[ThreadIdEntity]     = QObject::tr("thread id");
    enum_map[ContextIdEntity]    = QObject::tr("object");
    enum_map[ClassNameEntity]    = QObject::tr("class");
    enum_map[FunctionNameEntity] = QObject::tr("function");
    enum_map[SourceNameEntity]   = QObject::tr("source");
    enum_map[MessageTypeEntity]  = QObject::tr("message type");
    enum_map[MessageTextEntity]  = QObject::tr("message");
    enum_map[LabelNameEntity]    = QObject::tr("variable");
}

TraceEntityDescription &TraceEntityDescription::instance()
{
    static TraceEntityDescription d;

    return d;
}

QString TraceEntityDescription::entity_name(EntityClass type) const
{
    return enum_map[type];
}

/* ===================================================================================== */

MessageTypeItem::MessageTypeView MessageTypeItem::_type_view;

/* ===================================================================================== */

IDItem::IDItem() : EntityItem() {}

IDItem::IDItem(EntityClass class_id, const QString &string, quint64 i1, quint64 i2, quint64 id, qint64 index, const QColor &bg_color, const QColor &fg_color):
    EntityItem(class_id, id, QString("%1_%2] %3").arg(i1).arg(i2).arg(string), index, bg_color, fg_color),
    _id(id),
    _i1(i1),
    _i2(i2)
{
    if(string.isEmpty())
    {
        setText("");
    }

    setData(quintptr(i1), Idx1Role);
    setData(quintptr(i2), Idx2Role);
}

bool IDItem::operator<(const QStandardItem &other) const
{
    if(_i1 == other.data(Idx1Role).toULongLong())
    {
        return _i2 < other.data(Idx2Role).toULongLong();
    }

    return _i1 < other.data(Idx1Role).toULongLong();
}

void IDItem::read(QDataStream &in)
{
    in >> _id >> _i1 >> _i2;

    EntityItem::read(in);
}

void IDItem::write(QDataStream &out) const
{
    out << _id << _i1 << _i2;

    EntityItem::write(out);
}

QVariant IDItem::item_data(int role, int flags) const
{
    if(role == Qt::DisplayRole)
    {
        if(flags & OnlyIndex)
        {
            return QString("%1_%2").arg(_i1).arg(_i2);
        }
    }

    return EntityItem::item_data(role, flags);
}

/* ===================================================================================== */

ContextEntityItem::ContextEntityItem() : IDItem() {}

ContextEntityItem::ContextEntityItem(quint64 context, const QString &class_name, quint64 i1, quint64 i2, qint64 index, const QColor &bg_color, const QColor &fg_color):
    IDItem(ContextIdEntity, "", i1, i2, context, index, bg_color, fg_color),
    _text(QString("%1_%2] 0x%3").arg(i1).arg(i2).arg(context, 0, 16)),
    _class_name(class_name)
{
    setData(_class_name, ContextEntityItem::ClassNameRole);
}

QVariant ContextEntityItem::item_data(int role, int flags) const
{
    if(role == Qt::DisplayRole)
    {
        QString text = _text;

        if(flags & FullText)
        {
            text += " " % _class_name;
        }
        else if(flags & OnlyIndex)
        {
            text = QString("%1_%2").arg(_i1).arg(_i2);
        }

        return text;
    }

    return IDItem::item_data(role, flags);
}

bool ContextEntityItem::operator<(const QStandardItem &other) const
{
    if(_i1 == other.data(Idx1Role).toULongLong())
    {
        return _class_name < other.data(ContextEntityItem::ClassNameRole).toString();
    }

    return _i1 < other.data(Idx1Role).toULongLong();
}

void ContextEntityItem::read(QDataStream &in)
{
    in >> _text >> _class_name;

    IDItem::read(in);
}

void ContextEntityItem::write(QDataStream &out) const
{
    out << _text << _class_name;

    IDItem::write(out);
}

/* ===================================================================================== */

MessageTypeItem::MessageTypeItem():
    EntityItem()
{
}

MessageTypeItem::MessageTypeItem(int type):
    EntityItem(MessageTypeEntity, message_type_name(type), message_type_name(type), type, QColor(), message_type_color(type), message_type_name(type))
{
    setForeground(message_type_color(type));
}

FunctionEntityItem::FunctionEntityItem(): EntityItem() {}

FunctionEntityItem::FunctionEntityItem(const function_t &function, function_index_t fun_index, class_index_t class_index):
    EntityItem(FunctionNameEntity, function.full_name, function.full_name, fun_index, QColor(), QColor(),
               function.signature),
    _class_index(class_index)
{
}

void FunctionEntityItem::read(QDataStream &in)
{
    in >> _class_index;

    EntityItem::read(in);
}

void FunctionEntityItem::write(QDataStream &out) const
{
    out << _class_index;

    EntityItem::write(out);
}

/* ===================================================================================== */

EntityItemModel::EntityItemModel(QObject *parent):
    QAbstractItemModel(parent)
{
}

EntityStandardItemModel::EntityStandardItemModel(QObject *parent):
    QStandardItemModel(parent)
{
}

QMimeData *EntityItemModel::mimeData(const QModelIndexList &indexes) const
{
    X_CALL;

    return entity_mime_data(indexes);
}

QByteArray EntityItemModel::drag_from(const QModelIndexList &indexes) const
{
    X_CALL;

    return entity_mime_array(indexes);
}

QMimeData *EntityStandardItemModel::mimeData(const QModelIndexList &indexes) const
{
    X_CALL;

    return entity_mime_data(indexes);
}

QByteArray EntityStandardItemModel::drag_from(const QModelIndexList &indexes) const
{
    X_CALL;

    return entity_mime_array(indexes);
}

/* ===================================================================================== */

ProxyEntryItem::ProxyEntryItem(const QString &text, const QStandardItem *parent):
    QStandardItem(text),
    _parent(parent)
{
    setData(parent->data(ColorRole), Qt::DecorationRole);
}

bool ProxyEntryItem::operator<(const QStandardItem &other) const
{
    return _parent->operator <(other);
}

QVariant ProxyEntryItem::data(int role) const
{
    if(role == Qt::DisplayRole)
    {
        return QStandardItem::data(role);
    }

    if(role == Qt::DecorationRole)
    {
        return _parent->data(ColorRole);
    }

    return _parent->data(role);
}

/* ===================================================================================== */

TraceEntityModel::TraceEntityModel(QObject *parent):
    EntityStandardItemModel(parent),
    _controller(0)
{
}

TraceEntityModel::TraceEntityModel(TraceController *controller, const QList<EntityClass> &class_list, bool full_trace_model, QObject *parent):
    EntityStandardItemModel(parent),
    _controller(controller),
    _class_list(class_list)
{
    X_CALL;

    if(full_trace_model)
    {
        connect(_controller, &TraceController::model_updated, this, &TraceEntityModel::update_trace_model);

        update_trace_model();
    }
}

QSize TraceEntityModel::span(const QModelIndex &index) const
{
    if(!index.parent().isValid())
    {
        return QSize(1, 1);
    }
    else if(!index.sibling(index.row(), 1).isValid())
    {
        return QSize(1, 1);
    }

    return QSize(0, 0);
}

namespace
{
QString name_for_class(EntityClass class_id)
{
    switch (class_id)
    {
    case ProcessIdEntity:    return "Process";
    case ProcessNameEntity:  return "Process name";
    case ProcessUserEntity:  return "Process user";
    case ModuleNameEntity:   return "Module";
    case ThreadIdEntity:     return "Thread";
    case ContextIdEntity:    return "Object";
    case ClassNameEntity:    return "Classe";
    case FunctionNameEntity: return "Function";
    case SourceNameEntity:   return "Source";
    case MessageTypeEntity:  return "Message type";
    case MessageTextEntity:  return "Message";
    case LabelNameEntity: return "Variable";
    }

    return QString();
}

}

EntityItem * TraceEntityModel::item_for_class(EntityClass class_id, const trace_index_t::iterator &it) const
{
    trace_message_t fake_message;

    fake_message.process_index = it->process_index;
    fake_message.module_index = it->module_index;
    fake_message.tid_index = it->tid_index;
    fake_message.context_index = it->context_index;
    fake_message.function_index = it->function_index;
    fake_message.source_index = it->source_index;
    fake_message.label_index = it->label_index;

    switch (class_id)
    {
    case ProcessIdEntity:    return _controller->process_item_at(&fake_message);
    case ProcessNameEntity:  return _controller->process_name_at(&fake_message);
    case ProcessUserEntity:  return _controller->process_user_at(&fake_message);
    case ModuleNameEntity:   return _controller->module_at(&fake_message);
    case ThreadIdEntity:     return _controller->thread_item_at(&fake_message);
    case ContextIdEntity:    return _controller->context_item_at(&fake_message);
    case ClassNameEntity:    return _controller->class_at(&fake_message);
    case FunctionNameEntity: return _controller->function_at(&fake_message);
    case SourceNameEntity:   return _controller->source_at(&fake_message);
    case MessageTypeEntity:  return _controller->message_type_at(it->type);
    case LabelNameEntity: return _controller->label_item_at(&fake_message);
    default: return 0;
    }

    return 0;
}

void TraceEntityModel::append_class_items(EntityClass class_id, const QList<EntityItem *> &class_items)
{
    X_CALL;

    if((!class_items.empty() && !class_items.first()->text().isEmpty()) || (class_items.count() > 1))
    {
        QStandardItem *root_item = make_root_group(::name_for_class(class_id));

        // align words by columns
        int word_count = class_items.last()->text().split(' ').size(); //last, because first m.b. empty

        QVector<int> word_max_len(word_count);

        foreach (const QStandardItem *item, class_items)
        {
            if(!item->text().isEmpty())
            {
                QStandardItem *new_item = new ProxyEntryItem(item->text(), item);

                if(word_count > 1)
                {
                    QStringList word_list = item->text().split(' ');

                    for(int i = 0; i < word_list.size(); ++i)
                    {
                        if(word_list[i].length() > word_max_len[i])
                        {
                            word_max_len[i] = word_list[i].length();
                        }
                    }
                }

                root_item->appendRow(new_item);
            }
        }

        if(word_count > 1)
        {
            for(int i = 0; i < root_item->rowCount(); ++i)
            {
                QString complete_text;

                QStringList word_list = root_item->child(i)->text().split(' ');

                for(int k = 0; k < word_list.size(); ++k)
                {
                    complete_text += word_list[k] + " ";

                    if(k != word_list.size() - 1)
                    {
                        complete_text += QString(' ').repeated(word_max_len[k] - word_list[k].length());
                    }
                }

                root_item->child(i)->setText(complete_text);
            }
        }

        root_item->sortChildren(0);

        root_item->setToolTip(root_item->text() + QString(" [%1]").arg(root_item->rowCount()));
    }
}

void TraceEntityModel::set_index_model(const trace_index_t &index_model)
{
    X_CALL;

    emit layoutAboutToBeChanged();

    clear();

    setColumnCount(1);

    foreach (EntityClass class_type, _class_list)
    {
        const QList<EntityItem *> &class_items = _controller->items_by_class(class_type);

        // we shows only groups with number of childs more than one - it`s simpler
        if(class_items.size() > 1)
        {
            QSet<EntityItem *> item_set;

            for(trace_index_t::iterator it = index_model.begin(); it != index_model.end(); ++it)
            {
                EntityItem *index_item = item_for_class(class_type, it);

                if(!item_set.contains(index_item))
                {
                    item_set.insert(index_item);
                }
            }

            append_class_items(class_type, item_set.toList());
        }
    }

    emit layoutChanged();
}

void TraceEntityModel::update_trace_model()
{
    X_CALL;

    emit layoutAboutToBeChanged();

    clear();

    setColumnCount(1);

    foreach (EntityClass class_type, _class_list)
    {
        const QList<EntityItem *> &class_items = _controller->items_by_class(class_type);

        // we shows only groups with number of childs more than one - it`s simpler
        if(class_items.size() > 1)
        {
            append_class_items(class_type, class_items);
        }
    }

    emit layoutChanged();
}

QStandardItem *TraceEntityModel::make_root_group(const QString &text)
{
    QStandardItem *item = new QStandardItem(text);

    item->setEditable(false);
    item->setSelectable(false);
    item->setEnabled(false);

    QFont font = item->font();
    font.setBold(true);

    item->setTextAlignment(Qt::AlignCenter);
    item->setFont(font);

    this->invisibleRootItem()->appendRow(item);

    return item;
}

