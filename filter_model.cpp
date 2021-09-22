#include "filter_model.h"

#include <QMimeData>

#include "trace_controller.h"

#include "trace_x/trace_x.h"

QDataStream &operator<<(QDataStream &out, const FilterItem &value)
{
    X_CALL_F;

    out << static_cast<const QStandardItem&>(value) << value._class_id << value._id_type << value._id_pattern << value._index;

    return out;
}

QDataStream &operator >>(QDataStream &in, FilterItem &value)
{
    X_CALL_F;

    qint32 id_type;

    in >> static_cast<QStandardItem&>(value) >> value._class_id >> id_type >> value._id_pattern >> value._index;

    value._id_type = FilterItem::IdPatterType(id_type);
    value.initialize();

    return in;
}

QDataStream &operator<<(QDataStream &out, const FilterGroup &value)
{
    X_CALL_F;

    out << static_cast<const QStandardItem&>(value) << value._class_id << qint8(value._operator_type) << qint32(value._item_count) << qint32(value._subgroup_count);

    for(int i = 0; i < value._item_count; ++i)
    {
        out << *value.item_at(i);
    }

    for(int i = 0; i < value._subgroup_count; ++i)
    {
        out << *value.subgroup_at(i);
    }

    return out;
}

QDataStream &operator >>(QDataStream &in, FilterGroup &value)
{
    X_CALL_F;

    qint8 operator_type;

    qint32 item_count;
    qint32 subgroup_count;

    in >> static_cast<QStandardItem&>(value) >> value._class_id >> operator_type >> item_count >> subgroup_count;

    for(qint32 i = 0; i < item_count; ++i)
    {
        FilterItem filter_item;

        in >> filter_item;

        value << filter_item;
    }

    for(qint32 i = 0; i < subgroup_count; ++i)
    {
        FilterGroup filter_group;

        in >> filter_group;

        value << filter_group;
    }

    value.set_filter_type(FilterOperator(operator_type));

    return in;
}

QDataStream &operator<<(QDataStream &out, const FilterModel &value)
{
    X_CALL_F;

    out << qint32(value.rowCount());

    for(int i = 0; i < value.rowCount(); ++i)
    {
        out << *static_cast<FilterGroup*>(value.item(i));
    }

    return out;
}

QDataStream &operator >>(QDataStream &in, FilterModel &value)
{
    X_CALL_F;

    qint32 group_count = 0;

    in >> group_count;

    for(qint32 i = 0; i < group_count; ++i)
    {
        FilterGroup group;

        in >> group;

        value << group;
    }

    return in;
}

/* ===================================================================================== */

namespace
{

//(0xFF0B)
static const char* OperatorTypeSymbols[] = {"-", "+"};

template<class T>
bool check_filter_list(const QStandardItem *root_item, T value, int first = 0)
{
    if(first > root_item->rowCount())
    {
        return false;
    }

    bool has_include = false;
    bool has_noninclude = false;
    bool has_exclude = false;
    bool has_nonexclude = false;

    for(int i = first; i < root_item->rowCount(); ++i)
    {
        FilterGroup *child_filter = static_cast<FilterGroup*>(root_item->child(i));

        int result = child_filter->check_filter(value);

        if(result == -1)
        {
            has_exclude = true;
            break;
        }
        else if(result == 1)
        {
            has_include = true;
        }
        else
        {
            // Ignore
            if(child_filter->operator_type() == ExcludeOperator)
            {
                has_nonexclude = true;
            }
            else
            {
                has_noninclude = true;
            }
        }
    }

    if(!has_exclude && (has_include || (has_nonexclude && !has_noninclude)))
    {
        return true;
    }

    return false;
}

}

BaseFilterItem::BaseFilterItem():
    QStandardItem(),
    _class_id(-1)
{
}

BaseFilterItem::BaseFilterItem(const QString &text, int class_id):
    QStandardItem(QString(text).replace('\n', ' ')),
    _class_id(class_id)
{
}

int BaseFilterItem::class_id() const
{
    return _class_id;
}

/////////////////

FilterItem::FilterItem() : BaseFilterItem(),
    _index(-1)
{
    initialize();
}

