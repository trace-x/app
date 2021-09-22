#ifndef TRACE_FILTER_MODEL_H
#define TRACE_FILTER_MODEL_H

#include "filter_model.h"

class TraceController;

typedef QPair<const FilterModel*, const FilterModel*> FilterChain;

//class FilterChain
//{
//public:
//    FilterChain(const QVector<const FilterModel*> &chain = QVector<const FilterModel*>());

//    void set_chain(const QVector<const FilterModel*> &chain);

//    bool operator < (const FilterChain &other) const
//    {
//        return _chain < other._chain;
//    }

//    bool check_filter(const QHash<int, ItemDescriptor> &complex_value) const;
//    bool check_filter(const trace_message_t *message) const;

//private:
//    std::vector<const FilterModel*> _chain;
//};

//! Модель набора фильтров
class FilterListModel : public QObject
{
    Q_OBJECT

public:
    explicit FilterListModel(bool auto_disabling = true, TraceController *controller = 0, QObject *parent = 0);

    void set_controller(TraceController *controller);

    FilterListModel(const FilterListModel &other);

    FilterListModel & operator = (const FilterListModel &other);

    const QList<FilterModel> & models() const;
    QList<FilterModel> & models();

    QStandardItemModel * names_model();

    int add_model();
    void remove_model(int index);

    void update_names();

private:
    friend QDataStream & operator << (QDataStream &, const FilterListModel &);
    friend QDataStream & operator >> (QDataStream &, FilterListModel &);

    void register_model(FilterModel *model);
    void update_model_name(FilterModel *model);

    void update_model();

    QString model_text(int i, const FilterModel &model) const;

signals:
    void filter_added(FilterModel *model);
    void filter_changed(FilterModel *model);
    void list_changed();

private:
    TraceController *_trace_controller;

    QStandardItemModel _names_model;
    QList<FilterModel> _models;

    bool _auto_disabling;
};

QDataStream & operator << (QDataStream &out, const FilterListModel &value);
QDataStream & operator >> (QDataStream &in, FilterListModel &value);

Q_DECLARE_METATYPE(FilterListModel)

#endif // TRACE_FILTER_MODEL_H
