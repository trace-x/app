#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <QDialog>
#include <QMenu>

#include "trace_controller.h"
#include "trace_view_widget.h"

namespace Ui {
class SessionManager;
}

struct Session
{
    Session() {}

    Session(const QString &_name) :
        name(_name)
    {}

    QString name;

    QByteArray capture_filter;
    QByteArray trace_filter;
    QByteArray main_view_filter;
};

QDataStream & operator << (QDataStream &out, const Session &value);
QDataStream & operator >> (QDataStream &in, Session &value);

class SessionListModel;

class SessionManager : public QDialog
{
    Q_OBJECT
    
public:
    SessionManager(QWidget *menu_parent, QWidget *parent = 0);
    ~SessionManager();
    
    QByteArray save_state();

    void restore_state(const QByteArray &data);

    const Session &current_session() const;

    QMenu * session_menu();

signals:
    void session_changed(const Session &session);
    void session_name_changed();
    void request_session_settings(Session &session);

private:
    void add_session();
    void clone_session();
    void remove_session();
    void rename_session();

    void session_activated(const QModelIndex &index);
    void selected_changed(const QModelIndex &current, const QModelIndex &previous);

    void save_current_session();
    void update_session_list();

private slots:
    void action_activated();

private:
    Ui::SessionManager *ui;
    SessionListModel *_list_model;
    TraceController *_controller;
    TraceViewWidget *_main_trace_view;

    QActionGroup _session_group;
    QMenu *_session_menu;
};

#endif // SESSION_MANAGER_H