FilterItem::FilterItem(const FilterItem &other):
    BaseFilterItem(other.text(), other._class_id),
    _id_type(other._id_type),
    _id_pattern(other._id_pattern),
    _index(other._index)
{
    setForeground(other.foreground());
    setToolTip(other.toolTip());

    setData(other.data(Qt::DecorationRole), Qt::DecorationRole);

    initialize();
}

FilterItem::FilterItem(const char *id):
    BaseFilterItem(id, 0),
    _id_type(ID),
    _id_pattern(id),
    _index(-1)
{
    initialize();
}

FilterItem::FilterItem(const QVariant &id):
    BaseFilterItem(id.toString(), 0),
    _id_type(ID),
    _id_pattern(id),
    _index(-1)
{
    initialize();
}

FilterItem::FilterItem(const QString &text, const QVariant &id_pattern, int entity_class, IdPatterType id_type, qint64 index,
                       const QVariant &foreground, const QVariant &decoration):
    BaseFilterItem(text, entity_class),
    _id_type(id_type),
    _id_pattern(id_pattern),
    _index(index)
{
    setToolTip(text);

    setData(foreground, Qt::ForegroundRole);
    setData(decoration, Qt::DecorationRole);

    initialize();
}

FilterItem::FilterItem(const QString &pattern, int entity_class):
    BaseFilterItem(pattern, entity_class),
    _id_type(ID),
    _id_pattern(pattern),
    _index(-1)
{
    initialize();
}

FilterGroup *FilterItem::group_item()
{
    return static_cast<FilterGroup*>(parent());
}

QString FilterItem::string_id() const
{
    return _id_pattern.toString();
}

int FilterItem::type() const
{
    return FilterItemType;
}

int FilterItem::class_id() const
{
    return _class_id;
}

QVariant FilterItem::data(int role) const
{
    if(role == FilterDataRole)
    {
        return QVariant::fromValue(*this);
    }

    return QStandardItem::data(role);
}

bool FilterItem::matched(const ItemDescriptor &descriptor) const
{
    if(_regexp.isEmpty())
    {
        if(_index >= 0)
        {
            return _index == descriptor.index;
        }

        return _id_pattern == descriptor.id;
    }

    return _regexp.exactMatch(descriptor.id.toString());
}

bool find_contains(const QString &string, const QString &substring, QVector<QPair<int, int>> &indexes)
{
    int pos = 0;

    while ((pos = string.indexOf(substring, pos)) != -1)
    {
        indexes.append(QPair<int, int>(pos, pos + substring.length()));

        pos += substring.length();
    }

    return !indexes.isEmpty();
}

bool find_contains(const QString &string, const QRegExp &regepx, QVector<QPair<int, int>> &indexes)
{
    int pos = 0;

    while ((pos = regepx.indexIn(string, pos)) != -1)
    {
        indexes.append(QPair<int, int>(pos, pos + regepx.matchedLength()));

        pos += regepx.matchedLength();
    }

    return !indexes.isEmpty();
}

bool FilterItem::contains(const ItemDescriptor &descriptor, QVector<QPair<int, int>> &indexes) const
{
    if(_regexp.isEmpty())
    {
        if(_index >= 0)
        {
            /// for indexed search

            return _index == descriptor.index;
        }

        return find_contains(descriptor.id.toString(), _id_pattern.toString(), indexes);
    }

    return find_contains(descriptor.id.toString(), _regexp, indexes);
}

bool FilterItem::is_valid() const
{
    return !_id_pattern.toString().isEmpty() && (_class_id != -1);
}

bool FilterItem::is_empty() const
{
    return _id_pattern.toString().isEmpty();
}

bool FilterItem::operator ==(const FilterItem &other) const
{
    return (_index == other._index) && (_id_type == other._id_type) && (_id_pattern == other._id_pattern);
}

void FilterItem::reset_index()
{
    _index = -1;

    setData(QVariant(), Qt::DecorationRole);
}

bool FilterItem::is_exact() const
{
    return _regexp.isEmpty();
}

void FilterItem::initialize()
{
    setDropEnabled(false);
    setDragEnabled(true);
    setEditable(false);

    if(_id_pattern.toString().contains('*'))
    {
        //TODO QRegExp::RegExp

        _regexp = QRegExp(_id_pattern.toString(), Qt::CaseInsensitive, QRegExp::Wildcard);
    }
}

/////////////////

