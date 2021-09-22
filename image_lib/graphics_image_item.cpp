#include "graphics_image_item.h"

#include <QPainter>
#include <QGraphicsView>

GraphicsImageItem::GraphicsImageItem(QGraphicsItem *parent):
    QGraphicsObject(parent)
{
}

GraphicsImageItem::~GraphicsImageItem()
{
}

QPainterPath GraphicsImageItem::opaqueArea() const
{
    return shape();
}

void GraphicsImageItem::set_grpahics_image_flag(GraphicsImageItem::GraphicsImageItemFlag flag, bool enable)
{
    if(enable)
    {
        _flags |= flag;
    }
    else
    {
        _flags &= ~flag;
    }

    update();
}

int GraphicsImageItem::type() const
{
    return Type;
}

QRectF GraphicsImageItem::boundingRect() const
{
    return QRectF(QPointF(), _image.size());
}

bool GraphicsImageItem::contains(const QPointF &point) const
{
    return QGraphicsItem::contains(point);
}

void GraphicsImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QSize dest_size = painter->viewport().size();

    if(painter->transform().m31() > 0)
    {
        dest_size.rwidth() -= 2 * painter->transform().m31();
    }

    if(painter->transform().m32() > 0)
    {
        dest_size.rheight() -= 2 * painter->transform().m32();
    }

    QGraphicsView *view = widget ? static_cast<QGraphicsView*>(widget->parentWidget()) : 0;

    QRect src_roi;

    if(view)
    {
        src_roi = view->mapToScene(view->viewport()->rect()).boundingRect().intersected(view->sceneRect()).toRect();
    }
    else
    {
        src_roi = scene()->sceneRect().toRect();
    }

    bool upsampling = (dest_size.width() > src_roi.width()) || (dest_size.height() > src_roi.height());

    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

    if(_flags.testFlag(ItemSmoothUpsampling) && upsampling)
    {
        // upsampling

        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    }
    else if(_flags.testFlag(ItemSmoothDownsampling) && !upsampling)
    {
        // downsampling

        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    }

    painter->drawImage(QPoint(), _image, _image.rect(), Qt::AutoColor);
}

void GraphicsImageItem::set_image(const QImage &image)
{
    _image = image;

    update();
}
