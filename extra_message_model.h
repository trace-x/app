#ifndef EXTRA_MESSAGE_MODEL_H
#define EXTRA_MESSAGE_MODEL_H

#include <QObject>
#include <QStandardItemModel>

#include "trace_model.h"
#include "trace_controller.h"

class ModelItem;

//! Table model for data visualisation of single message
class ExtraMessageModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ExtraMessageModel(const TraceController &trace_controller, QObject *parent = 0);

    void set_message(const trace_message_t *message);
    void clear();

protected:
    QMimeData *mimeData(const QModelIndexList &indexes) const;

private:
    void append_row(const QString &name, ModelItem *item);

private:
    friend class ModelItem;

    const trace_message_t *_ref_message;
    trace_message_t _message;
    const TraceController &_trace_controller;
};

#endif // EXTRA_MESSAGE_MODEL_H