FilterGroup::FilterGroup() : BaseFilterItem()
{
    X_CALL;

    initialize();
}

FilterGroup::FilterGroup(const FilterGroup &other):
    BaseFilterItem(other.text(), other.class_id())
{
    X_CALL;

    setToolTip(other.toolTip());

    set_filter_type(other.operator_type());

    initialize();

    foreach(FilterItem *item, other.filter_items())
    {
        appendRow(new FilterItem(*item));
    }

    foreach(FilterGroup *item, other.child_root_items())
    {
        appendRow(new FilterGroup(*item));
    }

    _item_count = other._item_count;
    _subgroup_count = other._subgroup_count;
}

FilterGroup::FilterGroup(FilterOperator filter_operator, int class_id, const QList<FilterItem> &items, const QString &class_name):
    BaseFilterItem(class_name, class_id),
    _subgroup_count(0)
{
    X_CALL;

    setToolTip(class_name);

    set_filter_type(filter_operator);

    initialize();

    foreach (const FilterItem &item, items)
    {
        appendRow(new FilterItem(item));
    }

    _item_count = items.count();
}

QString FilterGroup::as_string() const
{
    QString set_string;

    for(int i = 0; i < _item_count; ++i)
    {
        if(i != 0)
        {
            set_string += ", ";
        }

        set_string += item_at(i)->text();
    }

    if(_item_count > 1)
    {
        set_string = "(" + set_string + ")";
    }

    if(_operator_type == ExcludeOperator)
    {
        set_string = "!" + set_string;
    }

    QString children_string;

    for(int i = 0; i < _subgroup_count; i++)
    {
        if(i != 0)
        {
            children_string += " + ";
        }

        children_string += subgroup_at(i)->as_string();

        ++i;
    }

    if(_subgroup_count > 1)
    {
        children_string = "(" + children_string + ")";
    }

    if(!children_string.isEmpty())
    {
        set_string = set_string + " & " + children_string;
    }

    return set_string;
}

void FilterGroup::set_filter_type(FilterOperator operator_type)
{
    X_CALL;

    _operator_type = operator_type;

    setData(QString(OperatorTypeSymbols[operator_type]), BranchCharRole);

    if(model())
    {
        emit model()->layoutChanged();
    }
}

bool FilterGroup::has_child_roots() const
{
    return (_subgroup_count != 0);
}

QList<FilterItem *> FilterGroup::filter_items() const
{
    X_CALL;

    QList<FilterItem*> result;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        if(this->child(i)->type() == FilterItemType)
        {
            result << static_cast<FilterItem*>(this->child(i));
        }
        else
        {
            break;
        }
    }

    return result;
}

QList<FilterGroup *> FilterGroup::child_root_items() const
{
    X_CALL;

    QList<FilterGroup*> result;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        if(this->child(i)->type() == FilterGroupItemType)
        {
            result << static_cast<FilterGroup*>(this->child(i));
        }
    }

    return result;
}

FilterGroup *FilterGroup::group_item()
{
    return static_cast<FilterGroup*>(parent());
}

int FilterGroup::check_filter(const QHash<int, ItemDescriptor> &complex_value) const
{
    X_CALL;

    //! -1 - exclude
    //! 0  - ignore
    //! 1  - include

    bool in_set = false;

    for(int i = 0; i < _item_count; ++i)
    {
        if(item_at(i)->matched(complex_value[_class_id]))
        {
            in_set = true;

            break;
        }
    }

    if(!in_set)
    {
        return 0;
    }

    if(::check_filter_list(this, complex_value, _item_count) || !_subgroup_count)
    {
        return (_operator_type == ExcludeOperator) ? -1 : 1;
    }

    return 0;
}

int FilterGroup::check_filter(const trace_message_t *message) const
{
    X_CALL;

    //! -1 - exclude
    //! 0  - ignore
    //! 1  - include

    bool in_set = false;

    for(int i = 0; i < _item_count; ++i)
    {
        // TODO make it simpler ?

        if(_class_id == MessageTextEntity)
        {
            if(item_at(i)->matched(model()->_controller->message_text_at(message)))
            {
                in_set = true;

                break;
            }
        }
        else
        {
            if(item_at(i)->matched(model()->_controller->items_by_class(EntityClass(_class_id), true).at(model()->_controller->index_by_class(message, _class_id))->descriptor()))
            {
                in_set = true;

                break;
            }
        }
    }

    if(!in_set)
    {
        return 0;
    }

    if(::check_filter_list(this, message, _item_count) || !_subgroup_count)
    {
        return (_operator_type == ExcludeOperator) ? -1 : 1;
    }

    return 0;
}

