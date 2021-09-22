#include "filter_tree_view.h"

#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QMenu>
#include <QActionGroup>
#include <QLabel>
#include <QClipboard>
#include <QApplication>

#include "trace_x/trace_x.h"

FilterTreeView::FilterTreeView(QWidget *parent):
    BaseTreeView(parent),
    _model(0)
{
    X_CALL;
}

void FilterTreeView::set_model(FilterModel *model)
{
    X_CALL;

    _model = model;

    setModel(model);
}

void FilterTreeView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QTreeView::drawBranches(painter, rect, index);

    if(index.data(BranchCharRole).isValid())
    {
        QRect char_rect = rect;

        char_rect.setLeft(char_rect.right() - char_rect.height());

        QFont font = painter->font();

        painter->setFont(font);
        painter->drawText(char_rect, Qt::AlignCenter, index.data(BranchCharRole).toString());
    }
}

void FilterTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    X_CALL;

    if(!model())
        return;

    QStandardItem *current_item = _model->itemFromIndex(this->currentIndex());

    if(current_item)
    {
        emit before_context_menu();

        QMenu context_menu;

       // context_menu.addAction(tr("Add new item ..."));

        QAction *remove_action = context_menu.addAction(tr("Remove"));

        connect(remove_action, &QAction::triggered, this, &FilterTreeView::remove_current, Qt::QueuedConnection);

        if(current_item->type() == FilterGroupItemType)
        {
            FilterGroup *item = static_cast<FilterGroup*>(current_item);

            QActionGroup operation_type_group(&context_menu);
            operation_type_group.setExclusive(true);

            QAction action_include("Include", &operation_type_group);
            QAction action_exclude("Exclude", &operation_type_group);

            action_include.setData(IncludeOperator);
            action_exclude.setData(ExcludeOperator);

            action_include.setCheckable(true);
            action_exclude.setCheckable(true);

            action_include.setChecked(item->operator_type() == IncludeOperator);
            action_exclude.setChecked(item->operator_type() == ExcludeOperator);

            context_menu.addSeparator();

            context_menu.addAction(&action_include);
            context_menu.addAction(&action_exclude);

            //

//            context_menu.addSeparator();

//            QAction *filter_type = new QAction("Condition", &context_menu);

//            filter_type->setCheckable(true);
//            filter_type->setChecked(item->condition);

//            context_menu.addAction(filter_type);

            //

            QAction *selected_action = context_menu.exec(event->globalPos());

            if(selected_action)
            {
                item->set_filter_type(FilterOperator(operation_type_group.checkedAction()->data().toInt()));
            }
        }
        else
        {
            context_menu.exec(event->globalPos());
        }
    }

    BaseTreeView::contextMenuEvent(event);
}

void FilterTreeView::keyPressEvent(QKeyEvent *event)
{
    X_CALL;

    if(!this->model())
    {
        return;
    }

    if(event->matches(QKeySequence::Delete))
    {
        remove_current();
    }
    else if(event->matches(QKeySequence::Copy))
    {
        qApp->clipboard()->setMimeData(model()->mimeData(this->selectedIndexes()));

        return;
    }
    else if(event->matches(QKeySequence::Paste))
    {
        model()->dropMimeData(qApp->clipboard()->mimeData(), Qt::CopyAction, this->currentIndex().row(), this->currentIndex().column(), this->currentIndex().parent());
    }

    BaseTreeView::keyPressEvent(event);
}

void FilterTreeView::mousePressEvent(QMouseEvent *event)
{
    BaseTreeView::mousePressEvent(event);

    if(!indexAt(event->pos()).isValid())
    {
        clearSelection();
        setCurrentIndex(QModelIndex());
    }
}

void FilterTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTreeView::mouseDoubleClickEvent(event);

    if(!indexAt(event->pos()).isValid())
    {
        emit doubleClicked(QModelIndex());
    }
}

void FilterTreeView::remove_current()
{
    X_CALL;

    if(this->selectedIndexes().isEmpty())
    {
        _model->clear();
    }
    else
    {
        foreach (const QModelIndex &index, this->selectedIndexes())
        {
            this->model()->removeRow(index.row(), index.parent());
        }
    }

    emit this->model()->layoutChanged();
}
