#ifndef TRACE_X_TYPES_H
#define TRACE_X_TYPES_H

#ifdef __cplusplus

#include "api.h"

#include <sstream>
#include <stdint.h>
#include <vector>
#include <limits>
#include <memory>

namespace trace_x
{

class TRACE_X_API logger
{
public:
    logger();
    ~logger();

    //! file logger with default pattern
    static std::shared_ptr<logger> file(const std::string &filename, bool append = false, const std::string &logger_name = std::string());

    //! file logger with empty pattern
    static std::shared_ptr<logger> simple_file(const std::string &filename, bool append = false, const std::string &logger_name = std::string());

    static std::shared_ptr<logger> cout(bool colored = true, const std::string &logger_name = std::string());
    static std::shared_ptr<logger> cerr(bool colored = true, const std::string &logger_name = std::string());

    //! complex logger as union of other loggers
    static std::shared_ptr<logger> complex(const std::vector<std::shared_ptr<logger>> &loggers, const std::string &logger_name = std::string());

    //! logger to nowhere
    static std::shared_ptr<logger> null();

    void log(const char *data, size_t size, char level);

    void set_pattern(const std::string &pattern);

private:
    class logger_impl;
    logger_impl *_p;
};

typedef std::shared_ptr<logger> x_logger;

struct stream
{
    std::stringstream s;
};

class trace_stream_noop
{
public:
    template<class T> trace_stream_noop & operator << (const T &) { return *this; }
};

enum data_type_t
{
    T_CUSTOM = -1,
    T_8U     = 0,
    T_8S     = 1,
    T_16U    = 2,
    T_16S    = 3,
    T_32U    = 4,
    T_32S    = 5,
    T_64U    = 6,
    T_64S    = 7,
    T_32F    = 8,
    T_64F    = 9
};

static const char* data_type_names[] = { "8u", "8s", "16u", "16s", "32u", "32s", "64u", "64s", "32f", "64f"};
static const int_fast8_t data_type_byte_count[] = { 1, 1, 2, 2, 4, 4, 8, 8, 4, 8};

template<class T> struct type_of    { static const data_type_t type = T_CUSTOM; };
template<> struct type_of<bool>     { static const data_type_t type = T_8U; };
template<> struct type_of<uint8_t>  { static const data_type_t type = T_8U; };
template<> struct type_of<int8_t>   { static const data_type_t type = T_8S; };
template<> struct type_of<char>     { static const data_type_t type = std::numeric_limits<char>::is_signed ? T_8S : T_8U; };
template<> struct type_of<uint16_t> { static const data_type_t type = T_16U; };
template<> struct type_of<int16_t>  { static const data_type_t type = T_16S; };
template<> struct type_of<uint32_t> { static const data_type_t type = T_32U; };
template<> struct type_of<int32_t>  { static const data_type_t type = T_32S; };
template<> struct type_of<uint64_t> { static const data_type_t type = T_64U; };
template<> struct type_of<int64_t>  { static const data_type_t type = T_64S; };
template<> struct type_of<float>    { static const data_type_t type = T_32F; };
template<> struct type_of<double>   { static const data_type_t type = T_64F; };

enum data_format_t
{
    INVALID_FORMAT = 0x0,
    NUMERIC_FORMAT = 0x1,
    QT_IMAGE_FORMAT = 0x1000,
    USER_FORMAT = 0x10000,
};

enum image_band_type_t
{
    BAND_UNDEFINED = 0,
    BAND_GRAY      = 1,
    BAND_BINARY    = 2,
    BAND_RED       = 3,
    BAND_GREEN     = 4,
    BAND_BLUE      = 5,
    BAND_ALPHA     = 6

    //TODO HSV, CMYK, YUV and other?
};

template<class T>
T align_bits(T value)
{
    return value + value % 8;
}

template <typename T>
class make_vector
{
public:
    typedef make_vector<T> data_type;

    data_type& operator<< (const T& val)
    {
        _data.push_back(val);

        return *this;
    }

    operator std::vector<T>() const { return _data; }

private:
    std::vector<T> _data;
};

//! N-dimensional K-channel data array description with range support (subarray-s)
struct data_array_t
{
    data_array_t():
        data(0),
        palette(0),
        palette_size(0),
        channels(0),
        unit_type(T_CUSTOM),
        elem_bit_count(0),
        data_format(0)
    {
    }

    void assign_numeric_matrix(uint64_t width, uint64_t height, uint32_t channels,
                               int8_t unit_type, const void *data, uint64_t step = 0)
    {
        this->unit_type = unit_type;

        assign_matrix(width, height, channels, 0, data, NUMERIC_FORMAT, step);
    }

