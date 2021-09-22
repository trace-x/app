#include "image_view.h"

#include <QApplication>
#include <QWheelEvent>

#include "trace_x/trace_x.h"

class PinchGestureParams
{
public:
    PinchGestureParams() :
          total_scale_factor(1), last_scale_factor(1), scale_factor(1),
          total_rotation_angle(0), last_rotation_angle(0), rotation_angle(0),
          is_new_sequence(true)
    {
    }

    QPointF start_center_point;
    QPointF last_center_point;
    QPointF center_point;

    qreal total_scale_factor;
    qreal last_scale_factor;
    qreal scale_factor;

    qreal total_rotation_angle;
    qreal last_rotation_angle;
    qreal rotation_angle;

    bool is_new_sequence;
    QPointF start_position[2];
};

class ImageViewPrivate
{
public:
    ImageViewPrivate():
        captured(false),
        auto_fit(true),
        block(false),
        zoom_speed(0.15),
        max_zoom(25.0),
        capture_button(Qt::RightButton),
        captured_button(Qt::NoButton)
    {}

    bool captured;
    bool auto_fit;
    bool block;

    qreal zoom_speed;
    qreal max_zoom;

    QPointF center_point;
    QPoint  capture_point;

    PinchGestureParams pinch_gesture;

    Qt::MouseButton capture_button;
    Qt::MouseButton captured_button;
};

ImageView::ImageView(QWidget *parent):
    QGraphicsView(parent),
    _p(new ImageViewPrivate)
{
    X_CALL;

    setAttribute(Qt::WA_AcceptTouchEvents);

    setContentsMargins(QMargins());

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ImageView::~ImageView()
{
    X_CALL;

    delete _p;
}

void ImageView::fit_to_view()
{
    _fit_to_view(sceneRect());

    emit zoom_changed();
}

void ImageView::_fit_to_view(const QRectF &rect)
{
    if(scene())
    {
        qreal image_ratio = rect.width() / rect.height();

        qreal view_aspect_ratio = contentsRect().width() / qreal(contentsRect().height());

        qreal scale = 0;

        if(image_ratio > view_aspect_ratio)
        {
            scale = contentsRect().width() / rect.width();
        }
        else
        {
            scale = contentsRect().height() / rect.height();
        }

        setTransform(transform().fromScale(scale, scale));

        setSceneRect(scene()->sceneRect());

        emit view_rect_changed(roi_rect());
    }
}

void ImageView::fit_to_original()
{
    setTransform(transform().fromScale(1, 1));
}

void ImageView::set_auto_fit(bool autoFit)
{
    _p->auto_fit = autoFit;
}

bool ImageView::is_auto_fit() const
{
    return _p->auto_fit;
}

bool ImageView::is_fitted() const
{
    qreal s = transform().m11();

    bool brx = qRound(sceneRect().width() * s) > contentsRect().width();
    bool bry = qRound(sceneRect().height() * s) > contentsRect().height();

    return !(brx || bry);
}

void ImageView::paintEvent(QPaintEvent *event)
{
    if(!sceneRect().isValid())
    {
        QPainter painter;
        painter.begin(viewport());
        painter.save();

        painter.drawText(viewport()->geometry(), Qt::AlignCenter, tr("Image is empty"));

        painter.restore();
        painter.end();
    }
    else
    {
        QGraphicsView::paintEvent(event);
    }
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if(scene())
    {
        int num_steps = event->delta() / 120;

        set_scale(transform().m11() * (1.0 + num_steps * _p->zoom_speed), event->pos());
    }

    QFrame::wheelEvent(event);
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    event->setAccepted(false);

    if(!_p->block)
    {
        QGraphicsView::mousePressEvent(event);
    }

    _p->captured_button = event->button();

    if(scene() && !event->isAccepted() && (event->button() == _p->capture_button) && !is_fitted())
    {
        viewport()->setCursor(Qt::ClosedHandCursor);

        _p->captured = true;
        _p->capture_point = event->pos();

        _p->center_point = mapToScene(viewport()->rect().center());
    }

    if(event->button() == Qt::LeftButton)
    {
        emit mouse_move(mapToScene(event->pos()));
    }

    if(!event->isAccepted())
    {
        //we must send event to parent widget to handle container clicking
        QAbstractScrollArea::mousePressEvent(event);
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    event->setAccepted(false);

    QGraphicsView::mouseReleaseEvent(event);

    if(_p->captured && (event->button() == _p->capture_button))
    {
        viewport()->unsetCursor();

        _p->captured = false;
    }

    QAbstractScrollArea::mouseReleaseEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);

    event->setAccepted(false);

    if(_p->captured)
    {
        QPointF delta = mapToScene(_p->capture_point) - mapToScene(event->pos());

        centerOn(_p->center_point + delta);

        //Обновление для того, что избежать артефактов
        viewport()->update();

        event->accept();
    }

    if(_p->captured_button == Qt::LeftButton)
    {
        emit mouse_move(mapToScene(event->pos()));
    }

    if(!event->isAccepted())
    {
        QAbstractScrollArea::mouseMoveEvent(event);
    }
}

void ImageView::_set_scale(qreal s)
{
    //hack for prevent rounding problems inside qt

    int ss = 0x00010000 / s;

    s = 0x00010000 / (qreal)(ss);

    int w = sceneRect().width() * s;

    s = w / qreal(sceneRect().width());

    QTransform newTransform = transform().fromScale(s, s);

    setTransform(newTransform);
}

bool ImageView::event(QEvent *event)
{
    if(event->type() == QEvent::TouchBegin)
    {
        return true;
    }
    else if(event->type() == QEvent::TouchUpdate)
    {
        const QTouchEvent *touch_event = static_cast<const QTouchEvent *>(event);

        if (touch_event->touchPoints().size() == 2)
        {
            QTouchEvent::TouchPoint p1 = touch_event->touchPoints().at(0);
            QTouchEvent::TouchPoint p2 = touch_event->touchPoints().at(1);

            QPointF center_point = (p1.screenPos() + p2.screenPos()) / 2.0;

            if (_p->pinch_gesture.is_new_sequence)
            {
                _p->pinch_gesture.start_position[0] = p1.screenPos();
                _p->pinch_gesture.start_position[1] = p2.screenPos();
                _p->pinch_gesture.last_center_point = center_point;
            }
            else
            {
                _p->pinch_gesture.last_center_point = _p->pinch_gesture.center_point;
            }

            _p->pinch_gesture.center_point = center_point;

            if (_p->pinch_gesture.is_new_sequence)
            {
                _p->pinch_gesture.scale_factor = current_scale();
                _p->pinch_gesture.last_scale_factor = current_scale();
            }
            else
            {
                _p->pinch_gesture.last_scale_factor = _p->pinch_gesture.scale_factor;
                QLineF line(p1.screenPos(), p2.screenPos());
                QLineF lastLine(p1.lastScreenPos(),  p2.lastScreenPos());
                _p->pinch_gesture.scale_factor = line.length() / lastLine.length();
            }

            _p->block = true;

            _p->pinch_gesture.total_scale_factor = _p->pinch_gesture.total_scale_factor * _p->pinch_gesture.scale_factor;

            _p->pinch_gesture.is_new_sequence = false;

            QPoint view_center = mapFromGlobal(_p->pinch_gesture.center_point.toPoint());

            set_scale(_p->pinch_gesture.total_scale_factor, view_center);

            _p->pinch_gesture.last_scale_factor = _p->pinch_gesture.total_scale_factor;
        }

        return true;
    }
    else if(event->type() == QEvent::TouchEnd)
    {
        _p->pinch_gesture.start_center_point = _p->pinch_gesture.last_center_point = _p->pinch_gesture.center_point = QPointF();
        _p->pinch_gesture.total_scale_factor = _p->pinch_gesture.last_scale_factor = _p->pinch_gesture.scale_factor = 1;
        _p->pinch_gesture.total_rotation_angle = _p->pinch_gesture.last_rotation_angle = _p->pinch_gesture.rotation_angle = 0;

        _p->pinch_gesture.is_new_sequence = true;
        _p->pinch_gesture.start_position[0] = _p->pinch_gesture.start_position[1] = QPointF();

        _p->block = false;

        return true;
    }

    return QGraphicsView::event(event);
}

void ImageView::set_max_zoom(qreal value)
{
    _p->max_zoom = value;
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QFrame::mouseDoubleClickEvent(event);
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);

    if(_p->auto_fit || is_fitted())
    {
        _p->auto_fit = true;

        fit_to_view();
    }
    else
    {
        setSceneRect(scene()->sceneRect());

        emit view_rect_changed(roi_rect());
    }
}

