#include "callstack_model.h"

CallstackModel::CallstackModel(TraceDataModel *trace_model, TraceController &trace_controller, QObject *parent):
    TraceTableModel(trace_model, trace_controller, QList<TraceTableModel::TraceColumns>() << TraceTableModel::Function, parent),
    _current_message(0)
{
}

CallstackModel::~CallstackModel()
{
}

void CallstackModel::set_current_message(const trace_message_t *message)
{
    _current_message = message;
}

QVariant CallstackModel::data(const QModelIndex &index, int role) const
{
    if(!_trace_model)
        return QVariant();

    {
        QMutexLocker locker(_trace_model->mutex());

        const trace_message_t *message = _trace_model->at(index.row());

        if((message == _current_message) && (role == Qt::DecorationRole))
        {
            return QIcon(":/icons/callstack_current");
        }
    }

    return TraceTableModel::data(index, role);
}
