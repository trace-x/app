#include "entry_model.h"
#include "filter_model.h"

#include "trace_x/trace_x.h"

#include <QSet>

EntityItem::EntityItem():
    QStandardItem()
{}

EntityItem::EntityItem(int class_id)
{
    _descriptor.class_id = class_id;
}

EntityItem::EntityItem(int class_id, const QVariant &id, const QString &text,
                       qint64 index, const QColor &bg_color, const QColor &fg_color, const QString &tool_tip):
    QStandardItem(text)
{
    if(!tool_tip.isEmpty())
    {
        setToolTip(tool_tip);
    }
    else
    {
        setToolTip(id.toString());
    }

    if(bg_color.isValid())
    {
        setData(bg_color, ColorRole);
    }

    if(fg_color.isValid())
    {
        setData(fg_color, TextColorRole);
    }

    _descriptor.class_id = class_id;
    _descriptor.id = id;
    _descriptor.index = index;
}

QVariant EntityItem::data(int role) const
{
    return item_data(role, FullText);
}

QVariant EntityItem::item_data(int role, int flags) const
{
    if(role == FilterDataRole)
    {
        return QVariant::fromValue(FilterItem(text(), _descriptor.id, _descriptor.class_id, FilterItem::ID, _descriptor.index,
                                              data(Qt::ForegroundRole), data(ColorRole)));
    }

    if((flags & EntityItem::WithDecorator) && (role == Qt::DecorationRole))
    {
        return QStandardItem::data(ColorRole);
    }

    if((flags & EntityItem::WithBackColor) && (role == Qt::BackgroundRole))
    {
        return QStandardItem::data(ColorRole);
    }

    return QStandardItem::data(role);
}

void EntityItem::read(QDataStream &in)
{
    in >> _descriptor.class_id;
    in >> _descriptor.id;
    in >> _descriptor.index;
    in >> _descriptor.label;

    QStandardItem::read(in);
}

void EntityItem::write(QDataStream &out) const
{
    out << _descriptor.class_id;
    out << _descriptor.id;
    out << _descriptor.index;
    out << _descriptor.label;

    QStandardItem::write(out);
}

//! Процедура фильтрации элементов модели по тексту
QStandardItem * parse_entry(const Entry &entry, const QRegExp &re)
{
    X_CALL_F;

    QList<QStandardItem*> result_list;

    QSet<QString> string_set;

    if(!entry.item_list->isEmpty())
    {
        // Выравниваем слова по колонкам
        int word_count = entry.item_list->last()->data(entry.search_role).toString().split(' ').size();

        QVector<int> word_max_len(word_count);

        foreach(const QStandardItem *item, *entry.item_list)
        {
            QString text = item->data(entry.search_role).toString();
            QString display_text = text;

            if(!display_text.isEmpty() && text.contains(re))
            {
                if(!entry.remove_equals || !string_set.contains(text))
                {
                    if(entry.remove_equals)
                    {
                        string_set.insert(text);
                    }

                    if(word_count > 1)
                    {
                        QStringList word_list = text.split(' ');

                        for(int i = 0; i < word_list.size(); ++i)
                        {
                            if(word_list[i].length() > word_max_len[i])
                            {
                                word_max_len[i] = word_list[i].length();
                            }
                        }
                    }

                    QStandardItem *it = new ProxyEntryItem(display_text, item);

                    result_list.append(it);
                }
            }
        }

        if(word_count > 1)
        {
            for(int i = 0; i < result_list.size(); ++i)
            {
                QString complete_text;

                QStringList word_list = result_list[i]->text().split(' ');

                for(int k = 0; k < word_list.size(); ++k)
                {
                    complete_text += word_list[k] + " ";

                    if(k != word_list.size() - 1)
                    {
                        complete_text += QString(' ').repeated(word_max_len[k] - word_list[k].length());
                    }
                }

                result_list[i]->setText(complete_text);
            }
        }
    }

   // if(!result_list.isEmpty())
    {
        QStandardItem *entry_root = new QStandardItem(entry.name);

        entry_root->setEnabled(false);
        entry_root->appendRows(result_list);
        entry_root->sortChildren(0);

        return entry_root;
    }

    return 0;
}

void find_in_entries(QList<QStandardItem *> &result, const QString &prefix, const QString &pattern, const EntrySet &entry_set)
{
    X_CALL_F;

    QRegExp re(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);

    if(prefix.isEmpty())
    {
        foreach(const Entry &entry, entry_set.entries)
        {
            if(entry.is_common)
            {
                QStandardItem *entry_root = parse_entry(entry, re);

                if(entry_root->rowCount() > 0)
                {
                    result.append(entry_root);
                }
            }
        }
    }
    else
    {
        if(Entry *entry = entry_set.entry_hash.value(prefix))
        {
            result.append(parse_entry(*entry, re));
        }
    }
}


ItemDescriptor::ItemDescriptor() : index(-1), class_id(-1) {}

ItemDescriptor::ItemDescriptor(const char *id_): id(id_), index(-1), class_id(-1) {}

ItemDescriptor::ItemDescriptor(const QString &id_): id(id_), index(-1), class_id(-1) {}

ItemDescriptor::ItemDescriptor(const QVariant &id_, const QString &label_): id(id_), index(-1), class_id(-1), label(label_) {}
