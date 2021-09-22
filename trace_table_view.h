#ifndef TRACE_TABLE_VIEW_H
#define TRACE_TABLE_VIEW_H

#include <QTableView>

#include "trace_table_model.h"
#include "trace_filter_widget.h"

class ScrollBar;

//! Table view of trace list
class TraceTableView : public TableView
{
    Q_OBJECT

public:
    explicit TraceTableView(QWidget *parent = 0);

    void initialize(TraceController *controller, TraceTableModel *model);

    void set_model(TraceTableModel *model);
    void set_filter(FilterChain filter);

    void setup_font();

    //! Set current index(i.e synchronized with other tables)
    void select_by_index(index_t message_index, bool clear_selection = true, bool set_current = false, int column = 0);

    void select_call();
    void select_return();
    void jump_to_next_call();
    void jump_to_prev_call();

    void set_active(bool is_selected);
    bool is_active() const;

    void exclude_selected();

public slots:
    void set_autoscroll(bool enabled);

    void find_next(/*bool next_current = true*/);
    void find_prev();

public:
    TraceTableModel * model() const;
    TraceFilterWidget * filter_widget() const;
    index_t current_message_index() const;
    FilterChain filter_set() const;

signals:
    void current_changed(const trace_message_t *message);
    void selection_duration_changed(uint64_t duration);
    void table_activated();

protected:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);

    void dropEvent(QDropEvent *event);

private:
    void slot_current_changed(const QModelIndex &current, const QModelIndex &prev);
    void slot_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);

    void reset_current();
    void restore_selected_index();
    void update_scroll();
    void update_columns();

    void update_search();
    void clear_search();
    bool find_highlighted(size_t start, size_t end, int step);

private:
    TraceController *_controller;
    TraceTableModel *_model;
    TraceFilterWidget *_filter_widget;
    FilterChain _filter_set;
    TableItemDelegate *_item_delegate;
    TableItemDelegate *_message_item_delegate;

    ScrollBar *_scrollbar;

    bool _auto_scroll;

    index_t _current_message_index;

    index_t _selected_message_id;
    int _selected_column;
};

#endif // TRACE_TABLE_VIEW_H
