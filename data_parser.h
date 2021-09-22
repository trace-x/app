#ifndef DATA_PARSER_H
#define DATA_PARSER_H

#include <stdint.h>
#include <string>

#include <QString>

template<class T>
inline T get_value(const char *data, size_t &offset)
{
    T result = *((T*)(data + offset));
    offset += sizeof(T);

    return result;
}

template<class T>
inline std::vector<T> get_vector(const char *data, size_t &offset)
{
    uint64_t size = get_value<uint64_t>(data, offset);

    std::vector<T> data_v((T*)(data + offset), (T*)(data + offset) + size);

    offset += size * sizeof(T);

    return std::move(data_v);
}

inline void parse_string(const char *data, const char **string, size_t &size, size_t &offset)
{
    size = get_value<uint32_t>(data, offset);

    *string = (const char *)(data + offset);

    offset += size;
}

inline QString get_string(const QByteArray &frame, size_t &offset, bool at_end = false)
{
    QString result;

    int size = 0;

    if(at_end)
    {
        size = frame.size() - int(offset);
    }
    else
    {
        size = get_value<uint32_t>(frame, offset);
    }

    result = QString::fromLocal8Bit(frame.data() + offset, size);

    offset += result.size();

    return result;
}

inline std::string get_std_string(const QByteArray &frame, size_t &offset, bool at_end = false)
{
    std::string result;

    size_t size = 0;

    if(at_end)
    {
        size = frame.size() - offset;
    }
    else
    {
        size = get_value<uint32_t>(frame, offset);
    }

    result = std::string(frame.data() + offset, size);

    offset += result.size();

    return result;
}

#endif // DATA_PARSER_H

