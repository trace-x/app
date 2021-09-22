#ifndef PANEL_MANAGER_H
#define PANEL_MANAGER_H

#include <QObject>
#include <QSplitter>
#include <QMenu>
#include <QFrame>

#include "panel_container.h"
#include "panel_layout_settings.h"

class DockWidget : public QSplitter
{
    Q_OBJECT

public:
    explicit DockWidget(Qt::Orientation orientation, QWidget* parent = 0);

    void add_container(PanelContainer *container);

private:
    bool event(QEvent *event);
    void update_state();

signals:
    void visibility_changed(bool is_visible);
    void has_childs_changed(bool has_child);
};

//! Class for dock panels managment(layout, order, menu)

class PanelManager : public QObject
{
    Q_OBJECT
public:
    explicit PanelManager(QObject *parent = 0);

    PanelContainer * add_widget(QWidget *widget, Qt::Edge default_edge);

    DockWidget *left_container();
    DockWidget *right_container();
    DockWidget *bottom_container();

    QMenu *panel_menu();
    QMenu *panel_menu(QWidget *parent);

    QByteArray save_state() const;
    void restore_state(const QByteArray &state_data);

private:
    DockWidget *_left_container;
    DockWidget *_right_container;
    DockWidget *_bottom_container;

    QList<PanelContainer*> _widgets;

    QMenu *_menu;
};

#endif // PANELMANAGER_H
