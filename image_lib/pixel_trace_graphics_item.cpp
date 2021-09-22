#include "pixel_trace_graphics_item.h"

#include <QPen>
#include <QGraphicsScene>

PixelTraceGraphicsItem::PixelTraceGraphicsItem()
{
    QPen pen(QBrush(Qt::red), 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);

    pen.setCosmetic(true);

    setPen(pen);
}

void PixelTraceGraphicsItem::set_position(const QPoint &pos)
{
    if(scene())
    {
        QPoint new_pos = pos;

        new_pos = pos;

        if(new_pos.x() < 0) new_pos.setX(0);
        if(new_pos.y() < 0) new_pos.setY(0);
        if(new_pos.x() >= scene()->width()) new_pos.setX(scene()->width() - 1);
        if(new_pos.y() >= scene()->height()) new_pos.setY(scene()->height() - 1);

        QPainterPath path;

        path.moveTo(new_pos.x() + 0.5, 0);
        path.lineTo(new_pos.x() + 0.5, new_pos.y());

        path.moveTo(new_pos.x() + 0.5, new_pos.y() + 1);
        path.lineTo(new_pos.x() + 0.5, scene()->height());

        path.moveTo(0, new_pos.y() + 0.5);
        path.lineTo(new_pos.x(), new_pos.y() + 0.5);

        path.moveTo(new_pos.x() + 1, new_pos.y() + 0.5);
        path.lineTo(scene()->width(), new_pos.y() + 0.5);

        path.addRect(new_pos.x(), new_pos.y(), 1, 1);

        path.moveTo(0, 0);
        path.lineTo(0, scene()->height());

        path.moveTo(scene()->width(), 0);
        path.lineTo(scene()->width(), scene()->height());

        setPath(path);
    }
}

