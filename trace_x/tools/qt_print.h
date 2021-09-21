#include "../detail/traits.h"

#ifdef QT_CORE_LIB

# include <QDebug>
# include <QString>

namespace trace_x_detail
{

template<typename T>
struct support_qdebug_operator : has_streaming_support<QDebug, T> {};

template<typename Char, typename T>
inline typename enable_if<!support_trace_operator<T>::value && !support_ostream_operator<T>::value && support_qdebug_operator<T>::value, basic_ostream<Char>&>::type
operator<<(basic_ostream<Char> &stream, const T &value)
{
    QString result;

    QDebug qstream(&result);

    qstream.nospace() << value;

    stream.ostream << result.toStdString();

    return stream;
}

/// registration for QDebug-supported types
template<typename T>
inline typename enable_if<!support_trace_operator<T>::value && !support_ostream_operator<T>::value && support_qdebug_operator<T>::value, basic_ostream_visitor&>::type
operator<<(basic_ostream_visitor &stream, const T &) { return stream; }

} // namespace trace_x_detail

#endif // #ifdef QT_CORE_LIB
