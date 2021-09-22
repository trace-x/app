#ifndef CALLSTACKMODEL_H
#define CALLSTACKMODEL_H

#include <QObject>

#include "trace_table_model.h"

class CallstackModel : public TraceTableModel
{
public:
    CallstackModel(TraceDataModel *trace_model, TraceController &trace_controller, QObject *parent = 0);

    ~CallstackModel();

    void set_current_message(const trace_message_t *message);

    QVariant data(const QModelIndex &index, int role) const;

private:
    const trace_message_t *_current_message;
};

#endif // CALLSTACKMODEL_H
