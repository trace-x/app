#include "graphics_rect_item.h"

#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QStyle>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOption>
#include <QKeyEvent>
#include <QtCore/qmath.h>
#include <QGraphicsView>
#include <QApplication>
#include <QScreen>

#define TRACE_POINT_F(p) TRACE_INFO(#p" (%f, %f)", p.x(), p.y());
#define TRACE_POINT(p) TRACE_INFO(#p" (%d, %d)", p.x(), p.y());

#include <QTouchDevice>

static QPainterPath qt_graphicsItem_shapeFromPath(const QPainterPath &path, const QPen &pen)
{
    const qreal penWidthZero = qreal(0.00000001);

    if (path == QPainterPath())
        return path;
    QPainterPathStroker ps;
    ps.setCapStyle(pen.capStyle());
    if (pen.widthF() <= 0.0)
        ps.setWidth(penWidthZero);
    else
        ps.setWidth(pen.widthF());
    ps.setJoinStyle(pen.joinStyle());
    ps.setMiterLimit(pen.miterLimit());
    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    return p;
}

GraphicsRectItem::GraphicsRectItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    initialize();
}

GraphicsRectItem::GraphicsRectItem(const QRect &rect, QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    initialize();

    setGeometry(rect);
}

GraphicsRectItem::GraphicsRectItem(int x, int y, int w, int h, QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    initialize();

    setGeometry(QRect(x, y, w, h));
}

GraphicsRectItem::~GraphicsRectItem()
{
}

