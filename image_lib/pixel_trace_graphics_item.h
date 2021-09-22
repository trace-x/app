#ifndef PIXELTRACEGRAPHICSITEM_H
#define PIXELTRACEGRAPHICSITEM_H

#include <QGraphicsPathItem>

class PixelTraceGraphicsItem : public QGraphicsPathItem
{
public:
    PixelTraceGraphicsItem();

    void set_position(const QPoint &pos);
};

#endif // PIXELTRACEGRAPHICSITEM_H