QSet<int> FilterGroup::get_class_set() const
{
    QSet<int> result_set;

    result_set.insert(_class_id);

    for(int i = 0; i < _subgroup_count; ++i)
    {
        result_set.unite(subgroup_at(i)->get_class_set());
    }

    return result_set;
}

QSet<int> FilterGroup::get_parent_class_set() const
{
    QSet<int> result_set;

    QStandardItem *parent = this->parent();

    result_set.insert(_class_id);

    while(parent && (parent->type() == FilterGroupItemType))
    {
        result_set.insert(static_cast<FilterGroup*>(parent)->class_id());

        parent = parent->parent();
    }

    return result_set;
}

void FilterGroup::initialize()
{
    _item_count = 0;
    _subgroup_count = 0;

    setDragEnabled(true);
    setEditable(false);

    QFont font = this->font();
    font.setBold(true);

    setFont(font);
}

int FilterGroup::type() const
{
    return FilterGroupItemType;
}

int FilterGroup::class_id() const
{
    return _class_id;
}

FilterOperator FilterGroup::operator_type() const
{
    return _operator_type;
}

QVariant FilterGroup::data(int role) const
{
    if(role == FilterDataRole)
    {
        return QVariant::fromValue(*this);
    }

    return QStandardItem::data(role);
}

bool FilterGroup::append_group(const FilterGroup &other)
{
    X_CALL;

    // check for parents nesting - we can`t have nesting possibility on parents filters chain

    if(this->get_parent_class_set().intersect(other.get_class_set()).isEmpty())
    {
        if(_class_id == other.class_id())
        {
            // first, merge filter items

            for(int i = 0; i < other._item_count; ++i)
            {
                append_filter(*other.item_at(i), _operator_type);
            }

            // second, merge subgroups

            for(int i = 0; i < other._subgroup_count; ++i)
            {
                append_group(*other.subgroup_at(i));
            }

            return true;
        }
        else
        {
            bool is_unique = true;

            for(int i = 0; i < _subgroup_count; ++i)
            {
                if(other.class_id() == this->subgroup_at(i)->class_id())
                {
                    is_unique = false;

                    this->subgroup_at(i)->append_group(other);

                    break;
                }
            }

            if(is_unique)
            {
                appendRow(new FilterGroup(other));

                _subgroup_count++;

                if(model())
                {
                    emit model()->layoutChanged();
                }
            }
        }
    }

    return false;
}

bool FilterGroup::append_filter(const FilterItem &filter_item, FilterOperator filter_sign, int except)
{
    X_CALL;

    if(!filter_item.is_valid())
    {
        set_filter_type(filter_sign);

        return false;
    }

    if(filter_item.class_id() == _class_id)
    {
        set_filter_type(filter_sign);

        bool is_unique = true;

        // check for unique
        for(int i = 0; i < _item_count; ++i)
        {
            if((filter_item == *item_at(i)) && (i != except))
            {
                is_unique = false;
                break;
            }
        }

        if(is_unique)
        {
            _item_count++;

            insertRow(has_child_roots() ? rowCount() - 1 : rowCount(), new FilterItem(filter_item));

            if(model())
            {
                emit model()->layoutChanged();
            }

            return false;
        }
    }
    else
    {
        append_group(FilterGroup(filter_sign, filter_item.class_id(), QList<FilterItem>() << filter_item, model() ? model()->class_name(filter_item.class_id()) : ""));
    }

    return false;
}

FilterModel *FilterGroup::model() const
{
    return static_cast<FilterModel*>(QStandardItem::model());
}

void FilterGroup::reset_indexes()
{
    X_CALL;

    for(int i = 0; i < _item_count; ++i)
    {
        item_at(i)->reset_index();
    }

    for(int i = 0; i < _subgroup_count; i++)
    {
        subgroup_at(i)->reset_indexes();
    }
}

