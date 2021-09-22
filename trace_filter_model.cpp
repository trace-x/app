#include "trace_filter_model.h"

#include "trace_controller.h"
#include "trace_x/trace_x.h"

FilterListModel::FilterListModel(bool auto_disabling, TraceController *controller, QObject *parent):
    QObject(parent),
    _trace_controller(controller),
    _auto_disabling(auto_disabling)
{
    add_model();
}

FilterListModel::FilterListModel(const FilterListModel &other):
    QObject(),
    _trace_controller(other._trace_controller),
    _models(other._models),
    _auto_disabling(other._auto_disabling)
{
    update_names();
}

void FilterListModel::set_controller(TraceController *controller)
{
    _trace_controller = controller;
}

FilterListModel &FilterListModel::operator =(const FilterListModel &other)
{
    X_CALL;

    _models = other._models;
    _trace_controller = other._trace_controller;
    _auto_disabling = other._auto_disabling;

    for(int i = 0; i < _models.size(); i++)
    {
        register_model(&_models[i]);
    }

    update_names();

    return *this;
}

const QList<FilterModel> & FilterListModel::models() const
{
    return _models;
}

QList<FilterModel> &FilterListModel::models()
{
    return _models;
}

QStandardItemModel *FilterListModel::names_model()
{
    return &_names_model;
}

int FilterListModel::add_model()
{
    X_CALL;

    _models << FilterModel();

    register_model(&_models.last());

    update_names();

    return _models.size() - 1;
}

void FilterListModel::remove_model(int index)
{
    X_CALL;

    QMutexLocker locker(_trace_controller->trace_model_service().filter_mutex());

    _models.removeAt(index);

    update_names();
}

void FilterListModel::register_model(FilterModel *model)
{
    X_CALL;

    model->set_service(_trace_controller);
    model->set_disabled_when_empty(_auto_disabling);
    model->update_index_model();

    connect(model, &FilterModel::layoutChanged, this, &FilterListModel::update_model);
    connect(model, &FilterModel::rowsRemoved, this, &FilterListModel::update_model, Qt::QueuedConnection);

    emit filter_added(model);
}

void FilterListModel::update_model_name(FilterModel *model)
{
    int index = _models.indexOf(*model);

    _names_model.item(index)->setText(model_text(index, *model));
}

void FilterListModel::update_model()
{
    FilterModel *model = static_cast<FilterModel*>(sender());

    update_model_name(model);
    emit filter_changed(model);
}

QString FilterListModel::model_text(int i, const FilterModel &model) const
{
    return (_models.size() > 1 ? QString("%1. ").arg(i + 1) : "") +  model.as_string();
}

void FilterListModel::update_names()
{
    emit _names_model.layoutAboutToBeChanged();

    _names_model.blockSignals(true);

    _names_model.clear();

    for(int i = 0; i < _models.size(); i++)
    {
        _names_model.appendRow(new QStandardItem(model_text(i, _models[i])));
    }

    names_model()->blockSignals(false);

    emit _names_model.layoutChanged();
}

QDataStream &operator <<(QDataStream &out, const FilterListModel &value)
{
    X_CALL_F;

    // Индексы сбрасываются, т.к в новой сессии они уже могут оказаться невалидными

    QList<FilterModel> filters = value._models;

    for(int i = 0; i < filters.size(); ++i)
    {
        filters[i].reset_indexes();
    }

    out << filters;

    return out;
}

QDataStream &operator >>(QDataStream &in, FilterListModel &value)
{
    X_CALL_F;

    in >> value._models;

    for(int i = 0; i < value._models.size(); i++)
    {
        value.register_model(&value._models[i]);
    }

    value.update_names();

    return in;
}
