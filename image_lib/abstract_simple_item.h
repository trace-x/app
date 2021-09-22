#ifndef ABSTRACT_SIMPLE_ITEM_H
#define ABSTRACT_SIMPLE_ITEM_H

#include <QFont>

#include "abstract_graphics_shape_item.h"

class AbstractSimpleItem : public AbstractGraphicsShapeItem
{
    Q_OBJECT

public:
    explicit AbstractSimpleItem(QGraphicsItem *parent = 0);

public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    virtual void move(const QPointF &point, qreal sx = 0);
    virtual void setPosition(const QPointF &point);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void setLabelText(const QString &text);
    void setLabelFont(const QFont &font);

    QString labelText() const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void updateParams();

private:
    QFont   _labelFont;
    QRectF  _labelRect;
    QString _labelText;
    QString _labelPaintText;
    QPointF _displayPos;

private:
    Q_DISABLE_COPY(AbstractSimpleItem)
};

#endif // ABSTRACT_SIMPLE_ITEM_H