void FilterGroup::update_indexes()
{
    X_CALL;

    for(int i = 0; i < _item_count; ++i)
    {
        FilterItem *item = item_at(i);

        if(item->is_exact() && (item->_index == -1))
        {
            EntityItem * entity = model()->_controller->item_by_descriptor_id(EntityClass(item->class_id()), item->_id_pattern);

            if(entity)
            {
                item->setText(entity->item_data(Qt::DisplayRole, EntityItem::FullText).toString());
                item->setData(entity->data(ColorRole), Qt::DecorationRole);
                item->setData(entity->data(Qt::ForegroundRole), Qt::ForegroundRole);
                item->_index = entity->descriptor().index;
            }
        }
    }

    for(int i = 0; i < _subgroup_count; i++)
    {
        subgroup_at(i)->update_indexes();
    }
}

void FilterGroup::set_class(int class_id, const QString &class_name)
{
    _class_id = class_id;

    setText(class_name);
    setToolTip(class_name);
}

FilterGroup &FilterGroup::operator <<(const FilterGroup &filter_group)
{
    X_CALL;

    append_group(filter_group);

    return *this;
}

FilterGroup &FilterGroup::operator <<(const FilterItem &filter_item)
{
    X_CALL;

    append_filter(filter_item, IncludeOperator);

    return *this;
}

FilterItem *FilterGroup::item_at(int i) const
{
    return static_cast<FilterItem*>(this->child(i));
}

FilterGroup *FilterGroup::subgroup_at(int i) const
{
    return static_cast<FilterGroup*>(this->child(i + _item_count));
}

////////////////

FilterModel::FilterModel(QObject *parent):
    EntityStandardItemModel(parent),
    _controller(0),
    _disabled_when_empty(false)
{
    X_CALL;
}

FilterModel::FilterModel(const QList<FilterGroup> &filters):
    EntityStandardItemModel(),
    _controller(0),
    _disabled_when_empty(false)
{
    X_CALL;

    foreach (const FilterGroup &filter_group, filters)
    {
        appendRow(new FilterGroup(filter_group));
    }
}

FilterModel::FilterModel(const FilterModel &other):
    EntityStandardItemModel(),
    _controller(0),
    _disabled_when_empty(other._disabled_when_empty)
{
    X_CALL;

    for(int i = 0; i < other.rowCount(); ++i)
    {
        appendRow(new FilterGroup(*static_cast<FilterGroup*>(other.item(i))));
    }
}

FilterModel &FilterModel::operator =(const FilterModel &other)
{
    X_CALL;

    this->clear();

    for(int i = 0; i < other.rowCount(); ++i)
    {
        appendRow(new FilterGroup(*static_cast<FilterGroup*>(other.item(i))));
    }

    _disabled_when_empty = other._disabled_when_empty;

    return *this;
}

void FilterModel::set_service(TraceController *controller)
{
    X_CALL;

    _controller = controller;

    if(_controller)
    {
        connect(_controller, &TraceController::model_updated, this, &FilterModel::update_index_model);
    }
}

void FilterModel::set_disabled_when_empty(bool disabled)
{
    _disabled_when_empty = disabled;
}

QString FilterModel::class_name(int class_id) const
{
    if(_controller)
    {
        return _controller->filter_class_name(class_id);
    }

    return QString::number(class_id);
}

void FilterModel::clear()
{
    X_CALL;

    EntityStandardItemModel::clear();

    emit layoutChanged();
}

void FilterModel::reset_indexes()
{
    X_CALL;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        static_cast<FilterGroup*>(this->item(i))->reset_indexes();
    }
}

void FilterModel::update_index_model()
{
    X_CALL;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        static_cast<FilterGroup*>(this->item(i))->update_indexes();
    }

    //TODO ? dataChanged ?

    //Лучше этого здесь не делать!!!

    // emit layoutChanged();
}

QString FilterModel::as_string() const
{
    X_CALL;

    if(hasChildren())
    {
        QString string;

        for(int i = 0; i < this->rowCount(); ++i)
        {
            if(i != 0)
            {
                string += " + ";
            }

            string += static_cast<FilterGroup*>(this->item(i))->as_string();
        }

        return string;
    }
    else if(_disabled_when_empty)
    {
        return QObject::tr("Include All");
    }

    return QObject::tr("Exclude All");
}

bool FilterModel::is_disabled_when_empty() const
{
    return _disabled_when_empty;
}

