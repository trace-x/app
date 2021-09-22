#ifndef TX_FILTER_WIDGET_H
#define TX_FILTER_WIDGET_H

#include <QFrame>
#include <QTreeView>

#include "common_ui_tools.h"
#include "trace_controller.h"
#include "tx_model_service.h"

namespace Ui
{
class TxFilterWidget;
}

//! Виджет, отвечающий за работу с фильтром захвата
class TxFilterWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TxFilterWidget(TransmitterModelService &model_service, QWidget *parent = 0);

    ~TxFilterWidget();

    void restore();

private slots:
    void apply_filter();
    void add_selected();
    void add_items(const QModelIndex &index);

    void show_changed();
    void clear_changes();

private:
    FilterModel * current_filter_model() const;
    void register_model(FilterModel *model);
    void keyPressEvent(QKeyEvent *);

private:
    Ui::TxFilterWidget *ui;

    FilterListModel _filter_model; //temporary model for "apply"-mechanism

    ModelTreeView *_model_view;
    TransmitterModelService &_model_service;
};

#endif // TX_FILTER_WIDGET_H
