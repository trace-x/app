#include "image.h"

#include <QDataStream>
#include <QFileInfo>

#include "data_parser.h"

#include "trace_x/trace_x.h"
#include "trace_x/detail/types.h"

#include "CImg.h"

namespace
{

static QVector<quint8> NATIVE_AXES = QVector<quint8>() << 1 << 2 << 3 << 0;
static QVector<quint8> QIMAGE_AXES = QVector<quint8>() << 0 << 1 << 2 << 3;

static const char AXES_XYZC[] = "xyzc";
static const char AXES_CXYZ[] = "cxyz";

static QVector<quint8> GRAY = QVector<quint8>() << trace_x::BAND_GRAY;
static QVector<quint8> GRAY_ALPHA_ORDER = QVector<quint8>() << trace_x::BAND_GRAY << trace_x::BAND_ALPHA;
static QVector<quint8> RGB_ORDER = QVector<quint8>() << trace_x::BAND_RED << trace_x::BAND_GREEN << trace_x::BAND_BLUE;
static QVector<quint8> BGR_ORDER = QVector<quint8>() << trace_x::BAND_BLUE << trace_x::BAND_GREEN << trace_x::BAND_RED;
static QVector<quint8> BGRA_ORDER = QVector<quint8>() << trace_x::BAND_BLUE << trace_x::BAND_GREEN << trace_x::BAND_RED << trace_x::BAND_ALPHA;
static QVector<quint8> RGBA_ORDER = QVector<quint8>() << trace_x::BAND_RED << trace_x::BAND_GREEN << trace_x::BAND_BLUE << trace_x::BAND_ALPHA;
static QVector<quint8> BGRX_ORDER = QVector<quint8>() << trace_x::BAND_BLUE << trace_x::BAND_GREEN << trace_x::BAND_RED << trace_x::BAND_UNDEFINED;
static QVector<quint8> RGBX_ORDER = QVector<quint8>() << trace_x::BAND_RED << trace_x::BAND_GREEN << trace_x::BAND_BLUE << trace_x::BAND_UNDEFINED;

QVector<QRgb> gray_palette()
{
    QVector<QRgb> colors(256);

    for(int i = 0; i < colors.size(); ++i)
    {
        colors[i] = qRgb(i, i, i);
    }

    return colors;
}

typedef bool (*SaveImage)(const Image &image, const QString &dest_file_name);

template<class T>
bool save_cimage(const Image &image, const QString &dest_file_name)
{
    X_CALL_F;

    cimg_library::CImg<T> c_img;

    c_img.assign((const T*)(image.data_start), image.width, image.height, 1, image.channels, true);

    c_img.save(qPrintable(dest_file_name));

    return true;
}

GENERATE_FUNCTION_ACCESS(gen_save_cimage, SaveImage, save_cimage)

template<class T>
void normalize_8u_min_max(const cimg_library::CImg<T> &src, cimg_library::CImg<uint8_t> &dest, T min, T max)
{
    X_CALL_F;

    double alpha = 255.0 / (max - min);
    double beta = -1.0 * min * alpha;

    if(min == max)
    {
        alpha = 0.0;
        beta = 255;
    }

    for(size_t i = 0; i < src.size(); ++i)
    {
        T val = src[i];

        if(val > max)
        {
            val = max;
        }

        if(val < min)
        {
            val = min;
        }

        dest[i] = val * alpha + beta;
    }
}

template<class T>
void binarize_8u(const cimg_library::CImg<T> &src, cimg_library::CImg<uint8_t> &dest)
{
    X_CALL_F;

    for(size_t i = 0; i < src.size(); ++i)
    {
        dest[i] = (src[i] > 0) ? 255 : 0;
    }
}

template<class T>
void histogram_quantiles(const cimg_library::CImg<uint64_t> &histogram, uint64_t size, T min, T max,
                         double left, double right, T &q_left, T &q_right)
{
    X_CALL_F;

    q_left = min;
    q_right = max;

    const double left_val = left * size;
    const double right_val = (1.0 - right) * size;
    const double bin_step = (max - min) / double(histogram.size());

    uint64_t sum = 0;

    for(size_t i = 0; i < histogram.size(); ++i)
    {
        sum += histogram[i];

        if(sum >= left_val)
        {
            q_left = min + i * bin_step;

            break;
        }
    }

    sum = 0;

    for(size_t i = histogram.size() - 1; i > 0; --i)
    {
        sum += histogram[i];

        if(sum >= right_val)
        {
            q_right = min + i * bin_step;

            break;
        }
    }
}

}

