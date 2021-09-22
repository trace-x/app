#ifndef TX_MODEL_SERVICE_H
#define TX_MODEL_SERVICE_H

#include <QObject>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include <QListView>
#include <QHash>

#include "entry_model.h"
#include "trace_model.h"
#include "filter_tree_view.h"
#include "trace_filter_model.h"

class TraceController;

//! Сервисный класс, отвечающий за хранение моделей передатчика и фильтра передатчика
//! Используется в TxFilterWidget
class TransmitterModelService : public QObject
{
    Q_OBJECT

public:
    explicit TransmitterModelService(TraceController *trace_controller, QObject *parent = 0);

    inline TraceController * trace_controller() const;

    inline TraceEntityModel * item_model() const;
    FilterModel * active_filter();
    FilterListModel * filter_model();

    int active_filter_index() const;

    QByteArray save_state() const;
    void restore_state(const QByteArray &state);

    void clear();

signals:
    void filter_changed();

public slots:
    void set_filter(const FilterListModel &capture_filter_list, int active_filter);

private:
    void emit_changed();

private:
    TraceController *_trace_controller;
    TraceEntityModel *_capture_model;

    mutable QMutex _filter_mutex;
    FilterListModel _filter_list;

    int _active_filter;
};

TraceEntityModel *TransmitterModelService::item_model() const
{
    return _capture_model;
}

TraceController *TransmitterModelService::trace_controller() const
{
    return _trace_controller;
}

#endif // TX_MODEL_SERVICE_H