void GraphicsRectItem::initialize()
{  
    _alert_counter = 0;

    _show_touch_regions = !QTouchDevice::devices().isEmpty();

    connect(&_timer, &QTimer::timeout, this, &GraphicsRectItem::alert_1);

    _capturedRect = 0;

    _last_scale = -1.0;

    _corners.resize(4);

    int pwBase = _show_touch_regions ? 2 : 1;

    _masterPen = QPen(QBrush(QColor(Qt::green)), pwBase, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    _masterPen.setCosmetic(true);

    _slavePen = QPen(QBrush(QColor(Qt::black)), pwBase + 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    _slavePen.setCosmetic(true);

    _highlightPen = QPen(QBrush(QColor(Qt::green)), pwBase + 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    _highlightPen.setCosmetic(true);

    _selectionPen = QPen(QBrush(QColor(Qt::yellow)), pwBase, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    _selectionPen.setCosmetic(true);

    _selectionHiglightPen = QPen(QBrush(QColor(Qt::yellow)), pwBase + 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    _selectionHiglightPen.setCosmetic(true);

    _pen1 = _slavePen;
    _pen2 = _masterPen;

    _keyDelta[Qt::Key_Up] = QPoint(0, -1);
    _keyDelta[Qt::Key_Down] = QPoint(0, 1);
    _keyDelta[Qt::Key_Left] = QPoint(-1, 0);
    _keyDelta[Qt::Key_Right] = QPoint(1, 0);

    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);

    setFlag(ItemIsFocusable);

    _flags |= ItemAcceptsKeyboard;
    _flags |= ItemAcceptsMouseCapture;
    _flags |= ItemSelectOnClick;

    _keyDx = 0;
    _keyDy = 0;

    _highlight = false;
    _selected = false;
    _captured = false;

    if(_show_touch_regions)
    {
        QScreen * screen = QApplication::screens().first();

        _inner_pad = 80;
        _outer_pad = 0;

        if(screen)
        {
            _inner_pad = screen->physicalDotsPerInch() / 3.0;
        }
    }
    else
    {
        _inner_pad = 3;
        _outer_pad = 3;
    }

    _bounding_pad = _outer_pad;

    _minimumSize = QSize(1, 1);
    _maximumSize = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void GraphicsRectItem::updatePen()
{
    if(_selected && _highlight)
    {
        _pen2 = _selectionHiglightPen;
    }
    else if(_selected)
    {
        _pen2 = _selectionPen;
    }
    else if(_highlight)
    {
        _pen2 = _highlightPen;
    }
    else
    {
        _pen2 = _masterPen;
    }

    update();
}

QString GraphicsRectItem::geometry_s() const
{
    return QString("%1;%2;%3;%4").arg(_geometry.left()).arg(_geometry.top()).arg(_geometry.right()).arg(_geometry.bottom());
}

QRect GraphicsRectItem::geometry() const
{
    return _geometry;
}

void GraphicsRectItem::setGeometry(const QRect &geometry)
{
    if (_geometry == geometry)
        return;

    _last_scale = -1;

    prepareGeometryChange();

    _geometry = geometry;

    _paintRect = QRectF(0, 0, geometry.width(), geometry.height());

    setPos(geometry.topLeft());

    _boundingRect = QRectF();

    update();

    emit geometryChanged(_geometry);
}

void GraphicsRectItem::setSize(const QSize &size)
{
    if (_geometry.size() == size)
        return;

    _last_scale = -1;

    QSize newSize = size.expandedTo(_minimumSize).boundedTo(_maximumSize);

    prepareGeometryChange();

    _geometry.setSize(newSize);

    _paintRect = QRectF(0, 0, newSize.width(), newSize.height());

    _boundingRect = QRectF();

    update();

    emit geometryChanged(_geometry);
}

void GraphicsRectItem::move(const QPoint &point)
{
    if (_geometry.topLeft() == point)
        return;

    _last_scale = -1;

    QPoint newPos = point;

    if(newPos.x() < 0) newPos.setX(0);
    if(newPos.y() < 0) newPos.setY(0);
    if(newPos.x() + _paintRect.width() >= scene()->width()) newPos.setX(scene()->width() - _paintRect.width());
    if(newPos.y() + _paintRect.height() >= scene()->height()) newPos.setY(scene()->height() - _paintRect.height());

    _geometry.moveTopLeft(newPos);

    setPos(newPos);

    emit positionChanged(newPos);
}

void GraphicsRectItem::raise()
{
    foreach (QGraphicsItem *other, collidingItems(Qt::IntersectsItemShape))
    {
        if(!other->isAncestorOf(this) && (other->parentItem() == parentItem()))
        {
            other->stackBefore(this);
        }
    }
}

QRectF GraphicsRectItem::boundingRect() const
{
    if (_boundingRect.isNull())
    {
        qreal halfpw = _slavePen.widthF() / 2;

        _boundingRect = _paintRect;

        if (halfpw > 0.0)
        {
            _boundingRect.adjust(-halfpw, -halfpw, halfpw, halfpw);
        }

        _boundingRect.adjust(-_bounding_pad, -_bounding_pad, _bounding_pad, _bounding_pad);
    }

    return _boundingRect;
}

QPainterPath GraphicsRectItem::shape() const
{
    QPainterPath path;

    path.addRect(_paintRect.adjusted(-_bounding_pad, -_bounding_pad, _bounding_pad, _bounding_pad));

    return qt_graphicsItem_shapeFromPath(path, _slavePen);
}

bool GraphicsRectItem::contains(const QPointF &point) const
{
    return QGraphicsObject::contains(point);
}

void GraphicsRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    updateRegions(1.0 / painter->transform().m11());

    if(_show_touch_regions && _flags.testFlag(ItemAcceptsMouseCapture) && _selected)
    {
        QPen p1 = _pen1;
        p1.setStyle(Qt::DashLine);

        painter->setPen(p1);

        painter->drawRect(_mouse_regions.bottomLeftRegion);
        painter->drawRect(_mouse_regions.bottomRightRegion);
        painter->drawRect(_mouse_regions.topLeftRegion);
        painter->drawRect(_mouse_regions.topRightRegion);

        QPen p2 = _pen2;
        p2.setStyle(Qt::DashLine);

        painter->setPen(p2);

        painter->drawRect(_mouse_regions.bottomLeftRegion);
        painter->drawRect(_mouse_regions.bottomRightRegion);
        painter->drawRect(_mouse_regions.topLeftRegion);
        painter->drawRect(_mouse_regions.topRightRegion);
    }

    painter->setPen(_pen1);
    painter->drawRect(_paintRect);

    painter->setPen(_pen2);
    painter->drawRect(_paintRect);
}

bool GraphicsRectItem::isCaptured() const
{
    return _captured;
}

bool GraphicsRectItem::isObscuredBy(const QGraphicsItem *item) const
{
    return QGraphicsObject::isObscuredBy(item);
}

QPainterPath GraphicsRectItem::opaqueArea() const
{
    return QGraphicsObject::opaqueArea();
}

void GraphicsRectItem::setRectFlag(GraphicsRectItem::GraphicsRectItemFlag flag, bool enable)
{
    if(enable)
    {
        _flags |= flag;
    }
    else
    {
        _flags &= ~flag;
    }
}

void GraphicsRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(_captured)
    {
        QPointF delta = event->scenePos() - _capturePoint;

        if((_kdx || _kdy) && !(_kdw || _kdh))
        {
            QPoint newPos = (_captureGeometry.topLeft() + delta).toPoint();

            move(newPos);
        }
        else if((_kdw || _kdh) && !(_kdx || _kdy))
        {
            QSize newSize(qRound(_captureGeometry.width() + _kdw * delta.x()), qRound(_captureGeometry.height() + _kdh * delta.y()));

            if(newSize.width() + _captureGeometry.x() >= scene()->width())
            {
                newSize.setWidth(scene()->width() - _captureGeometry.x());
            }

            if(newSize.height() + _captureGeometry.y() >= scene()->height())
            {
                newSize.setHeight(scene()->height() - _captureGeometry.y());
            }

            if(!_alert_counter && ((newSize.width() > _maximumSize.width()) || (newSize.height() > _maximumSize.height()) ||
                                   (newSize.width() < _minimumSize.width()) || (newSize.height() < _minimumSize.height())))
            {
                alert_1();
            }

            setSize(newSize);
        }
        else
        {
            int newX = qRound(_captureGeometry.x() + _kdx * delta.x());
            int newY = qRound(_captureGeometry.y() + _kdy * delta.y());

            int newWidth  = qRound(_captureGeometry.width() + _kdw * delta.x());
            int newHeight = qRound(_captureGeometry.height() + _kdh * delta.y());

            if(newX < 0)
            {
                newX = 0;
                newWidth = _captureGeometry.right();
            }

            if(newY < 0)
            {
                newY = 0;
                newHeight = _captureGeometry.bottom();
            }

            if(newWidth + newX >= scene()->width())
            {
                newWidth = scene()->width() - newX;
            }

            if(newHeight + newY >= scene()->height())
            {
                newHeight = scene()->height() - newY;
            }

            int boundWidth  = qBound(_minimumSize.width(), newWidth, _maximumSize.width());
            int boundHeight = qBound(_minimumSize.height(), newHeight, _maximumSize.height());

            if(boundWidth != newWidth)
            {
                newX = (_kdw == 1) ? _captureGeometry.left() : _captureGeometry.right() - boundWidth;
            }

            if(boundHeight != newHeight)
            {
                newY = (_kdh == 1) ? _captureGeometry.top() : _captureGeometry.bottom() - boundHeight;
            }

            setGeometry(QRect(QPoint(newX, newY), QSize(boundWidth, boundHeight)));

            if(!_alert_counter && ((newWidth > _maximumSize.width()) || (newHeight > _maximumSize.height()) ||
                                   (newWidth < _minimumSize.width()) || (newHeight < _minimumSize.height())))
            {
                alert_1();
            }
        }

        scene()->invalidate();
    }
}

void GraphicsRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit mousePress();

    if(!_flags.testFlag(ItemAcceptsMouseCapture))
    {
        return;
    }

    if(event->button() == Qt::LeftButton)
    {
        if(_flags.testFlag(ItemSelectOnClick))
        {
            _selected = true;

            updatePen();
        }

        _captured = true;

        _capturePoint = event->scenePos();
        _captureGeometry = QRectF(scenePos(), _paintRect.size());

        updateMousePosition(event->scenePos(), event->lastScenePos(), event->screenPos(), event->lastScreenPos(), event->pos(), event->widget());
    }
}

void GraphicsRectItem::updateMousePosition(const QPointF &, const QPointF &,
                                             const QPointF &, const QPointF &,
                                             const QPointF &event_pos, QWidget *widget)
{
    _kdx = 0; _kdy = 0; _kdw = 0; _kdh = 0;

    if(!_selected && _flags.testFlag(ItemFillScene))
    {
        _kdx = 1; _kdy = 1;

        widget->setCursor(Qt::SizeAllCursor);

        if(!_highlight)
        {
            _highlight = true;

            updatePen();
        }
    }
    else
    {
        if(_mouse_regions.outerRegion.contains(event_pos))
        {
            QCursor newCursor;

            if(_mouse_regions.topLeftRegion.contains(event_pos))
            {
                _kdx = 1; _kdy = 1; _kdw = -1; _kdh = -1;

                _capturedRect = &_mouse_regions.topLeftRegion;

                newCursor = Qt::SizeFDiagCursor;
            }
            else if(_mouse_regions.bottomRightRegion.contains(event_pos))
            {
                _kdw = 1; _kdh = 1;

                _capturedRect = &_mouse_regions.bottomRightRegion;

                newCursor = Qt::SizeFDiagCursor;
            }
            else if(_mouse_regions.bottomLeftRegion.contains(event_pos))
            {
                _kdx = 1; _kdw = -1; _kdh = 1;

                _capturedRect = &_mouse_regions.bottomLeftRegion;

                newCursor = Qt::SizeBDiagCursor;
            }
            else if(_mouse_regions.topRightRegion.contains(event_pos))
            {
                _kdy = 1; _kdw = 1; _kdh = -1;

                _capturedRect = &_mouse_regions.topRightRegion;

                newCursor = Qt::SizeBDiagCursor;
            }
            else if(_mouse_regions.innerRegion.contains(event_pos))
            {
                _kdx = 1; _kdy = 1;

                newCursor = Qt::SizeAllCursor;
            }
            else if(_mouse_regions.topRegion.contains(event_pos))
            {
                _kdy = 1; _kdh = -1;

                newCursor = Qt::SizeVerCursor;
            }
            else if(_mouse_regions.bottomRegion.contains(event_pos))
            {
                _kdh = 1;

                newCursor = Qt::SizeVerCursor;
            }
            else if(_mouse_regions.leftRegion.contains(event_pos))
            {
                _kdx = 1; _kdw = -1;

                newCursor = Qt::SizeHorCursor;
            }
            else if(_mouse_regions.rightRegion.contains(event_pos))
            {
                _kdw = 1;

                newCursor = Qt::SizeHorCursor;
            }

            widget->setCursor(newCursor);

            if(!_highlight)
            {
                _highlight = true;

                updatePen();
            }
        }
        else
        {
            if(_highlight)
            {
                _highlight = false;

                updatePen();
            }

            widget->unsetCursor();
        }
    }
}

void GraphicsRectItem::alert_1()
{
    _pen2.setColor(Qt::red);

    if(++_alert_counter < 3)
    {
        QTimer::singleShot(100, this, SLOT(alert_2()));
    }
    else
    {
        _pen2 = _selectionHiglightPen;

        _alert_counter = 0;
    }

    update();
}

void GraphicsRectItem::alert_2()
{
    _pen2.setColor(Qt::green);

    update();

    QTimer::singleShot(80, this, SLOT(alert_1()));
}

void GraphicsRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{


    Q_UNUSED(event);

    _captured = false;
    _capturedRect = 0;

    foreach (QGraphicsItem *other, collidingItems(Qt::IntersectsItemShape))
    {
        if(!other->isAncestorOf(this) && (other->parentItem() == parentItem()))
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

void GraphicsRectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsObject::hoverEnterEvent(event);

    scene()->invalidate();
}

void GraphicsRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    _kdx = 0; _kdy = 0; _kdw = 0; _kdh = 0;

    event->widget()->unsetCursor();

    _highlight = false;

    updatePen();

    QGraphicsObject::hoverLeaveEvent(event);

    scene()->invalidate();
}

void GraphicsRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if(_flags.testFlag(ItemAcceptsMouseCapture))
    {
        updateMousePosition(event->scenePos(), event->lastScenePos(), event->screenPos(), event->lastScreenPos(), event->pos(), event->widget());
    }

    QGraphicsObject::hoverMoveEvent(event);

    scene()->invalidate();
}

void GraphicsRectItem::keyPressEvent(QKeyEvent *event)
{
    bool blockEvent = false;

    if(_flags.testFlag(ItemAcceptsKeyboard) && _selected)
    {
        QPoint delta = _keyDelta.value(event->key());

        if(!delta.isNull())
        {
            blockEvent = true;

            if(!_keyState[event->key()])
            {
                _keyState[event->key()] = true;

                _keyAD += delta;
            }

            move(geometry().topLeft() + _keyAD);
        }
    }

    if(!blockEvent) QGraphicsObject::keyPressEvent(event);
}

void GraphicsRectItem::keyReleaseEvent(QKeyEvent *event)
{
    bool blockEvent = false;

    if(_flags.testFlag(ItemAcceptsKeyboard) && _selected)
    {
        blockEvent = true;

        QPoint delta = _keyDelta.value(event->key());

        _keyAD -= delta;

        if(!delta.isNull()) _keyState[event->key()] = false;
    }

    if(!blockEvent) QGraphicsObject::keyReleaseEvent(event);
}

QVariant GraphicsRectItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsObject::itemChange(change, value);
}