void ImageView::dragEnterEvent(QDragEnterEvent *event)
{
    QFrame::dragEnterEvent(event);
}

void ImageView::dragMoveEvent(QDragMoveEvent *event)
{
    QFrame::dragMoveEvent(event);
}

void ImageView::dragLeaveEvent(QDragLeaveEvent *event)
{
    QFrame::dragLeaveEvent(event);
}

void ImageView::updateSceneRect(const QRectF &rect)
{
    QGraphicsView::updateSceneRect(rect);

    _fit_to_view(rect);
}

void ImageView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);

    emit view_rect_changed(roi_rect());

    update();
}

void ImageView::set_zoom_speed(qreal zoomSpeed)
{
    _p->zoom_speed = zoomSpeed;
}

QRectF ImageView::roi_rect() const
{
    return mapToScene(viewport()->rect()).boundingRect();
}

qreal ImageView::current_scale() const
{
    return transform().m11();
}

void ImageView::set_scale(qreal scale, const QPoint &center)
{
    if(scale > _p->max_zoom)
    {
        scale = _p->max_zoom;
    }

    QPointF pos1 = mapToScene(center);
    QPointF center1 = mapToScene(viewport()->rect().center());

    qreal rx = sceneRect().width() * scale / contentsRect().width();
    qreal ry = sceneRect().height() * scale / contentsRect().height();

    if((rx > 1.0) || (ry > 1.0))
    {
        if(!qFuzzyCompare(scale, transform().m11()))
        {
            _set_scale(scale);

            if(qFuzzyCompare(scale, _p->max_zoom))
            {
                _p->max_zoom = transform().m11();
            }

            QPointF pos2 = mapToScene(center);

            QPointF delta = pos2 - pos1;

            centerOn(center1 - delta);

            emit view_rect_changed(roi_rect());

            _p->auto_fit = false;
        }
    }
    else
    {
        fit_to_view();

        centerOn(scene()->sceneRect().center());

        _p->auto_fit = true;
    }

    emit zoom_changed();
}