template<class T>
QImage hist_quantile_normalization(Image &image, double left_p, double right_p)
{
    X_CALL_F;

    cimg_library::CImg<T> c_img;

    c_img.assign((const T*)(image.data_start), image.width, image.height, 1, image.channels, true);

    QVector<int> channel_widths(image.channels);

    cimg_library::CImg<uint8_t> normed(image.width, image.height, 1, (image.channels == 2) ? 4 : image.channels);

    if(image.channels == 2)
    {
        normed.get_shared_channel(3).fill(255);
    }

    for(quint32 i = 0; i < image.channels; ++i)
    {
        cimg_library::CImg<T> src_channel = c_img.get_shared_channel(i);

        cimg_library::CImg<uint8_t> dest_channel;

        if(image.band_index[i] == trace_x::BAND_ALPHA)
        {
            dest_channel = normed.get_shared_channel(3);
        }
        else
        {
            dest_channel = normed.get_shared_channel(i);
        }

        T min, max;

        min = src_channel.min_max(max);

        // find maximum width in characters
        T abs_max = qMax(qAbs(min), qAbs(max));

        QString v_s = (image.unit_type >= trace_x::T_32F) ? QString::number(abs_max, 'f', 3) : QString::number(abs_max);

        channel_widths[i] = v_s.length();

        if(image.band_index[i] == trace_x::BAND_BINARY)
        {
            ::binarize_8u(src_channel, dest_channel);
        }
        else if(image.unit_type != trace_x::T_8U)
        {
            cimg_library::CImg<uint64_t> histogram = src_channel.get_histogram(10000, min, max);

            T left_q, right_q;

            ::histogram_quantiles<T>(histogram, image.width * image.height, min, max, left_p, right_p, left_q, right_q);

            X_INFO_F("min[{}]: {}; max: {}", i, min, max);
            X_INFO_F("left_q[{}]: {}; right_q: {}", i, left_q, right_q);

            ::normalize_8u_min_max(src_channel, dest_channel, left_q, right_q);
        }
        else
        {
            dest_channel.assign(src_channel);
        }

        if(image.channels == 2)
        {
            if(image.band_index[i] == trace_x::BAND_GRAY)
            {
                normed.get_shared_channel(1).assign(dest_channel);
                normed.get_shared_channel(2).assign(dest_channel);
            }
        }
    }

    for(int i = 0; i < channel_widths.size(); ++i)
    {
        image.max_width += channel_widths[i];
    }

    image.max_width += image.channels + 1;

    image.max_width = qMax(image.max_width, QString::number(image.width).length() + 1);

    if(image.channels > 1)
    {
        // convert to QImage axes

        normed.permute_axes(AXES_CXYZ);
    }

    //

    QImage::Format qt_format;

    switch (image.channels)
    {
    case 1: qt_format = QImage::Format_Indexed8; break;
    case 3: qt_format = QImage::Format_RGB888; break;
    case 2:
    case 4: qt_format = QImage::Format_ARGB32; break;
    }

    QImage qimage(normed.data(), image.width, image.height, image.width * ((image.channels == 2) ? 4 : image.channels), qt_format);

    if(qt_format == QImage::Format_Indexed8)
    {
        qimage.setColorTable(::gray_palette());
    }

    qimage.detach();

    if((qt_format == QImage::Format_RGB888) && (image.band_index == BGR_ORDER))
    {
        // convert BGR to RGB

        qimage = qimage.rgbSwapped();
    }

    return qimage.copy();
}

QString get_permute_string(const QVector<quint8> &from, const QVector<quint8> &to)
{
    QString permute_string;

    QMap<quint8, quint8> from_axes_map;

    for(int i = 0; i < from.size(); ++i)
    {
        from_axes_map[from[i]] = i;
    }

    for(int i = 0; i < from.size(); ++i)
    {
        permute_string += AXES_XYZC[from_axes_map[to[i]]];
    }

    return permute_string;
}

template<class T>
void convert_axes(Image &image, const QVector<quint8> &dest_axes)
{
    X_CALL_F;

    if(image.axes != dest_axes)
    {
        cimg_library::CImg<T> c_img;

        QVector<quint64> sizes = QVector<quint64> () << image.channels << image.width << image.height << 1;
        QVector<quint64> dest_sizes(sizes.size());

        for(int i = 0; i < image.axes.size(); ++i)
        {
            dest_sizes[i] = sizes[image.axes[i]];
        }

        c_img.assign((const T*)(image.data_start), dest_sizes[0], dest_sizes[1], dest_sizes[2], dest_sizes[3], true);

        QString permute_string = get_permute_string(image.axes, dest_axes);

        X_INFO_F("permute_string = {}", permute_string);

        c_img.permute_axes(qPrintable(permute_string));

        image.axes = dest_axes;
    }
}

typedef void (*PrepareImage)(Image &image);

template<class T>
void prepare_image(Image &image)
{
    X_CALL_F;

    if(image.band_index.empty())
    {
        switch (image.channels)
        {
        case 1: image.band_index = GRAY; break;
        case 2: image.band_index = GRAY_ALPHA_ORDER; break;
        case 3: image.band_index = RGB_ORDER; break;
        case 4: image.band_index = RGBA_ORDER; break;
        }
    }

    if(!image.band_index.empty())
    {
        for(int i = 0; i < image.band_index.size(); ++i)
        {
            image.band_map[image.band_index[i]] = i;
        }
    }

    if(image.display_image.isNull())
    {
        convert_axes<T>(image, NATIVE_AXES);

        image.display_image = hist_quantile_normalization<T>(image, 0.02, 0.98);
    }
    else
    {
        // QImage already completed

        convert_axes<T>(image, NATIVE_AXES);
    }
}

