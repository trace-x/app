#ifndef FILTER_MODEL_H
#define FILTER_MODEL_H

#include <QStandardItemModel>

#include "trace_model.h"

class TraceController;
class FilterModel;
class FilterGroup;

struct trace_message_t;

enum
{
    FilterDataRole = Qt::UserRole + 1000,
    SearchHighlightDataRole,
    SearchIndexesDataRole,
    FunctionLevelDataRole
};

enum FilterOperator
{
    ExcludeOperator,
    IncludeOperator
};

enum
{
    FilterGroupItemType = QStandardItem::UserType + 1,
    FilterItemType
};

enum
{
    BranchCharRole = Qt::UserRole + 8000,
};

class BaseFilterItem : public QStandardItem
{
public:
    BaseFilterItem();
    BaseFilterItem(const QString &text, int class_id);

    int class_id() const;

    int _class_id;
};

//! Элемент фильтра с заданным типом и шаблоном
class FilterItem : public BaseFilterItem
{
public:
    enum IdPatterType
    {
        ID,
        Label
    };

public:
    FilterItem();
    FilterItem(const FilterItem &other);
    FilterItem(const char *id);
    FilterItem(const QVariant &id);
    FilterItem(const QString &text, const QVariant &id_pattern, int entity_class, IdPatterType id_type, qint64 index,
               const QVariant &foreground, const QVariant &decoration);
    FilterItem(const QString &pattern, int entity_class);

    FilterGroup * group_item();

    QString string_id() const;

    int type() const;
    int class_id() const;

    QVariant data(int role = Qt::UserRole + 1) const;

    bool matched(const ItemDescriptor &descriptor) const;
    bool contains(const ItemDescriptor &descriptor, QVector<QPair<int, int>> &indexes) const;

    bool is_valid() const;
    bool is_empty() const;

    bool operator ==(const FilterItem &other) const;

    void reset_index();

    bool is_exact() const;

public:
    QRegExp      _regexp;
    IdPatterType _id_type;
    QVariant     _id_pattern;
    qint64       _index;

private:
    friend class FilterModel;
    friend class FilterGroup;
    friend QDataStream & operator >> (QDataStream &in, FilterItem &value);

    void initialize();
};

//! Набор фильтров, сгруппированных по классу(OR-композиция). Может иметь вложенные наборы(AND-композиция)
class FilterGroup : public BaseFilterItem
{
public:
    FilterGroup();
    FilterGroup(const FilterGroup &other);
    FilterGroup(FilterOperator filter_operator, int class_id, const QList<FilterItem> &items = QList<FilterItem>(), const QString &class_name = QString());

    QString as_string() const;

    void set_filter_type(FilterOperator operator_type);

    bool has_child_roots() const;

    QList<FilterItem*> filter_items() const;
    QList<FilterGroup*> child_root_items() const;
    FilterGroup * group_item();

    int check_filter(const QHash<int, ItemDescriptor> &complex_value) const;
    int check_filter(const trace_message_t *message) const;

    QSet<int> get_class_set() const;
    QSet<int> get_parent_class_set() const;

    int type() const;

    int class_id() const;
    FilterOperator operator_type() const;

    QVariant data(int role = Qt::UserRole + 1) const;

    bool append_group(const FilterGroup &other);
    bool append_filter(const FilterItem &filter_item, FilterOperator filter_sign, int except = -1);

    FilterModel * model() const;

    void reset_indexes();
    void update_indexes();

    void set_class(int class_id, const QString &class_name);

public:
    FilterGroup & operator << (const FilterGroup &filter_group);
    FilterGroup & operator << (const FilterItem &filter_item);

private:
    FilterItem * item_at(int i) const;
    FilterGroup * subgroup_at(int i) const;

private:
    friend class FilterModel;

    friend QDataStream & operator << (QDataStream &, const FilterGroup &);
    friend QDataStream & operator >> (QDataStream &, FilterGroup &);

    int _item_count;
    int _subgroup_count;

    FilterOperator _operator_type;

private:
    void initialize();
};

//! Модель фильтра. По сути представляет собой набор групп FilterGroup(OR-композиция)
//! Имеет метод проверки составного значения, заданного набором идентификаторов по классам
class FilterModel : public EntityStandardItemModel
{
    Q_OBJECT

public:
    explicit FilterModel(QObject *parent = 0);
    FilterModel(const QList<FilterGroup> &filters);

    FilterModel(const FilterModel &other);

    FilterModel & operator =(const FilterModel &other);

    void set_service(TraceController *controller);
    void set_disabled_when_empty(bool disabled);

    QString class_name(int class_id) const;

    void clear();
    void reset_indexes();
    void update_index_model();

public:
    QString as_string() const;

    bool is_disabled_when_empty() const;

    bool is_enabled() const;

    //! returns true, when filter allows complex_value
    bool check_filter(const QHash<int, ItemDescriptor> &complex_value) const;

    //! fast check overloaded function
    bool check_filter(const trace_message_t *message) const;

    bool has_class_in_filter(int class_id) const;

    bool append_group(const FilterGroup &filter_group);
    bool append_filter(const FilterItem &filter_item, FilterOperator operator_type = IncludeOperator);

public:
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QStringList mimeTypes() const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    void drop_to(const QByteArray &data, const QModelIndex &parent = QModelIndex());

public:
    FilterModel & operator << (const FilterGroup &filter_group);
    bool operator ==(const FilterModel &other);

signals:
    void error(const QString &message);
    void changed();

private:
    friend class FilterGroup;

    TraceController *_controller;

    bool _disabled_when_empty;
};

QDataStream & operator << (QDataStream &out, const FilterItem &value);
QDataStream & operator >> (QDataStream &in, FilterItem &value);

QDataStream & operator << (QDataStream &out, const FilterGroup &value);
QDataStream & operator >> (QDataStream &in, FilterGroup &value);

QDataStream & operator << (QDataStream &out, const FilterModel &value);
QDataStream & operator >> (QDataStream &in, FilterModel &value);

Q_DECLARE_METATYPE(FilterGroup)
Q_DECLARE_METATYPE(FilterItem)
Q_DECLARE_METATYPE(FilterModel)

#endif // FILTER_MODEL_H
