#include <QImage>
#include <QPixmap>

inline trace_x::image_stream &operator<<(trace_x::image_stream &os, const QImage &image)
{
    if(!image.isNull())
    {
        uint32_t num_of_channels = 4;

        if(image.colorTable().isEmpty())
        {
            switch (image.depth())
            {
            case 1:
            case 8: num_of_channels = 1; break;
            case 16:
            case 24: num_of_channels = 3; break;
            case 32: num_of_channels = 4; break;
            default: num_of_channels = 1;
            }
        }

        os.data.assign_matrix(image.width(), image.height(), num_of_channels, image.depth(),
                         image.constBits(), trace_x::QT_IMAGE_FORMAT | image.format(), image.bytesPerLine());

        if(!image.colorTable().isEmpty())
        {
            QVector<QRgb> color_table = image.colorTable();

            os.data.set_palette(&color_table[0], color_table.size() * sizeof(QRgb));
        }
    }

    return os;
}

inline trace_x::image_stream &operator<<(trace_x::image_stream &os, const QPixmap &pixmap)
{
    return os << pixmap.toImage();
}

//TODO QBitmap
