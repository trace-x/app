#ifndef COMPLETER_H
#define COMPLETER_H

#include <QObject>

#include <QLineEdit>

#include <QStandardItemModel>
#include <QTreeView>

#include "trace_model.h"
#include "trace_model_service.h"

class CompletionList;

//! Абстрактный класс автодополнителя пользовательского ввода в текстовое поле QLineEdit.
//! Отслеживает ввод и показывает окно автодополнителя рядом с текстовым полем.
class Completer : public QObject
{
    Q_OBJECT

public:
    Completer(LineEdit *line_edit, QWidget *completer_parent, bool manual_mode = false, QObject *parent = 0);

    BaseTreeView * completer_view() const;

    void set_grip_enabled(bool enable);
    void set_grip_corner(Qt::Corner corner);

    void set_always_visible(bool enable);

protected:
    bool eventFilter(QObject *object, QEvent *event);

    virtual void process_text(const QString &text);
    virtual void accept() = 0;

signals:
    void entry_accepted(const QModelIndex &index);
    void completer_shows();
    void text_accepted();
    void find();

private:
    void show_completer();
    void update_action();
    void hide_completer(bool clear_focus = true);

protected:
    LineEdit *_line_edit;
    CompletionList *_completer_view;
    bool _always_visible;
    bool _manual_mode;

    QAction *_close_action;
    QAction *_clear_action;
};

//! Класс, специализирующий поиск автодополнителя по элементам модели трассы
class TraceCompleter : public Completer
{
    Q_OBJECT

public:
    TraceCompleter(const QList<EntityClass> &class_list, TraceController *trace_controller,
                   LineEdit *line_edit, QWidget *completer_parent, bool manual_mode, QObject *parent = 0);

    void set_default_filters(const QSet<int> &default_filters);
    void set_current_class(EntityClass class_id, const QString &pre_id = QString());

    void clear();

    QString current_text() const;
    int current_class() const;

    FilterItem current_filter() const;

    QModelIndex current_index() const;
    const QSet<int> & default_filters() const;

    virtual void process_text(const QString &text);
    virtual void accept();

signals:
    void activated();

private:
    void find_by_class(const QString &prefix, const QString &pattern);
    void switch_to_help();
    void resize_columns();

    void find_message(const QRegExp &regexp);
    void find_message_finished();

protected:
    TraceController *_controller;

    QStandardItemModel _help_model;
    EntityStandardItemModel _entry_search_model;
    ListModel _message_search_model;
    QIdentityProxyModel _search_model;
    QIdentityProxyModel _view_model;

    EntrySet _entry_set;
    QHash<int, QString> _prefix_hash;

    bool _help_mode;

    QFutureWatcher<void> _search_watcher;
    boost::atomic<bool> _search_break_flag;

    QString _current_text;
    int _current_class;

    QSet<int> _default_filter_set;
};

#endif // COMPLETER_H
