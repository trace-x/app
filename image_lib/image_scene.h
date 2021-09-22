#ifndef IMAGESCENE_H
#define IMAGESCENE_H

#include <QGraphicsScene>

#include "image.h"
#include "graphics_image_item.h"

class ImageScene : public QGraphicsScene
{
    Q_OBJECT

public:
    ImageScene(QObject *parent = 0);

    void set_image(const Image &image);

    const Image &image() const;

    GraphicsImageItem * image_item();

protected:
    GraphicsImageItem *_image_item;

    Image _image;
};

#endif // IMAGESCENE_H
