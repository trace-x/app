#include "graphics_marker_item.h"

#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>

GraphicsMarkerItem::GraphicsMarkerItem(QGraphicsItem *parent):
    AbstractSimpleItem(parent)
{
    _flags |= ItemDisplayLabel;
    _flags |= ItemDiscreteGeometry;

    _radius = 8;

    QPen pen(Qt::white, 2);
    pen.setCosmetic(true);

    _pen = pen;
    _brush = Qt::red;
}

QRectF GraphicsMarkerItem::boundingRect() const
{
    return QRectF(-_radius, -_radius, 2 * _radius, 2 * _radius);
}

void GraphicsMarkerItem::setRadius(qreal radius)
{
    prepareGeometryChange();

    _radius = radius;

    updateParams();
}

qreal GraphicsMarkerItem::radius() const
{
    return _radius;
}

void GraphicsMarkerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if(scene()->sceneRect().contains(pos()))
    {
        painter->setRenderHint(QPainter::Antialiasing);

        QPen cpen = _pen;

        if(_highlight)
        {
            cpen.setWidth(3);
        }

        if(_selected) cpen.setColor(Qt::yellow);

        painter->save();

        QTransform t = painter->transform();

        t.scale(1.0 / t.m11(), 1.0 / t.m22());

        painter->setTransform(t);

        painter->setPen(cpen);
        painter->setBrush(_brush);

        painter->drawEllipse(QPointF(0, 0), _radius, _radius);

        painter->restore();

        AbstractSimpleItem::paint(painter, option, widget);
    }
}
