#ifndef IMAGE_H
#define IMAGE_H

#include <QByteArray>
#include <QImage>
#include <QMap>

//! macro for generation template functions
#define GENERATE_FUNCTION_ACCESS(gn, fp, fn)            \
    fp gn(uint8_t unit_type)                            \
    {                                                   \
        static fp mi_tab[] =                            \
        {                                               \
            fn<uint8_t>,  fn<int8_t>,                   \
            fn<uint16_t>, fn<int16_t>,                  \
            fn<uint32_t>, fn<int32_t>,                  \
            fn<uint64_t>, fn<int64_t>,                  \
            fn<float>,    fn<double>, 0                 \
        };                                              \
                                                        \
        fp func = mi_tab[unit_type];                    \
        return func;                                    \
    }


QByteArray make_image_array(const char *data, QString &description);

struct Image
{
    Image() :
        width(0),
        height(0),
        channels(0),
        unit_type(0),
        data_start(0),
        max_width(0)
    {}

    quint32 width;
    quint32 height;
    quint32 channels;
    qint8   unit_type;

    QVector<quint8> band_index;
    QVector<quint8> axes;

    QMap<quint8, quint8> band_map;

    const void *data_start;

    QByteArray data;
    QImage display_image;
    QImage shared_image; //image for data sharing(instead of QByteArray data)

    int max_width;
};

Image get_image(const QByteArray &image_data);

void save_image(const Image &image, const QString &save_file_name);

#endif // IMAGE_H