void GraphicsRectItem::updateRegions(qreal scale)
{
    if(!qFuzzyCompare(scale, _last_scale))
    {
        _last_scale = scale;

        double pad1 = _outer_pad * scale;
        double pad2 = _inner_pad * scale;

        prepareGeometryChange();

        _boundingRect = QRectF();

        if(_paintRect.width() <= (2.5 * pad2) || _paintRect.height() <= (2.5 * pad2))
        {
            pad1 = pad2;
            pad2 = 0;

            _bounding_pad = pad1;
        }
        else
        {
            _bounding_pad = 0;
        }

        if(!_selected && _flags.testFlag(ItemFillScene))
        {
            _bounding_pad = qMax(scene()->sceneRect().width(), scene()->sceneRect().height());
        }

        _mouse_regions.set_pad(pad1, pad2, _paintRect, _capturedRect);

        update();
    }
}

int GraphicsRectItem::type() const
{
    return Type;
}

void GraphicsRectItem::setMinimumSize(const QSize &size)
{
    _minimumSize = size;
}

void GraphicsRectItem::setMaximumSize(const QSize &size)
{
    _maximumSize = size;
}

void GraphicsRectItem::setMasterColor(const QColor &color)
{
    _masterPen.setColor(color);
    _highlightPen.setColor(color);

    updatePen();
}

void GraphicsRectItem::setSlaveColor(const QColor &color)
{
    _slavePen.setColor(color);

    updatePen();
}

void GraphicsRectItem::setSelectionColor(const QColor &color)
{
    _selectionPen.setColor(color);
    _selectionHiglightPen.setColor(color);

    updatePen();
}

void GraphicsRectItem::setMousePad(qreal pad)
{
    _outer_pad = pad;

    update();
}

void GraphicsRectItem::setSelected(bool selected)
{
    _selected = selected;

    updatePen();
}
