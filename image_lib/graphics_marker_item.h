#ifndef GRAPHICS_MARKER_ITEM_H
#define GRAPHICS_MARKER_ITEM_H

#include <QFont>
#include <QPen>
#include <QBrush>

#include "abstract_simple_item.h"

class GraphicsMarkerItem : public AbstractSimpleItem
{
    Q_OBJECT

public:
    explicit GraphicsMarkerItem(QGraphicsItem *parent = 0);

public:
    void setRadius(qreal radius);
    qreal radius() const;

public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    QRectF boundingRect() const;

private:
    qreal _radius;

private:
    Q_DISABLE_COPY(GraphicsMarkerItem)

    QPen _pen;
    QBrush _brush;
};

#endif // GRAPHICS_MARKER_ITEM_H
