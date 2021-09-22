#include "session_manager.h"
#include "ui_session_manager.h"

#include "common_ui_tools.h"
#include "settings.h"
#include "text_input_dialog.h"
#include "trace_x/trace_x.h"

#include <QStringListModel>
#include <QInputDialog>

class SessionListModel : public QStringListModel
{
public:
    SessionListModel(QObject *parent = 0):
        QStringListModel(parent),
        current_session(0)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        return session_list.size();
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if(index.row() >= session_list.size())
        {
            return QVariant();
        }

        if((role == Qt::DisplayRole) || (role == Qt::ToolTipRole))
        {
            return session_list[index.row()].name;
        }

        if((index.row() == current_session) && (role == Qt::FontRole))
        {
            QFont font;

            font.setBold(true);

            return font;
        }

        return QStringListModel::data(index, role);
    }

    QList<Session> session_list;

    int current_session;
};

SessionManager::SessionManager(QWidget *menu_parent, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SessionManager),
    _list_model(new SessionListModel(this)),
    _session_menu(new QMenu(tr("Sessions"), menu_parent)),
    _session_group(this)
{
    X_CALL;

    ui->setupUi(this);

    ui->list_view->setItemDelegate(new TreeItemDelegate(this));

    ui->list_view->setModel(_list_model);

    connect(ui->list_view, &ListView::activated_ex, this, &SessionManager::session_activated);
    connect(ui->list_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &SessionManager::selected_changed);

    connect(ui->new_session_button, &QPushButton::clicked, this, &SessionManager::add_session);
    connect(ui->delete_button, &QPushButton::clicked, this, &SessionManager::remove_session);
    connect(ui->clone_button, &QPushButton::clicked, this, &SessionManager::clone_session);
    connect(ui->rename_button, &QPushButton::clicked, this, &SessionManager::rename_session);

    connect(ui->switch_button, &QPushButton::clicked, this, [this] { session_activated(ui->list_view->currentIndex()); });
}

SessionManager::~SessionManager()
{
    delete ui;
}

QByteArray SessionManager::save_state()
{
    X_CALL;

    QByteArray state;

    QDataStream stream(&state, QIODevice::WriteOnly);

    save_current_session();

    stream << _list_model->session_list;
    stream << _list_model->current_session;

    return state;
}

void SessionManager::restore_state(const QByteArray &state)
{
    X_CALL;

    emit _list_model->layoutAboutToBeChanged();

    _list_model->session_list << Session(tr("Default"));

    if(!state.isEmpty())
    {
        DataStream stream(state);

        stream >> _list_model->session_list;
        stream >> _list_model->current_session;
    }

    ui->list_view->setCurrentIndex(_list_model->index(_list_model->current_session));

    update_session_list();

    emit _list_model->layoutChanged();
}

const Session &SessionManager::current_session() const
{
    return _list_model->session_list[_list_model->current_session];
}

QMenu *SessionManager::session_menu()
{
    return _session_menu;
}

void SessionManager::add_session()
{
    X_CALL;

    TextInputDialog input_dialog(tr("New Session Name"), tr("Enter the name of the session"), "", this);

    if((input_dialog.exec() == QDialog::Accepted) && !input_dialog.text().isEmpty())
    {
        _list_model->session_list << Session(input_dialog.text());

        update_session_list();

        session_activated(_list_model->index(_list_model->session_list.size() - 1));
    }
}

void SessionManager::clone_session()
{
    X_CALL;

    save_current_session();

    Session selected_session = _list_model->session_list[ui->list_view->currentIndex().row()];

    TextInputDialog input_dialog(tr("Cloned Session Name"), tr("Enter the name of the session"), selected_session.name, this);

    if((input_dialog.exec() == QDialog::Accepted) && !input_dialog.text().isEmpty())
    {
        selected_session.name = input_dialog.text();

        _list_model->session_list << selected_session;

        update_session_list();

        session_activated(_list_model->index(_list_model->session_list.size() - 1));
    }
}

void SessionManager::remove_session()
{
    X_CALL;

    int index = ui->list_view->currentIndex().row();

    _list_model->removeRow(index);
    _list_model->session_list.removeAt(index);

    if(_list_model->current_session > index)
    {
        _list_model->current_session--;
    }

    emit _list_model->layoutChanged();

    selected_changed(ui->list_view->currentIndex(), ui->list_view->currentIndex());

    update_session_list();
}

void SessionManager::rename_session()
{
    X_CALL;

    Session &selected_session = _list_model->session_list[ui->list_view->currentIndex().row()];

    TextInputDialog input_dialog(tr("Rename Session"), tr("Enter the name of the session"), selected_session.name, this);

    if((input_dialog.exec() == QDialog::Accepted) && !input_dialog.text().isEmpty())
    {
        selected_session.name = input_dialog.text();

        emit _list_model->layoutChanged();

        update_session_list();

        if(ui->list_view->currentIndex().row() == _list_model->current_session)
        {
            emit session_name_changed();
        }
    }
}

void SessionManager::session_activated(const QModelIndex &index)
{
    X_CALL;

    if(index.isValid())
    {
        save_current_session();

        ui->list_view->setCurrentIndex(index);

        _list_model->current_session = index.row();

        _session_menu->actions().at(index.row())->setChecked(true);

        selected_changed(index, index);

        emit _list_model->layoutChanged();

        emit session_changed(current_session());
    }
}

void SessionManager::selected_changed(const QModelIndex &current, const QModelIndex &)
{
    X_CALL;

    ui->delete_button->setDisabled((current.row() == 0) || (current.row() == _list_model->current_session));
    ui->switch_button->setDisabled(current.row() == _list_model->current_session);
    ui->rename_button->setDisabled(current.row() == 0);
}

void SessionManager::save_current_session()
{
    X_CALL;

    Session &current_session = _list_model->session_list[_list_model->current_session];

    emit request_session_settings(current_session);
}

void SessionManager::update_session_list()
{
    X_CALL;

    _session_menu->clear();

    foreach (const Session &session, _list_model->session_list)
    {
        QAction *action = _session_menu->addAction(session.name, this, SLOT(action_activated()));

        action->setCheckable(true);

        _session_group.addAction(action);
    }

    _session_menu->actions().at(_list_model->current_session)->setChecked(true);
}

void SessionManager::action_activated()
{
    X_CALL;

    int session_index = _session_menu->actions().indexOf(static_cast<QAction*>(sender()));

    session_activated(_list_model->index(session_index));
}

QDataStream &operator <<(QDataStream &out, const Session &value)
{
    X_CALL_F;

    out << value.name << value.capture_filter << value.trace_filter << value.main_view_filter;

    return out;
}

QDataStream &operator >>(QDataStream &in, Session &value)
{
    X_CALL_F;

    in >> value.name >> value.capture_filter >> value.trace_filter >> value.main_view_filter;

    return in;
}
