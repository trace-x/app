#include "tx_model_service.h"

#include <QFileInfo>

#include "settings.h"
#include "trace_controller.h"

#include "trace_x/trace_x.h"

TransmitterModelService::TransmitterModelService(TraceController *trace_controller, QObject *parent):
    QObject(parent),
    _trace_controller(trace_controller),
    _filter_list(true, trace_controller, 0),
    _active_filter(0)
{
    X_CALL;

    _capture_model = new TraceEntityModel(trace_controller, x_settings().capture_layout, true, this);
}

FilterModel * TransmitterModelService::active_filter()
{
    QMutexLocker lock(&_filter_mutex);

    if(_active_filter != -1)
    {
        return &_filter_list.models()[_active_filter];
    }

    return 0;
}

FilterListModel *TransmitterModelService::filter_model()
{
    return &_filter_list;
}

int TransmitterModelService::active_filter_index() const
{
    return _active_filter;
}

QByteArray TransmitterModelService::save_state() const
{
    X_CALL;

    QByteArray state;

    QDataStream stream(&state, QIODevice::WriteOnly);

    QMutexLocker lock(&_filter_mutex);

    stream << _filter_list;
    stream << _active_filter;

    return state;
}

void TransmitterModelService::restore_state(const QByteArray &state)
{
    X_CALL;

    if(!state.isEmpty())
    {
        QDataStream stream(state);

        stream >> _filter_list;
        stream >> _active_filter;

        foreach (EntityItem *process, _trace_controller->_process_models)
        {
            static_cast<ProcessModel*>(process)->update_filter();
        }

        emit filter_changed();
    }
}

void TransmitterModelService::clear()
{
    X_CALL;

    _capture_model->clear();
}

void TransmitterModelService::set_filter(const FilterListModel &capture_filter_list, int active_capture_filter)
{
    X_CALL;

    {
        QMutexLocker lock(&_filter_mutex);

        _filter_list = capture_filter_list;
        _active_filter = active_capture_filter;
    }

    emit_changed();
}

void TransmitterModelService::emit_changed()
{
    X_CALL;

    foreach (EntityItem *process, _trace_controller->_process_models)
    {
        static_cast<ProcessModel*>(process)->update_filter();
    }

    emit filter_changed();
}
