#include "image_table_model.h"

#include <QTimer>

#include "trace_x/trace_x.h"

template<class T>
QVector<QString> pixel_accessor(const Image &image, int x, int y)
{
    QVector<QString> pixel(image.channels);

    // acess by native (XYZC) axes order

    for(quint32 i = 0; i < image.channels; ++i)
    {
        T value = *((const T*)(image.data_start) + (i * image.height * image.width) + (image.width * y) + x);

        pixel[i] = (image.unit_type >= trace_x::T_32F) ? QString::number(value, 'f', 3) : QString::number(value);
    }

    return pixel;
}

GENERATE_FUNCTION_ACCESS(get_pixel_accessor, ImageAccess, pixel_accessor)

ImageTableModel::ImageTableModel(QObject *parent):
    QAbstractTableModel(parent)
{
    X_CALL;
}

ImageTableModel::~ImageTableModel()
{
    X_CALL;
}

void ImageTableModel::set_image(const Image &image)
{
    X_CALL;

    emit layoutAboutToBeChanged();

    _image = image;

    _accessor = get_pixel_accessor(_image.unit_type);

    emit layoutChanged();
}

int ImageTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _image.height;
}

int ImageTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _image.width;
}

QVariant ImageTableModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    switch(role)
    {
    case Qt::DisplayRole:
    {
        QVector<QString> pixel = _accessor(_image, index.column(), index.row());

        const auto &m = _image.band_map;

        switch (_image.channels)
        {
        case 1: result = pixel[0]; break;
        case 2: result = QString("%1;%2").arg(pixel[m.value(trace_x::BAND_GRAY)]).arg(pixel[m.value(trace_x::BAND_ALPHA)]); break;
        case 3: result = QString("%1;%2;%3").arg(pixel[m.value(trace_x::BAND_RED)]).arg(pixel[m.value(trace_x::BAND_GREEN)]).arg(pixel[m.value(trace_x::BAND_BLUE)]); break;
        case 4: result = QString("%1;%2;%3;%4").arg(pixel[m.value(trace_x::BAND_RED)]).arg(pixel[m.value(trace_x::BAND_GREEN)]).arg(pixel[m.value(trace_x::BAND_BLUE)]).arg(pixel[m.value(trace_x::BAND_ALPHA)]); break;
        }

        break;
    }
     case Qt::TextAlignmentRole:
        result = Qt::AlignCenter;
        break;
    }

    return result;
}

QVariant ImageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    QVariant result;

    if(role == Qt::DisplayRole)
    {
        result = section;
    }

    return result;
}
