#ifndef FILTER_ITEM_EDITOR_H
#define FILTER_ITEM_EDITOR_H

#include <QDialog>

namespace Ui {
class FilterItemEditor;
}

#include "trace_controller.h"
#include "filter_model.h"
#include "completer.h"

class FilterItemEditor : public QDialog
{
    Q_OBJECT

public:
    FilterItemEditor(TraceController &controller, const QList<EntityClass> &entity_list, int initial_class, FilterOperator filter_sign, QWidget *parent = 0);
    FilterItemEditor(TraceController &controller, const QList<EntityClass> &entity_list, const FilterItem &edited_item, FilterOperator filter_sign, QWidget *parent = 0);

    ~FilterItemEditor();

    FilterItem filter() const;
    FilterOperator filter_sign() const;

private:
    void initialize(int initial_class, const QList<EntityClass> &entity_list, FilterOperator filter_sign, const QString &pre_id = QString());
    void entry_accepted(const QModelIndex &index);
    void text_accepted();

private slots:
    void clear_selection();

private:
    Ui::FilterItemEditor *ui;

    TraceController &_controller;

    TraceCompleter *_completer;

    FilterItem _accepted_filter;
};

#endif // FILTER_ITEM_EDITOR_H
