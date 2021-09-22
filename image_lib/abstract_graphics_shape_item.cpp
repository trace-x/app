#include "abstract_graphics_shape_item.h"

#include <QKeyEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

AbstractGraphicsShapeItem::AbstractGraphicsShapeItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    _selected = false;

    setFlag(ItemIsFocusable);

    setAcceptHoverEvents(true);

    _flags |= ItemAcceptsKeyboard;

    _keyDelta[Qt::Key_Up]    = QPointF(0, -1);
    _keyDelta[Qt::Key_Down]  = QPointF(0, 1);
    _keyDelta[Qt::Key_Left]  = QPointF(-1, 0);
    _keyDelta[Qt::Key_Right] = QPointF(1, 0);
}

void AbstractGraphicsShapeItem::setPos(const QPointF &pos)
{
    setPosition(pos);
}

void AbstractGraphicsShapeItem::move(const QPointF &point, qreal sx)
{
    Q_UNUSED(sx);

    if (pos() == point)
        return;

    setPos(point);

    scene()->invalidate();

    emit positionChanged(point);
}

void AbstractGraphicsShapeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit mousePress();

    if(event->button() == Qt::LeftButton)
    {
        if(_flags.testFlag(ItemSelectOnClick))
        {
            _selected = true;
        }

        _captured = true;

        _captureMousePoint = event->scenePos();
        _capturePos = scenePos();
    }
}

void AbstractGraphicsShapeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    _captured = false;

    foreach (QGraphicsItem *other, collidingItems(Qt::IntersectsItemShape))
    {
        if(!other->isAncestorOf(this) && (other->parentItem() == this->parentItem()))
        {
            if(other->collidesWithItem(this, Qt::ContainsItemShape))
            {
                this->stackBefore(other);
            }
            else
            {
                other->stackBefore(this);
            }
        }
    }

    emit mouseRelease();
}

void AbstractGraphicsShapeItem::keyPressEvent(QKeyEvent *event)
{
    emit keyPress(event);

    bool blockEvent = false;

    if(_flags.testFlag(ItemAcceptsKeyboard) && _selected)
    {
        QPointF delta = _keyDelta.value(event->key());

        if(!delta.isNull())
        {
            blockEvent = true;

            if(!_keyState[event->key()])
            {
                _keyState[event->key()] = true;

                _keyAD += delta;
            }

            move(pos() + _keyAD);

            emit moved(pos());
        }
    }

    if(!blockEvent) QGraphicsObject::keyPressEvent(event);
}

void AbstractGraphicsShapeItem::keyReleaseEvent(QKeyEvent *event)
{
    bool blockEvent = false;

    if(_flags.testFlag(ItemAcceptsKeyboard) && _selected)
    {
        blockEvent = true;

        QPointF delta = _keyDelta.value(event->key());

        _keyAD -= delta;

        if(!delta.isNull()) _keyState[event->key()] = false;
    }

    if(!blockEvent) QGraphicsObject::keyReleaseEvent(event);
}

void AbstractGraphicsShapeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    _highlight = true;

    update();

    QGraphicsObject::hoverEnterEvent(event);
}

void AbstractGraphicsShapeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    _highlight = false;

    update();

    QGraphicsObject::hoverLeaveEvent(event);
}

QVariant AbstractGraphicsShapeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemVisibleChange)
    {
        _highlight = isUnderMouse();

        update();
    }

    return QGraphicsObject::itemChange(change, value);
}

void AbstractGraphicsShapeItem::setSelected(bool selected)
{
    _selected = selected;

    update();
}

bool AbstractGraphicsShapeItem::isSelected() const
{
    return _selected;
}
