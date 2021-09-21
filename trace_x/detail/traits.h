#ifndef TRACE_X_TRAITS
#define TRACE_X_TRAITS

#include <ostream>

#include "types.h"

namespace trace_x_detail
{

template <bool, typename T> struct enable_if { };
template <typename T> struct enable_if<true, T> { typedef T type; };

struct anyx
{
    template <class T>
    anyx(const T &);
};

typedef char yes;
typedef char (&no)[2];

no operator << (const anyx &, const anyx &);

template <class T> yes detect(T const&);
no detect(no);

template <typename StreamType, typename T>
struct has_streaming_support
{
    static StreamType &stream;
    static T &x;
    static const bool value = (sizeof(detect(stream << x)) == sizeof(yes));
};

template<typename T>
struct support_ostream_operator : has_streaming_support<std::ostream, T> {};

template<typename T>
struct support_trace_operator : has_streaming_support<trace_x::stream, T> {};

} // namespace trace_x_detail

#endif // TRACE_X_TRAITS
