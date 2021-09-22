#include "color_generator.h"

ColorGenerator::ColorGenerator(int seed):
    _current(seed)
{
    _color_list << Qt::white
                << Qt::lightGray
                << ColorPair(Qt::red, Qt::white)
                << Qt::green
                << ColorPair(Qt::blue, Qt::white)
                << Qt::cyan
                << ColorPair(Qt::magenta, Qt::white)
                << Qt::yellow
                << ColorPair(Qt::darkRed, Qt::white)
                << ColorPair(Qt::darkGreen, Qt::white)
                << ColorPair(Qt::darkBlue, Qt::white)
                << ColorPair(Qt::darkCyan, Qt::white)
                << ColorPair(Qt::darkMagenta, Qt::white)
                << ColorPair(Qt::darkYellow, Qt::white)
                << ColorPair(Qt::darkGray, Qt::white);
}

ColorPair ColorGenerator::next()
{
    ColorPair color = _color_list.at(_current);

    _current = (_current + 1) % _color_list.size();

    return color;
}

void ColorGenerator::reset()
{
    _current = 0;
}