    // elem_size in bytes
    void assign_matrix(uint64_t width, uint64_t height, uint32_t channels,
                       uint32_t elem_size, const void *data, uint32_t data_format, uint64_t step = 0)
    {
        std::vector<uint64_t> size_v(2);

        size_v[0] = height;
        size_v[1] = width;

        if(step)
        {
            std::vector<uint64_t> step_v(2);

            step_v[0] = step;
            step_v[1] = elem_size;

            assign_array(size_v, this->unit_type, elem_size, data_format, data, channels, step_v);
        }
        else
        {
            assign_array(size_v, this->unit_type, elem_size, data_format, data, channels);
        }
    }

    //    void assign_cube(uint64_t width, uint64_t height, uint64_t bands,
    //                     uint8_t unit_type, uint32_t image_format,
    //                     const void *pdata, uint32_t channels = 1)
    //    {
    //        assign_array({bands, height, width}, unit_type, image_format, pdata, channels);
    //    }

    //! first size - last(higest, deepest) dimension
    //! first step - size in bytes for one band in N-dimension data array. Last step defined in bits.
    //! elem_size - size in bytes for single K-channel element (if unit_type is T_CUSTOM)
    void assign_array(const std::vector<uint64_t> &size, int8_t unit_type, uint64_t elem_size, uint32_t data_format,
                      const void *data, uint32_t channels = 1, const std::vector<uint64_t> &step = std::vector<uint64_t>())
    {
        this->data = data;
        this->unit_type = unit_type;
        this->size = size;
        this->channels = channels;
        this->elem_bit_count = unit_type == T_CUSTOM ? elem_size : 8 * data_type_byte_count[unit_type] * channels;
        this->data_format = data_format;
        this->step = step;

        //this->step.resize(size.size());

        //        if(!step.empty())
        //        {
        //        }
        //        else if(unit_type != T_CUSTOM)
        //        {
        //            //fill steps automatically ?? зачем собсна ? если данные непрерывны, то передаём не парясь и усё.

        //            this->step[step.size() - 1] = elem_size;

        //            for(intptr_t i = this->step.size() - 2; i >= 0; i--)
        //            {
        //                this->step[i] = this->step[i + 1] * this->size[i + 1];
        //            }
        //        }
    }

    void set_band_map(const std::vector<uint8_t> &band_map)
    {
        this->band_map = band_map;
    }

    void set_axes(const std::vector<uint8_t> &axes)
    {
        this->axes = axes;
    }

    void set_palette(const void *data, uint32_t size)
    {
        this->palette = data;
        this->palette_size = size;
    }

    bool is_continuous() const
    {
        if(!step.empty())
        {
            for(size_t i = 0; i < this->step.size() - 1; ++i)
            {
                if(step[i] != align_bits(size[i] * elem_bit_count) / 8)
                {
                    return false;
                }
            }
        }

        return true;
    }

    uint64_t buffer_size() const
    {
        return step.empty() ? data_byte_size() : size.back() * step[0];
    }

    uint64_t data_byte_size() const
    {
        uint64_t total_size = 1;

        for(size_t i = 0; i < this->size.size(); ++i)
        {
            total_size *= this->size[i];
        }

        return align_bits(total_size * this->elem_bit_count) / 8;
    }

public:
    const void *data; //! pointer to data array

    const void *palette; //! pointer to palette array (used for indexed data)
    uint32_t palette_size; //!size of palette data array in bytes

    uint32_t channels; //! number of units in element
    int8_t unit_type; //! type of single unit in element
    uint64_t elem_bit_count; //! element size in bits (depth)

    uint32_t data_format; //! format of data array

    std::vector<uint64_t> size; //! size[i] - number of elements in dimension # i
    std::vector<uint64_t> step; //! step[i] - "jump" in bytes to next "band" in dimension # i (pitch, stride)
    std::vector<uint8_t> band_map; //! types of each channel in element(red, green, blue and other)
    std::vector<uint8_t> axes; //! axes order: 0-channel, 1 - x, 2 - y, 3 - z, ... (0,1,2,3 by default)
};

//! class for user data streaming support
struct data_array_stream
{
    template<class T> data_array_stream(const T &data_object)
    {
        *this << data_object;
    }

    data_array_t data;
};

//! class for image streaming support
struct image_stream
{
    template<class T> image_stream(const T &data_object)
    {
        *this << data_object;
    }

    data_array_t data;
};

}

#endif // #ifdef __cplusplus

#endif //TRACE_X_TYPES_H