bool FilterModel::is_enabled() const
{
    return !(_disabled_when_empty && !hasChildren());
}

bool FilterModel::check_filter(const QHash<int, ItemDescriptor> &complex_value) const
{
    return check_filter_list(this->invisibleRootItem(), complex_value);
}

bool FilterModel::check_filter(const trace_message_t *message) const
{
    return check_filter_list(this->invisibleRootItem(), message);
}

bool FilterModel::has_class_in_filter(int class_id) const
{
    X_CALL;

    bool result = false;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        FilterGroup* group = static_cast<FilterGroup*>(item(i));

        if(group->get_class_set().contains(class_id))
        {
            result = true;

            break;
        }
    }

    return result;
}

bool FilterModel::append_group(const FilterGroup &filter_group)
{
    X_CALL;

    bool is_new = true;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        FilterGroup* group = static_cast<FilterGroup*>(item(i));

        if(group->class_id() == filter_group.class_id())
        {
            group->append_group(filter_group);

            is_new = false;

            break;
        }
    }

    if(is_new)
    {
        appendRow(new FilterGroup(filter_group));

        emit layoutChanged();
    }

    return true;
}

bool FilterModel::append_filter(const FilterItem &filter_item, FilterOperator operator_type)
{
    X_CALL;

    if(!filter_item.is_valid())
    {
        return false;
    }

    bool is_new = true;

    for(int i = 0; i < this->rowCount(); ++i)
    {
        FilterGroup* group = static_cast<FilterGroup*>(item(i));

        if(group->class_id() == filter_item.class_id())
        {
            group->append_filter(filter_item, operator_type);

            is_new = false;

            break;
        }
    }

    if(is_new)
    {
        appendRow(new FilterGroup(operator_type, filter_item.class_id(), QList<FilterItem>() << filter_item,
                                  this->class_name(filter_item.class_id())));

        emit layoutChanged();
    }

    return true;
}

bool FilterModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int , int , const QModelIndex &parent)
{
    X_CALL;

    if(data->hasFormat("application/trace.items.list"))
    {
        drop_to(data->data("application/trace.items.list"), parent);

        return true;
    }

    return false;
}

QStringList FilterModel::mimeTypes() const
{
    return QStringList() << "application/trace.items.list";
}

bool FilterModel::removeRows(int row, int count, const QModelIndex &parent)
{
    X_CALL;

    FilterGroup *parent_group = static_cast<FilterGroup*>(itemFromIndex(parent));

    if(parent_group)
    {
        for(int i = row; i < row + count; ++i)
        {
            QStandardItem *item = itemFromIndex(parent.child(i, 0));

            if(item->type() == FilterGroupItemType)
            {
                parent_group->_subgroup_count--;
            }
            else
            {
                parent_group->_item_count--;
            }
        }
    }

    bool result = EntityStandardItemModel::removeRows(row, count, parent);

    if(parent_group && !parent_group->_item_count)
    {
        removeRows(parent_group->row(), 1, parent.parent());
    }

    return result;
}

void FilterModel::drop_to(const QByteArray &data, const QModelIndex &parent)
{
    X_CALL;

    QDataStream stream(data);

    QVariantList item_list;

    stream >> item_list;

    QStandardItem *parent_item = parent.isValid() ? this->itemFromIndex(parent) : this->invisibleRootItem();

    if(parent_item->type() == FilterItemType)
    {
        parent_item = parent_item->parent();
    }

    FilterGroup *root_group_item = static_cast<FilterGroup*>(parent_item);

    foreach(const QVariant &item, item_list)
    {
        if(item.canConvert<FilterItem>())
        {
            if(!parent.isValid())
            {
                this->append_filter(item.value<FilterItem>());
            }
            else
            {
                root_group_item->append_filter(item.value<FilterItem>(), root_group_item->operator_type());
            }
        }
        else if(item.canConvert<FilterGroup>())
        {
            if(!parent.isValid())
            {
                this->append_group(item.value<FilterGroup>());
            }
            else
            {
                root_group_item->append_group(item.value<FilterGroup>());
            }
        }
    }
}

FilterModel &FilterModel::operator <<(const FilterGroup &filter_group)
{
    append_group(filter_group);

    return *this;
}

bool FilterModel::operator ==(const FilterModel &other)
{
    return (this == &other);
}
