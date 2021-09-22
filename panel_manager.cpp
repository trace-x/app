#include "panel_manager.h"

#include "trace_x/trace_x.h"
#include "settings.h"

DockWidget::DockWidget(Qt::Orientation orientation, QWidget *parent):
    QSplitter(orientation, parent)
{
}

void DockWidget::add_container(PanelContainer *container)
{
    addWidget(container);

    connect(container, &PanelContainer::visibility_changed, this, &DockWidget::update_state);
}

bool DockWidget::event(QEvent *event)
{
    if(event->type() == QEvent::HideToParent)
    {
        emit visibility_changed(false);
    }
    else if(event->type() == QEvent::ShowToParent)
    {
        emit visibility_changed(true);
    }

    return QSplitter::event(event);
}

void DockWidget::update_state()
{
    X_CALL;

    bool has_visible_child = false;

    for(int i = 0; i < count(); ++i)
    {
        has_visible_child = widget(i)->isVisibleTo(this);

        if(has_visible_child)
        {
            break;
        }
    }

    setVisible(has_visible_child);
}

///////////////////

PanelManager::PanelManager(QObject *parent) : QObject(parent),
    _left_container(new DockWidget(Qt::Vertical)),
    _right_container(new DockWidget(Qt::Vertical)),
    _bottom_container(new DockWidget(Qt::Horizontal))
{
    X_CALL;

    _menu = new QMenu(tr("Views"));

    _left_container->setHandleWidth(1);
    _left_container->setChildrenCollapsible(false);

    _right_container->setHandleWidth(1);
    _right_container->setChildrenCollapsible(false);

    _bottom_container->setHandleWidth(1);
    _bottom_container->setChildrenCollapsible(false);
}

PanelContainer * PanelManager::add_widget(QWidget *widget, Qt::Edge default_edge)
{
    X_CALL;

    PanelContainer *container = new PanelContainer(widget);

    _widgets.append(container);

    if(default_edge == Qt::LeftEdge)
    {
        _left_container->add_container(container);
    }
    else if(default_edge == Qt::RightEdge)
    {
        _right_container->add_container(container);
    }
    else if(default_edge == Qt::BottomEdge)
    {
        _bottom_container->add_container(container);
    }

    QAction *hide_show_action = _menu->addAction(widget->windowTitle());

    hide_show_action->setCheckable(true);
    hide_show_action->setChecked(true);

    connect(hide_show_action, &QAction::triggered, container, &PanelContainer::setVisible);
    connect(container, &PanelContainer::dock_changed, hide_show_action, &QAction::setEnabled);

    return container;
}

DockWidget *PanelManager::left_container()
{
    return _left_container;
}

DockWidget *PanelManager::right_container()
{
    return _right_container;
}

DockWidget *PanelManager::bottom_container()
{
    return _bottom_container;
}

QMenu *PanelManager::panel_menu()
{
    return _menu;
}

QMenu *PanelManager::panel_menu(QWidget *parent)
{
    QMenu *menu = new QMenu(_menu->title(), parent);

    menu->addActions(_menu->actions());

    return menu;
}

QByteArray PanelManager::save_state() const
{
    X_CALL;

    QByteArray state_array;

    QDataStream stream(&state_array, QIODevice::WriteOnly);

    stream << _left_container->saveState();
    stream << _right_container->saveState();
    stream << _bottom_container->saveState();

    QList<bool> view_states;

    foreach (QAction *action, _menu->actions())
    {
        view_states << action->isChecked();
    }

    stream << view_states;

    foreach (PanelContainer *panel, _widgets)
    {
        stream << panel->save_state();
    }

    return state_array;
}

void PanelManager::restore_state(const QByteArray &state_data)
{
    X_CALL;

    QList<bool> view_states;

    if(!state_data.isEmpty())
    {
        DataStream stream(state_data);

        _left_container->restoreState(stream.read_next_array());
        _right_container->restoreState(stream.read_next_array());
        _bottom_container->restoreState(stream.read_next_array());

        stream >> view_states;

        foreach (PanelContainer *panel, _widgets)
        {
            panel->restore_state(stream.read_next_array());
        }
    }

    int index = 0;

    foreach (QAction *action, _menu->actions())
    {
        action->setChecked(view_states.isEmpty() ? true : view_states[index]);

        _widgets[index]->setVisible(action->isChecked());

        index++;
    }
}
