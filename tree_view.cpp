#include "tree_view.h"

#include <QKeySequence>
#include <QKeyEvent>
#include <QHeaderView>

BaseTreeView::BaseTreeView(QWidget *parent) :
    QTreeView(parent)
{
    connect(this, &QAbstractItemView::doubleClicked, this, &BaseTreeView::activated_ex);
}

void BaseTreeView::set_logo(const QPixmap &pixmap)
{
    _logo_pixmap = pixmap;

    repaint();
}

void BaseTreeView::keyPressEvent(QKeyEvent *event)
{
    if(model())
    {
        if(event->matches(QKeySequence::Find))
        {
            event->accept();

            emit find();

            return;
        }
        else if((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return))
        {
            emit activated_ex(currentIndex());
        }
    }

    QTreeView::keyPressEvent(event);
}

void BaseTreeView::paintEvent(QPaintEvent *event)
{
    QTreeView::paintEvent(event);

    if(!_logo_pixmap.isNull() && model() && !model()->hasChildren())
    {
        QPainter painter;
        painter.begin(viewport());
        painter.save();

        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        double scale_factor = 1.0;

        if((_logo_pixmap.height() + 20) > viewport()->height())
        {
            scale_factor = (viewport()->height()) / double(_logo_pixmap.height() + 20);
        }

        QRect dest = QRect(QPoint(), _logo_pixmap.rect().size() * scale_factor);

        dest.moveCenter(viewport()->geometry().center());

        painter.drawPixmap(dest, _logo_pixmap);

        painter.restore();
        painter.end();
    }
}

// ==========================================================

GridTreeView::GridTreeView(QWidget *parent) :
    BaseTreeView(parent)
{
    header()->setVisible(false);

    setRootIsDecorated(false);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setDragDropMode(QAbstractItemView::DragOnly);
}

void GridTreeView::paintEvent(QPaintEvent *event)
{
    BaseTreeView::paintEvent(event);

    QPainter painter;
    painter.begin(viewport());
    painter.save();

    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);

    //вертикальная линия между столбцами
    QLine line(columnWidth(0), 0, columnWidth(0), this->rowHeight(this->model()->index(0, 0)) * this->model()->rowCount() - 1);

    painter.drawLine(line);

    painter.restore();
    painter.end();
}

void GridTreeView::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    QTreeView::drawRow(painter, options, index);

    painter->save();

    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter->setPen(pen);

    // горизонтальная линия между полями
    QLine line(options.rect.left(), options.rect.bottom(),
               options.rect.right(), options.rect.bottom());

    painter->drawLine(line);

    painter->restore();
}

