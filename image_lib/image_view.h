#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QGraphicsView>

class ImageViewPrivate;

//! Graphics view with zoom, auto fitting, pinch gestures and more features.
class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = 0);

    virtual ~ImageView();

    void fit_to_view();
    void fit_to_original();

    void set_auto_fit(bool autoFit);
    void set_zoom_speed(qreal zoomSpeed);
    void set_max_zoom(qreal value);
    void set_scale(qreal scale, const QPoint &center);

public:
    bool is_auto_fit() const;
    bool is_fitted() const;

    QRectF roi_rect() const;
    qreal current_scale() const;

public slots:
    void updateSceneRect(const QRectF &rect);

signals:
    void view_rect_changed(const QRectF &rect);
    void zoom_changed();
    void mouse_move(const QPointF &pos);

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);

    void wheelEvent(QWheelEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void scrollContentsBy(int dx, int dy);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);

private:
    void _set_scale(qreal s);
    void _fit_to_view(const QRectF &rect);

private:
    ImageViewPrivate *_p;
};

#endif // IMAGEVIEW_H
