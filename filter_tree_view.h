#ifndef FILTER_TREE_VIEW_H
#define FILTER_TREE_VIEW_H

#include <QTreeView>

#include "trace_model.h"
#include "filter_model.h"
#include "tree_view.h"

class FilterTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    explicit FilterTreeView(QWidget *parent = 0);

    void set_model(FilterModel *model);

signals:
    void before_context_menu();

public:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;
    void contextMenuEvent(QContextMenuEvent *event);
    void remove_current();

protected:
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    FilterModel *_model;
};

#endif // FILTER_TREE_VIEW_H
