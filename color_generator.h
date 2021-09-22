#ifndef COLOR_GENERATOR_H
#define COLOR_GENERATOR_H

#include <QColor>
#include <QMutex>

struct ColorPair
{
    ColorPair() {}

    ColorPair(Qt::GlobalColor c1) : bg_color(c1), fg_color(Qt::black) {}
    ColorPair(Qt::GlobalColor c1, Qt::GlobalColor c2) : bg_color(c1), fg_color(c2) {}

    Qt::GlobalColor bg_color;
    Qt::GlobalColor fg_color;
};

class ColorGenerator
{
public:
    ColorGenerator(int seed = 0);

    ColorPair next();

    void reset();

private:
    int _current;

    QList<ColorPair> _color_list;
};

#endif // COLOR_GENERATOR_H
