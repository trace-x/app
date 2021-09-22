#include "image_scene.h"

ImageScene::ImageScene(QObject *parent):
    QGraphicsScene(parent),
    _image_item(new GraphicsImageItem())
{
    addItem(_image_item);
}

void ImageScene::set_image(const Image &image)
{
    _image = image;
    _image_item->set_image(image.display_image);

    this->setSceneRect(image.display_image.rect());
}

const Image &ImageScene::image() const
{
    return _image;
}

GraphicsImageItem *ImageScene::image_item()
{
    return _image_item;
}
