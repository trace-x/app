#ifndef TRACE_TABLE_MODEL_H
#define TRACE_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QFont>
#include <QSize>
#include <QColor>

#include "trace_model.h"
#include "trace_controller.h"

class TraceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum TraceColumns
    {
        Number = 0,
        Timestamp,
        Process,
        Module,
        Thread,
        Type,
        Context,
        Message,
        Level,
        Function
    };

public:
    TraceTableModel(TraceDataModel *trace_model, TraceController &trace_controller,
                    const QList<TraceColumns> &columns, QObject *parent = 0);

    void set_data(TraceDataModel *data_model);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QString column_size_hint(int column) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;

    inline const trace_message_t * at(int index) const;

    inline TraceDataModel * data_model() const;
    inline const TraceController &controller() const;

signals:
    void cleaned();
    void refiltered();

private slots:
    void model_updated();

private:
    QVariant data_number(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_timestamp(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_process(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_module(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_thread(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_context(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_message_type(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_call_level(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_message(const trace_message_t *message, const QModelIndex &index, int role) const;
    QVariant data_function(const trace_message_t *message, const QModelIndex &index, int role) const;

protected:
    struct ColumnData
    {
        ColumnData() {}
        ColumnData(const QString &text_, const QString tool_tip_, const QString &width_):
            text(text_), tool_tip(tool_tip_), width(width_) {}

        QString text;
        QString tool_tip;
        QString width;
    };

    QVector<ColumnData> _header_model;

    TraceDataModel *_trace_model;
    TraceController &_trace_controller;

    int _number_index;
    int _timestamp_index;
    int _process_index;
    int _module_index;
    int _thread_index;
    int _context_index;
    int _message_type_index;
    int _call_level_index;
    int _message_index;
    int _function_index;
};

const trace_message_t *TraceTableModel::at(int index) const
{
    return _trace_model->value(index);
}

TraceDataModel *TraceTableModel::data_model() const
{
    return _trace_model;
}

const TraceController &TraceTableModel::controller() const
{
    return _trace_controller;
}

#endif // TRACE_TABLE_MODEL_H
