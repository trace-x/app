#ifndef GRAPHICS_IMAGE_ITEM_H
#define GRAPHICS_IMAGE_ITEM_H

#include <QGraphicsObject>

class GraphicsImageItem : public QGraphicsObject
{
    Q_OBJECT

public:
    GraphicsImageItem(QGraphicsItem *parent = 0);

    ~GraphicsImageItem();

    enum GraphicsImageItemFlag {
        ItemSmoothDownsampling = 0x1,
        ItemSmoothUpsampling = 0x2
    };

    Q_DECLARE_FLAGS(GraphicsImageItemFlags, GraphicsImageItemFlag)

    enum { Type = UserType + 3 };
    int type() const;

    QRectF boundingRect() const;

    bool contains(const QPointF &point) const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPainterPath opaqueArea() const;

    void set_grpahics_image_flag(GraphicsImageItemFlag flag, bool enable = true);

public slots:
    void set_image(const QImage &image);

private:
    QImage _image;
    GraphicsImageItemFlags _flags;

private:
    Q_DISABLE_COPY(GraphicsImageItem)
};

#endif // GRAPHICS_IMAGE_ITEM_H
