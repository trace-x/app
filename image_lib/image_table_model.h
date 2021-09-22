#ifndef IMAGE_TABLE_MODEL_H
#define IMAGE_TABLE_MODEL_H

#include "image.h"

#include <QAbstractTableModel>

typedef QVector<QString> (*ImageAccess)(const Image &image, int x, int y);

class ImageTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ImageTableModel(QObject *parent = 0);
    ~ImageTableModel();

    void set_image(const Image &image);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    Image _image;
    ImageAccess _accessor;
};

#endif // IMAGE_TABLE_MODEL_H
