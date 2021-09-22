#include "abstract_simple_item.h"

#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

AbstractSimpleItem::AbstractSimpleItem(QGraphicsItem *parent):
    AbstractGraphicsShapeItem(parent)
{
}

void AbstractSimpleItem::updateParams()
{
    if(!scene()) return;

    QSizeF lsize = QFontMetricsF(_labelFont).size(0, _labelPaintText);
    QPointF topLeft = QPointF(-lsize.width() / 2.0, -lsize.height() - boundingRect().height() / 2.0 - 2);

    _labelRect = QRectF(topLeft, lsize);

    update();
}

void AbstractSimpleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(_captured)
    {
        QPointF newPos = _capturePos + event->scenePos() - _captureMousePoint;

        qreal sx = 0;

        if(!_flags.testFlag(ItemDiscreteGeometry))
        {
            if(QGraphicsView *view = static_cast<QGraphicsView*>(event->widget()->parentWidget()))
            {
                sx = view->transform().m11();
            }
        }

        move(newPos, sx);

        emit moved(pos());

        return;
    }

    AbstractGraphicsShapeItem::mouseMoveEvent(event);
}

void AbstractSimpleItem::move(const QPointF &point, qreal sx)
{
    QPointF newPos = point;

    if(scene())
    {
        if(newPos.x() < 0) newPos.setX(0);
        if(newPos.y() < 0) newPos.setY(0);
        if(newPos.x() >= scene()->width()) newPos.setX(scene()->width());
        if(newPos.y() >= scene()->height()) newPos.setY(scene()->height());
    }

    _displayPos = newPos;

    if((sx > 0) && (sx < 14.0))
    {
        _displayPos = QPointF(int(newPos.x()), int(newPos.y()));
        newPos = _displayPos;
    }

    if(_flags.testFlag(ItemDiscreteGeometry))
    {
        _displayPos = QPointF(int(newPos.x()), int(newPos.y()));
        newPos = QPointF(int(newPos.x()) + 0.5, int(newPos.y()) + 0.5);
    }

    if (pos() == newPos)
        return;

    if(_flags.testFlag(ItemDisplayRealCoordinates))
    {
        _labelPaintText = QString("%1\n(%2;%3)").arg(_labelText).arg(_displayPos.x()).arg(_displayPos.y());

        updateParams();
    }

    QGraphicsObject::setPos(newPos);

    if(scene()) scene()->invalidate();

    emit positionChanged(_displayPos);
}

void AbstractSimpleItem::setPosition(const QPointF &point)
{
    if (pos() == point)
        return;

    _displayPos = point;

    QGraphicsObject::setPos(point);

    if(scene()) scene()->invalidate();

    emit positionChanged(_displayPos);
}

void AbstractSimpleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if(!_flags.testFlag(ItemDisplayLabel)) return;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(QBrush(QColor(0, 0, 0, 150)));
    painter->setPen(Qt::NoPen);

    QTransform ct = painter->combinedTransform();
    QPointF tp = QPoint(ct.m31(), ct.m32());
    QRectF vr = painter->viewport();

    QRectF labelRect = _labelRect;

    qreal dleft = tp.x() + labelRect.left();
    qreal dtop = tp.y() + labelRect.top();
    qreal dright = vr.width() - (tp.x() - labelRect.left());

    if(dleft < 0) labelRect.moveLeft(labelRect.left() - dleft);
    if(dright < 0) labelRect.moveLeft(labelRect.left() + dright);
    if(dtop < 0) labelRect.moveTop(boundingRect().height() / 2);

    painter->drawRect(labelRect);

    painter->setPen(QPen(Qt::white, 1));

    if(_selected) painter->setPen(QPen(Qt::yellow, 1));

    painter->setFont(_labelFont);
    painter->drawText(labelRect, Qt::AlignCenter, _labelPaintText);
}

QVariant AbstractSimpleItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemVisibleChange)
    {
        updateParams();
    }

    return AbstractGraphicsShapeItem::itemChange(change, value);
}

void AbstractSimpleItem::setLabelText(const QString &text)
{
    _labelText = text;

    if(_flags.testFlag(ItemDisplayRealCoordinates))
    {
        _labelPaintText = QString("%1\n(%2;%3)").arg(_labelText).arg(_displayPos.x()).arg(_displayPos.y());
    }
    else
    {
        _labelPaintText = text;
    }

    updateParams();
}

void AbstractSimpleItem::setLabelFont(const QFont &font)
{
    _labelFont = font;

    updateParams();
}

QString AbstractSimpleItem::labelText() const
{
    return _labelText;
}
