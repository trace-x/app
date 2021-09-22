#ifndef TRACE_FILTER_WIDGET_H
#define TRACE_FILTER_WIDGET_H

#include <QFrame>
#include <QTimer>

#include "trace_controller.h"

namespace Ui
{
class TraceFilterWidget;
}

//! Виджет для отображения и редактирования списка фильтров
class TraceFilterWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TraceFilterWidget(QWidget *parent = 0);

    ~TraceFilterWidget();

    void set_bright_theme();

    void set_hideable(bool enabled);
    void set_auto_hide(bool enabled);

    void disbale_auto_hide();

    void initialize(FilterListModel *filter_model, int active_filter, const QString &title,
                    TraceController *controller, const QList<EntityClass> &class_list);

    void set_current_filter(int index);

    int current_filter_index() const;
    FilterModel *current_filter_model() const;
    FilterTreeView *filter_view() const;

    QFrame *tool_panel();

public slots:
    void show_content(bool hide_after_show = false);
    void hide_content();

signals:
    void filter_changed();

    void _remove_row_later(int row, const QModelIndex &parent);

private slots:
    void switch_filter(int index);
    void add_model();
    void remove_current_model();

    void add_item();
    void edit_item();
    void remove_row(int row, const QModelIndex &parent);

    void update_list();
    void update_filter();

private:
    bool eventFilter(QObject *, QEvent *);

private:
    Ui::TraceFilterWidget *ui;

    FilterListModel *_filter_model;
    TraceController *_controller;

    QList<EntityClass> _class_list;

    bool _hideable;
    bool _auto_hide;
    bool _is_hidden;

    QTimer _hide_timer;
};

#endif // TRACE_FILTER_WIDGET_H
