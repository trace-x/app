#ifndef ABSTRACT_GRAPHICS_SHAPE_ITEM_H
#define ABSTRACT_GRAPHICS_SHAPE_ITEM_H

#include <QAbstractGraphicsShapeItem>

class AbstractGraphicsShapeItem : public QGraphicsObject
{
    Q_OBJECT

public:
    enum GraphicsItemFlag {
        ItemAcceptsKeyboard = 0x1,
        ItemSelectOnClick = 0x2,
        ItemAcceptsMouseCapture = 0x4,
        ItemDiscreteGeometry = 0x8,
        ItemDisplayRealCoordinates = 0x10,
        ItemDisplayLabel = 0x20
    };

    Q_DECLARE_FLAGS(GraphicsItemFlags, GraphicsItemFlag)

public:
    explicit AbstractGraphicsShapeItem(QGraphicsItem *parent = 0);

    void setPos(const QPointF &pos);
    void setPos(qreal x, qreal y) { setPos(QPointF(x, y)); }

    void setSelected(bool selected);
    bool isSelected() const;

    virtual void move(const QPointF &point, qreal sx = 0);
    virtual void setPosition(const QPointF &point) = 0;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

signals:
    void positionChanged(const QPointF &pos);
    void moved(const QPointF &pos);

    void mousePress();
    void mouseRelease();
    void keyPress(QKeyEvent *event);

protected:
    GraphicsItemFlags _flags;

    QMap<int, QPointF> _keyDelta;
    QMap<int, bool> _keyState;

    QPointF _keyAD;

    bool _highlight;
    bool _selected;
    bool _captured;

    QPointF _captureMousePoint;
    QPointF _capturePos;
};

#endif // VC_ABSTRACT_GRAPHICS_SHAPE_ITEM_H
