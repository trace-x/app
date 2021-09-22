#ifndef TREE_VIEW_H
#define TREE_VIEW_H

#include <QTreeView>
#include <QPainter>

//! Base class for tree view with support drawing logos and more
class BaseTreeView : public QTreeView
{
    Q_OBJECT

public:
    BaseTreeView(QWidget *parent = 0);

    void set_logo(const QPixmap &pixmap);

signals:
    void find();
    void activated_ex(const QModelIndex &index);

protected:
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *);

private:
    QPixmap _logo_pixmap;
};

//! Tree view with grid lines
class GridTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    GridTreeView(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

    void drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const;
};

/*
 *
void MyDelegate::paint(QPainter *painter,
  const QStyleOptionViewItem &option,
  const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);



    painter->save();

    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::DotLine);
    pen.setWidth(2);
    painter->setPen(pen);

    QTreeView *tree = dynamic_cast<QTreeView*>(parent()); //?!!

    QRect rc(option.rect);
    if (index.column() == 0)
        rc.setLeft(-tree->horizontalScrollBar()->value());

    QLine lines[2] = {
        QLine(rc.left(), rc.bottom(), rc.right(), rc.bottom()), // горизонтальная линия снизу
        QLine(rc.right(), rc.top(), rc.right(), rc.bottom())    // верт. линия справа
    };
    painter->drawLines(&lines[0], index.column() == 0 ? 2 : 1);
    painter->restore();
}
 *
 *
 * */
#endif // TREE_VIEW_H
