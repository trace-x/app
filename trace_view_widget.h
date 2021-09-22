#ifndef TRACE_VIEW_WIDGET_H
#define TRACE_VIEW_WIDGET_H

#include <QFrame>
#include <QSplitter>
#include <QTextEdit>
#include <QMenuBar>
#include <QLabel>
#include <QGraphicsView>
#include <QStackedWidget>
#include <QTextBrowser>
#include "trace_service.h"
#include "trace_table_model.h"
#include "callstack_model.h"
#include "extra_message_model.h"
#include "tx_filter_widget.h"
#include "completer.h"
#include "trace_model.h"
#include "trace_filter_widget.h"
#include "trace_table_view.h"
#include "panel_manager.h"
#include "code_browser.h"

#include "image_lib/image_view.h"
#include "image_lib/image_scene.h"
#include "image_lib/image_browser.h"

typedef void(TraceTableView::*table_method_t)(void);

namespace Ui
{
class TraceViewWidget;
}

//! Главный виджет управления трассой
class TraceViewWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TraceViewWidget(QWidget *parent = 0);
    ~TraceViewWidget();

    void initialize(TraceService *trace_service, const QByteArray &filter_state);

    void restore_state(const QByteArray &state);
    void restore_filter_state(const QByteArray &filter_state);

    QByteArray save_state() const;
    QByteArray save_filter_state() const;

    void show();
    void clear();
    void reset_current();

    void update_trace_filter();

    void set_menu(QMenu *trace_menu, QMenu *help_menu);
    void set_common_actions(QAction *capture, QAction *capture_filter, QAction *clear);

private slots:
    void filter_item_activated(const QModelIndex &index);

    void update_current(const trace_message_t *message);
    void update_preview();

    void update_activated();
    void update_subtrace_model();

    void callstack_current_changed(const QModelIndex &current, const QModelIndex &prev);
    void callstack_activated(const QModelIndex &index);

    void update_duration(uint64_t time);

    void update_locator_pos();
    void update_search_completer_pos();
    void update_trace_indicators();
    void update_transmitter_filter();

    void update_issue_label();
    void issue_selected(const QModelIndex &index);
    void issue_activated(const QModelIndex &index);

    void search();
    void search_by();
    void search_by_index(const QModelIndex &index);
    void search_by_filter(const FilterItem &filter, bool find_next = true);
    void update_search();
    void cancel_search();

    void find_next();
    void find_prev();

    void update_code_browser(const trace_message_t *message);

    void subfilter_by_current_item();
    void subfilter_add_current_item();

    void invoke_on_current_table(table_method_t method);
    void profile_current_table();
    void open_image_browser();

    void select_next_table();
    void set_current_table(TraceTableView *table);

signals:
    void capture_filter_triggered();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void keyPressEvent(QKeyEvent *e);

private:
    TraceTableModel * make_table_model();

private:
    Ui::TraceViewWidget *ui;
    QMenuBar *_menu_bar;

    //

    TraceController *_trace_controller;

    TraceTableModel *_trace_table_model;
    TraceTableModel *_subtrace_table_model;

    //

    ExtraMessageModel *_extra_model;

    TraceCompleter *_trace_locator;
    TraceCompleter *_search_completer;

    TraceDataModel _call_stack;
    CallstackModel *_call_stack_model;
    TraceTableView *_current_table;
    CodeBrowser *_code_browser;

    uint64_t _current_duration;

    //

    ModelTreeView *_model_view;

    GridTreeView *_extra_tree_view;
    TableView *_callstack_view;
    ListView *_issues_view;

    QStackedWidget *_preview_dock;

    QTextBrowser *_text_preview;

    ImageView *_image_preview;
    ImageScene *_image_preview_scene;

    PanelManager _panel_manager;

    FilterItem _current_search_filter;

    QModelIndex _current_table_index;
    const trace_message_t *_current_message;

    QRect _bottom_panel_geometry;
};

#endif // TRACE_VIEW_WIDGET_H
