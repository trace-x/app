#ifndef TRACE_MODEL_H
#define TRACE_MODEL_H

#include <stdint.h>

#include <QObject>
#include <QString>
#include <QMutex>
#include <QStandardItem>
#include <QFileInfo>

#include "entry_model.h"

#include "trace_x/impl/types.h"
#include "trace_x/detail/trace.h"
#include "tx_index.h"

class ProcessModel;
class TraceController;

enum MessageFlags
{
    SearchHighlighted = 0x1,
};

struct trace_message_t
{
    index_t  index;
    uint8_t  type;
    uint64_t timestamp; // nanosecond
    uint64_t extra_timestamp; // nanosecond

    pid_index_t      process_index;
    tid_index_t      tid_index;
    context_index_t  context_index;
    module_index_t   module_index;
    function_index_t function_index;
    label_index_t    label_index;
    source_index_t   source_index;

    uint32_t source_line;
    int16_t  call_level;
    QString  message_text;

    uint8_t flags;
    QVector<QPair<int, int>> search_indexes;

    bool in_same_thread(const trace_message_t *other) const { return (process_index == other->process_index) && (tid_index == other->tid_index); }
};

enum EntityClass
{
    ProcessNameEntity,
    ProcessIdEntity,
    ProcessUserEntity,
    ModuleNameEntity,
    ThreadIdEntity,
    ContextIdEntity,
    ClassNameEntity,
    FunctionNameEntity,
    SourceNameEntity,
    MessageTypeEntity,
    LabelNameEntity,
    MessageTextEntity
};

class TraceEntityDescription
{
public:
    TraceEntityDescription();

    static TraceEntityDescription & instance();

    QString entity_name(EntityClass type) const;

private:
    QMap<EntityClass, QString> enum_map;
};

/////////////////////////////////////////////////////////////////////////////////////

class MessageTypeItem : public EntityItem
{
public:
    struct MessageTypeView
    {
        struct type_view_t
        {
            type_view_t() {}
            type_view_t(const QString &name_, const QColor &color_):
                name(name_),
                color(color_) {}

            QString name;
            QColor color;
        };

        MessageTypeView();

        QVector<type_view_t> message_type_view;
    };

    static QString message_type_name(int type)
    {
        return _type_view.message_type_view[type].name;
    }

    static QColor message_type_color(int type)
    {
        return _type_view.message_type_view[type].color;
    }

    MessageTypeItem();
    MessageTypeItem(int type);

private:
    static MessageTypeView _type_view;
};

//! Model item with sort operator "i1_i2"
class IDItem : public EntityItem
{
public:
    static const int Idx1Role = Qt::UserRole + 1;
    static const int Idx2Role = Qt::UserRole + 2;

public:
    IDItem();
    IDItem(EntityClass class_id, const QString &string, quint64 i1, quint64 i2,
           quint64 id, qint64 index, const QColor &bg_color, const QColor &fg_color);

    virtual bool operator<(const QStandardItem &other) const;
    virtual void read(QDataStream &in);
    virtual void write(QDataStream &out) const;

    QVariant item_data(int role, int flags) const;

    quint64 _id;

    quint64 _i1;
    quint64 _i2;
};

class ContextEntityItem : public IDItem
{
    static const int ClassNameRole = Qt::UserRole + 3;

public:
    ContextEntityItem();
    ContextEntityItem(quint64 context, const QString &class_name, quint64 i1, quint64 i2, qint64 index, const QColor &bg_color, const QColor &fg_color);

    virtual QVariant item_data(int role, int flags = ShortText) const;

    virtual bool operator<(const QStandardItem &other) const;
    virtual void read(QDataStream &in);
    virtual void write(QDataStream &out) const;

    QString _text;
    QString _class_name;
};

class FunctionEntityItem : public EntityItem
{
public:
    FunctionEntityItem();
    FunctionEntityItem(const function_t &function, function_index_t fun_index, class_index_t class_index);

    class_index_t _class_index;

    virtual void read(QDataStream &in);
    virtual void write(QDataStream &out) const;
};

class ProxyEntryItem : public QStandardItem
{
public:
    ProxyEntryItem(const QString &text, const QStandardItem *parent);

    virtual bool operator<(const QStandardItem &other) const;

    QVariant data(int role) const;

    const QStandardItem *_parent;
};

//

class EntityItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit EntityItemModel(QObject *parent = 0);

    QMimeData *mimeData(const QModelIndexList &indexes) const;

    QByteArray drag_from(const QModelIndexList &indexes) const;
};

class EntityStandardItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit EntityStandardItemModel(QObject *parent = 0);

    QMimeData *mimeData(const QModelIndexList &indexes) const;

    QByteArray drag_from(const QModelIndexList &indexes) const;
};

//! QStandardItemModel with two levels, represents set of trace model items
//! First level - group root items(modules, threads, classes, etc)
//! Second level - trace model items
class TraceEntityModel : public EntityStandardItemModel
{
    Q_OBJECT

public:
    TraceEntityModel(QObject *parent = 0);
    TraceEntityModel(TraceController *controller, const QList<EntityClass> &class_list, bool full_trace_model = true, QObject *parent = 0);

    QSize span(const QModelIndex &index) const;

    void set_index_model(const trace_index_t &index_model);
    void update_trace_model();

private:
    QStandardItem * make_root_group(const QString &text);
    EntityItem * item_for_class(EntityClass class_id, const trace_index_t::iterator &it) const;

    void append_class_items(EntityClass class_id, const QList<EntityItem *> &class_items);

private:
    TraceController *_controller;
    QList<EntityClass> _class_list;
};

QMimeData* entity_mime_data(const QModelIndexList &indexes);

#endif // TRACE_MODEL_H
