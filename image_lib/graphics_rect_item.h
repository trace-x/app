#ifndef VC_GRAPHICS_RECT_ITEM_H
#define VC_GRAPHICS_RECT_ITEM_H

#include <QGraphicsObject>
#include <QSet>
#include <QPen>
#include <QTimer>

class GraphicsRectItem : public QGraphicsObject
{
    Q_OBJECT

public:
    enum GraphicsRectItemFlag {
        ItemAcceptsKeyboard = 0x1,
        ItemSelectOnClick = 0x2,
        ItemAcceptsMouseCapture = 0x4,
        ItemFillScene = 0x8
    };

    Q_DECLARE_FLAGS(GraphicsRectItemFlags, GraphicsRectItemFlag)

public:
    GraphicsRectItem(QGraphicsItem *parent = 0);
    GraphicsRectItem(const QRect &geometry, QGraphicsItem *parent = 0);
    GraphicsRectItem(int x, int y, int w, int h, QGraphicsItem *parent = 0);

    virtual ~GraphicsRectItem();

public slots:
    void setSelected(bool selected);
    void setGeometry(const QRect &geometry);
    void setSize(const QSize &size);
    void move(const QPoint &point);
    void raise();

signals:
    void positionChanged(const QPoint &pos);
    void geometryChanged(const QRect &geometry);

    void mousePress();
    void mouseRelease();

public:
    QRect geometry() const;

    inline void setGeometry(int x, int y, int w, int h);

public:
    QRectF boundingRect() const;
    QPainterPath shape() const;
    bool contains(const QPointF &point) const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    bool isCaptured() const;
    bool isObscuredBy(const QGraphicsItem *item) const;
    QPainterPath opaqueArea() const;

    void setRectFlag(GraphicsRectItemFlag flag, bool enable = true);

    enum { Type = UserType + 2 };
    int type() const;

public:
    void setMinimumSize(const QSize &size);
    void setMaximumSize(const QSize &size);

    void setMasterColor(const QColor &color);
    void setSlaveColor(const QColor &color);
    void setSelectionColor(const QColor &color);

    void setMousePad(qreal pad);

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
    void updateRegions(qreal scale);
    void initialize();
    void updatePen();
    void updateMousePosition(const QPointF &scene_pos, const QPointF &last_scene_pos,
                             const QPointF &screen_pos, const QPointF &last_screen_pos,
                             const QPointF &event_pos, QWidget *widget);

private slots:
    void alert_1();
    void alert_2();

private slots:
    QString geometry_s() const;

private:
    GraphicsRectItemFlags _flags;

    struct regions_t
    {
        void set_pad(qreal pad1, qreal pad2, const QRectF &rect, QRectF *)
        {
            QRectF ambitRect(QPointF(-pad1, -pad1), QPointF(pad2, pad2));

            innerRegion       = QRectF(ambitRect.bottomRight(), rect.bottomRight() - ambitRect.bottomRight());
            outerRegion       = QRectF(ambitRect.topLeft(), rect.bottomRight() - ambitRect.topLeft());

            topLeftRegion     = QRectF(outerRegion.topLeft(), innerRegion.topLeft());
            bottomRightRegion = QRectF(innerRegion.bottomRight(), outerRegion.bottomRight());
            topRegion         = QRectF(topLeftRegion.topRight(), innerRegion.topRight());
            bottomRegion      = QRectF(innerRegion.bottomLeft(), bottomRightRegion.bottomLeft());
            leftRegion        = QRectF(topLeftRegion.bottomLeft(), bottomRegion.topLeft());
            rightRegion       = QRectF(topRegion.bottomRight(), bottomRightRegion.topRight());
            bottomLeftRegion  = QRectF(leftRegion.bottomLeft(), bottomRegion.bottomLeft());
            topRightRegion    = QRectF(topRegion.topRight(), rightRegion.topRight());
        }

        QRectF innerRegion;
        QRectF outerRegion;
        QRectF topRegion;
        QRectF leftRegion;
        QRectF topLeftRegion;
        QRectF bottomRegion;
        QRectF bottomLeftRegion;
        QRectF bottomRightRegion;
        QRectF rightRegion;
        QRectF topRightRegion;
    };

    QPen _pen1;
    QPen _pen2;

    QPen _masterPen;
    QPen _slavePen;
    QPen _highlightPen;
    QPen _selectionPen;
    QPen _selectionHiglightPen;

    QMap<int, QPoint> _keyDelta;
    QMap<int, bool> _keyState;

    QPoint _keyAD;

    int _keyDx;
    int _keyDy;

    qreal _outer_pad;
    qreal _inner_pad;
    qreal _bounding_pad;

    //TODO support multiple views
    qreal _last_scale;

    QRectF *_capturedRect;
    QRectF _paintRect;
    QRect  _geometry;

    QVector<QRectF> _corners;

    mutable QRectF _boundingRect;

    bool   _highlight;
    bool   _selected;
    bool   _captured;
    bool   _show_touch_regions;

    int _kdx;
    int _kdy;
    int _kdw;
    int _kdh;

    QPointF _capturePoint;

    QRectF _captureGeometry;

    regions_t _mouse_regions;

    QSize _minimumSize;
    QSize _maximumSize;

    QTimer _timer;
    int _alert_counter;

private:
    Q_DISABLE_COPY(GraphicsRectItem)
};

inline void GraphicsRectItem::setGeometry(int ax, int ay, int w, int h)
{ setGeometry(QRect(ax, ay, w, h)); }

#endif // VC_GRAPHICS_RECT_ITEM_H
