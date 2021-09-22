#define MODULE_NAME "qt4_test_app"

#include "trace_x/trace_x.h"

#include <QVector>
#include <QString>
#include <QImage>

#include <QDebug>

int main()
{
    X_INFO_F("qt {} {}", QT_VERSION_STR, QString("string"));

    QVector<float> vector;

    vector << 1.1 << 2.1 << 3.1;

    X_VALUE_F(vector);

    QImage qt4_image_1("://test_1");

    X_IMAGE_F(qt4_image_1);
}
