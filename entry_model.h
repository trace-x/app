#ifndef ENTRY_MODEL_H
#define ENTRY_MODEL_H

#include <QList>
#include <QHash>
#include <QStandardItem>

#include "trace_tools.h"

enum
{
    ColorRole = Qt::UserRole + 600,
    TextColorRole
};

struct ItemDescriptor
{
    ItemDescriptor();
    ItemDescriptor(const char *id);
    ItemDescriptor(const QString &id);
    ItemDescriptor(const QVariant &id, const QString &label = QString());

    QVariant id;
    qint64 index;
    int class_id;
    QString label;
};

typedef QHashBuilder<int, ItemDescriptor> TraceMessageDescription;

class EntityItem : public QStandardItem
{
public:
    enum DataFlags
    {
        ShortText     = 0x1,
        FullText      = 0x2,
        OnlyIndex     = 0x4,
        WithDecorator = 0x10,
        WithBackColor = 0x20
    };

    EntityItem();
    EntityItem(int class_id);
    EntityItem(int class_id, const QVariant &id, const QString &text,
               qint64 index, const QColor &bg_color = QColor(), const QColor &fg_color = QColor(), const QString &tool_tip = QString());

    inline const ItemDescriptor & descriptor() const { return _descriptor ; }

    virtual QVariant data(int role) const;
    virtual QVariant item_data(int role, int flags = ShortText) const;

    virtual void read(QDataStream &in);
    virtual void write(QDataStream &out) const;

protected:
    ItemDescriptor _descriptor;
};

struct Entry
{
    Entry(){}
    Entry(const QString &prefix_, const QString &name_, const QList<EntityItem*> *list_,
          int role_ = Qt::DisplayRole, bool common_ = true, bool remove_equals_ = false):
        prefix(prefix_), name(name_), item_list(list_), search_role(role_),
        is_common(common_), remove_equals(remove_equals_) {}

    QString prefix;
    QString name;
    const QList<EntityItem*> *item_list;
    int search_role;
    bool is_common;
    bool remove_equals;
};

struct EntrySet
{
    QList<Entry> entries;
    QHash<QString, Entry*> entry_hash;

    void append(const Entry &entry)
    {
        entries.append(entry);
        entry_hash.insert(entry.prefix, &entries.last());
    }
};

void find_in_entries(QList<QStandardItem *> &result, const QString &prefix, const QString &pattern, const EntrySet &entry_set);

#endif // ENTRY_MODEL_H