GENERATE_FUNCTION_ACCESS(gen_prepare_image, PrepareImage, prepare_image)

Image get_image(const QByteArray &image_data)
{
    X_CALL_F;

    Image image;

    QDataStream stream(image_data);

    QVector<quint64> size;

    quint64 elem_size;
    quint64 byte_size;
    quint32 data_format;
    quint32 palette_size;

    stream >> size >> elem_size >> image.channels >> image.unit_type >> data_format >> image.band_index >> image.axes >> palette_size >> byte_size;

    if((size.size() != 2) || !size[0] || !size[1])
    {
        return image;
    }

    X_INFO_F("image band_index: {}", image.band_index);

    image.height = size[0];
    image.width = size[1];

    const char *palette_data = 0;

    if(palette_size)
    {
        palette_data = image_data.constData() + stream.device()->pos();
    }

    if(image.axes.isEmpty())
    {
        image.axes = QIMAGE_AXES;
    }

    image.data = image_data;
    image.data_start = image_data.constData() + palette_size + stream.device()->pos();

    if(data_format & trace_x::QT_IMAGE_FORMAT)
    {
        image.unit_type = trace_x::T_8U;

        QImage::Format q_format = QImage::Format(data_format & ~trace_x::QT_IMAGE_FORMAT);

        X_INFO_F("qt image format: {}", q_format);

        image.display_image = QImage((const uchar*)(image.data_start), image.width, image.height, byte_size / image.height, q_format);

        if(palette_size)
        {
            QVector<QRgb> q_palette(palette_size / sizeof(QRgb));

            memcpy(&q_palette[0], palette_data, palette_size);

            image.display_image.setColorTable(q_palette);
        }

        if((image.display_image.depth() == 1) ||
                (image.display_image.depth() != image.display_image.bitPlaneCount()) ||
                (image.display_image.format() == QImage::Format_ARGB32_Premultiplied) ||
                (image.display_image.format() == QImage::Format_RGBA8888_Premultiplied))
        {
            //we need convert packed and premultiplied pixels

            if(image.channels == 3)
            {
                image.display_image = image.display_image.convertToFormat(QImage::Format_RGB888);

                QByteArray new_data(image.width * image.height * 3, Qt::Uninitialized); // /0-terminated

                char *new_data_ptr = new_data.data();

                //3-channel image with gap! (32-bit aligned)

                for(quint32 i = 0; i < image.height; ++i)
                {
                    memcpy(new_data_ptr + i * image.width * 3,
                           image.display_image.constScanLine(i),
                           image.width * 3);
                }

                image.band_index = RGB_ORDER;

                image.data = new_data;
                image.data_start = new_data.constData();
            }
            else if(image.channels == 4)
            {
                image.shared_image = image.display_image.convertToFormat(QImage::Format_ARGB32);

                //                if(image.display_image.allGray())
                //                {
                //                    image.display_image = image.display_image.convertToFormat(QImage::Format_ARGB32, ::gray_palette());

                //                    image.channels = 4;

                //                    //? как поменять палитру?
                //                }
                //                else
                //                {
                //                    image.display_image = image.display_image.convertToFormat(QImage::Format_ARGB32);
                //                }

                image.band_index = BGRA_ORDER;

                // we unpack image, so replace data pointer
                image.data_start = image.shared_image.constBits();
                image.data = QByteArray();
                image.display_image = image.display_image.copy();
            }
        }
        else
        {
            switch (image.display_image.format())
            {
            case QImage::Format_RGB888 : image.band_index = RGB_ORDER; break;
            case QImage::Format_RGB32 :
            case QImage::Format_ARGB32 : image.band_index = BGRA_ORDER; break;
            case QImage::Format_RGBX8888 :
            case QImage::Format_RGBA8888 : image.band_index = RGBA_ORDER; break;
            default:
                break;
            }

            image.display_image = image.display_image.copy();
        }

        image.max_width = 3 * image.channels + image.channels - 1;
    }

    PrepareImage prepare_image_fp = gen_prepare_image(image.unit_type);

    prepare_image_fp(image);

    X_INFO_F("image band_map: {}", image.band_map);

    return image;
}

void save_image(const Image &image, const QString &save_file_name)
{
    X_CALL_F;

    X_VALUE_F(save_file_name);

    QString dest_format = QFileInfo(save_file_name).suffix();

    if((dest_format == "tif") || (dest_format == "tiff"))
    {
        SaveImage save_fun = gen_save_cimage(image.unit_type);

        save_fun(image, save_file_name);
    }
    else
    {
        image.display_image.save(save_file_name);
    }
}
