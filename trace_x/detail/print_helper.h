#ifndef _TRACE_X_PRINT_HELPER_
#define _TRACE_X_PRINT_HELPER_

#include <ostream>
#include <sstream>
#include <typeinfo>

#include "types.h"
#include "traits.h"

#include <memory>

#ifndef _WIN32

#include <cxxabi.h>

#endif

namespace trace_x_detail
{

inline std::string type_name_demangle(const char *mangled)
{
 #ifdef _WIN32

    return mangled;

#else

    return mangled;

    // TODO
//    int status;

//    std::unique_ptr<char[], void (*)(void*)> result(abi::__cxa_demangle(mangled, 0, 0, &status), std::free);

//    return result.get() ? std::string(result.get()) : "unkonwn class";
#endif
}

template<typename Char>
struct basic_ostream
{
    basic_ostream(std::basic_streambuf<Char> *buf):
        ostream(buf)
    {
    }

    std::basic_iostream<Char> ostream;
};

/// helper struct for type streaming registration (for support types, which not supports any print operator)
struct basic_ostream_visitor {};

/// operator<< for types, which supports trace_x::stream operator

template<typename Char, typename T>
inline typename enable_if<support_trace_operator<T>::value, basic_ostream<Char>&>::type
operator<<(basic_ostream<Char> &stream, const T &value)
{
    trace_x::stream xstream;

    xstream << value;

    stream.ostream << xstream.s.str();

    return stream;
}

template<typename T>
inline typename enable_if<support_trace_operator<T>::value, basic_ostream_visitor&>::type
operator<<(basic_ostream_visitor &stream, const T &) { return stream; }

/// operator<< for types, which NOT supports trace_x::stream operator, BUT supports std::ostream

template<typename Char, typename T>
inline typename enable_if<!support_trace_operator<T>::value && support_ostream_operator<T>::value, basic_ostream<Char>&>::type
operator<<(basic_ostream<Char> &stream, const T &value)
{
    stream.ostream << value;

    return stream;
}

template<typename T>
inline typename enable_if<!support_trace_operator<T>::value && support_ostream_operator<T>::value, basic_ostream_visitor&>::type
operator<<(basic_ostream_visitor &stream, const T &) { return stream; }

/// operator<< for types, which NOT supports any print operator

template<typename Char, typename T>
inline typename enable_if<!has_streaming_support<basic_ostream_visitor, T>::value, basic_ostream<Char>&>::type
operator<<(basic_ostream<Char> &stream, const T &value)
{
    //static_assert(has_streaming_support<trace_x_detail::basic_ostream<Char>, T>::value, "This type is not streamable");

    stream.ostream << "object of  " << type_name_demangle(typeid(T).name()) << " " << (void*)(&value);

    return stream;
}

} // namespace trace_x_detail

#endif // #ifndef _TRACE_X_PRINT_HELPER_
