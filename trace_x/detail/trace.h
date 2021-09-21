//
// Copyright(c) 2014-2016 Nikita Kozhanov.
//

#ifndef TRACE_X_TRACE_H
#define TRACE_X_TRACE_H

#include "api.h"

#ifndef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "undefined_module"
#endif

#define SELECT(NAME, NUM) FMT_CONCAT(NAME ## _, NUM)

#define VA_SELECT(NAME, ...) FMT_EXPAND(SELECT(NAME, FMT_NARG(__VA_ARGS__))(__VA_ARGS__))

#define TRACE_X_ARG_FORMAT_STRINGINIZER(...) FMT_EXPAND(VA_SELECT(TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL, __VA_ARGS__))

#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(X) #X" = {}"
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_2(_1, _2) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_2)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_3(_1, _2, _3) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_2(_2, _3)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_4(_1, _2, _3, _4) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_3(_2, _3, _4)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_5(_1, _2, _3, _4, _5) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_4(_2, _3, _4, _5)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_6(_1, _2, _3, _4, _5, _6) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_5(_2, _3, _4, _5, _6)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_7(_1, _2, _3, _4, _5, _6, _7) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_6(_2, _3, _4, _5, _6, _7)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_8(_1, _2, _3, _4, _5, _6, _7, _8) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_7(_2, _3, _4, _5, _6, _7, _8)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_8(_2, _3, _4, _5, _6, _7, _8, _9)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_9(_2, _3, _4, _5, _6, _7, _8, _9, _10)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_10(_2, _3, _4, _5, _6, _7, _8, _9, _10, _11)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_11(_2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_12(_2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_13(_2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_15(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_14(_2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15)
#define TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_16(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_1(_1) ";\n" TRACE_X_ARG_FORMAT_STRINGINIZER_IMPL_15(_2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16)

#ifdef __cplusplus

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif

#include "stl_print.h"
#include "print_helper.h"
#include "types.h"

#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/printf.h"

#ifdef QT_CORE_LIB
#include "../tools/qt_print.h"
#endif

#ifdef QT_GUI_LIB
#include "../tools/qt_image_trace_x.h"
#endif

#endif //__cplusplus

#define TRACE_X_FUNCTION_UNKNOWN "<unknown>()"
#define TRACE_X_FILE_UNKNOWN "<unknown>"

///////////////////////////////////////////////// TRACE_X_CURRENT_FUNCTION definition //////////////////////////////////

#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)

# define TRACE_X_CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__DMC__) && (__DMC__ >= 0x810)

# define TRACE_X_CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__FUNCSIG__)

# define TRACE_X_CURRENT_FUNCTION __FUNCSIG__

#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))

# define TRACE_X_CURRENT_FUNCTION __FUNCTION__

#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)

# define TRACE_X_CURRENT_FUNCTION __FUNC__

#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)

# define TRACE_X_CURRENT_FUNCTION __func__

#elif defined(__cplusplus) && (__cplusplus >= 201103)

# define TRACE_X_CURRENT_FUNCTION __func__

#else

# define TRACE_X_CURRENT_FUNCTION "(unknown)"

#endif //#if defined(__GNUC__) ...

#ifndef TRACE_CONTEXT
# define TRACE_CONTEXT this
#endif

#ifdef __cplusplus

  #define TRACE_X_FBODY TRACE_X_MODULE_NAME, sizeof(TRACE_X_MODULE_NAME), __FILE__, sizeof(__FILE__), __LINE__, TRACE_X_CURRENT_FUNCTION, sizeof(TRACE_X_CURRENT_FUNCTION), nullptr
  #define TRACE_X_FCHECK_BODY TRACE_X_MODULE_NAME, __FILE__, TRACE_X_CURRENT_FUNCTION, nullptr

#else

  #define TRACE_X_FBODY TRACE_X_MODULE_NAME, sizeof(TRACE_X_MODULE_NAME), __FILE__, sizeof(__FILE__), __LINE__, TRACE_X_CURRENT_FUNCTION, sizeof(TRACE_X_CURRENT_FUNCTION), (const void *)(0)
  #define TRACE_X_FCHECK_BODY TRACE_X_MODULE_NAME, __FILE__, TRACE_X_CURRENT_FUNCTION, (const void *)(0)

#endif

// helper macro for "macro-overriding"
#define TRACE_X_N_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,NAME, ...) NAME

// macro for "macro-overriding" (1 and n-arguments) (n = 2..15)
#define TRACE_X_1N_MACRO(mn, m1, ...) FMT_EXPAND(TRACE_X_N_MACRO(__VA_ARGS__,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,m1)(__VA_ARGS__))

// macro for "macro-overriding" (1, 2 and n-arguments) (n = 3..14)
#define TRACE_X_12N_MACRO(mn, m2, m1, ...) FMT_EXPAND(TRACE_X_N_MACRO(__VA_ARGS__,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,m2,m1)(__VA_ARGS__))

// macro for "macro-overriding" (1, 2, 3 and n-arguments) (n = 4..13)
#define TRACE_X_123N_MACRO(mn, m3, m2, m1, ...) FMT_EXPAND(TRACE_X_N_MACRO(__VA_ARGS__,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,mn,m3,m2,m1)(__VA_ARGS__))

#define TRACE_X_CALL trace_x::TraceScope exec_object(trace_x::message_context_t{TRACE_X_BODY, "", 1, 0, trace_x::trace_timestamp(), 0})
#define TRACE_X_CALL_F trace_x::TraceScope exec_object(trace_x::message_context_t{TRACE_X_FBODY, "", 1, 0, trace_x::trace_timestamp(), 0})

#define TRACE_X_BODY TRACE_X_MODULE_NAME, sizeof(TRACE_X_MODULE_NAME), __FILE__, sizeof(__FILE__), __LINE__, TRACE_X_CURRENT_FUNCTION, sizeof(TRACE_X_CURRENT_FUNCTION), TRACE_CONTEXT
#define TRACE_X_CHECK_BODY TRACE_X_MODULE_NAME, __FILE__, TRACE_X_CURRENT_FUNCTION, TRACE_CONTEXT

#define TRACE_X_PRINTF(_type_, ...) trace_message(TRACE_X_FBODY, "", 1, _type_, __VA_ARGS__)

#ifdef __cplusplus

/// fmtlib formatting
#define TRACE_X_TRACE_M(_type_, _m_, ...) trace_x::make_stream<trace_x::fmt_writer_t>(trace_x::message_context_t{TRACE_X_ ##_m_ ##BODY, "", 1, _type_, trace_x::trace_timestamp(), 0}, ##__VA_ARGS__)

#define TRACE_X_TRACE(_type_, ...) trace_x::make_stream<trace_x::fmt_writer_t>(trace_x::message_context_t{TRACE_X_BODY, "", 1, _type_, trace_x::trace_timestamp(), 0}, ##__VA_ARGS__)
#define TRACE_X_TRACE_F(_type_, ...) trace_x::make_stream<trace_x::fmt_writer_t>(trace_x::message_context_t{TRACE_X_FBODY, "", 1, _type_, trace_x::trace_timestamp(), 0}, ##__VA_ARGS__)

/// printf formatting
#define TRACE_X_TRACE_S_M(_type_, _m_, ...) trace_x::make_stream<trace_x::printf_writer_t>(trace_x::message_context_t{TRACE_X_ ##_m_ ##BODY, "", 1, _type_, trace_x::trace_timestamp(), 0}, ##__VA_ARGS__)

#define TRACE_X_TRACE_S(_type_, ...) trace_x::make_stream<trace_x::printf_writer_t>(trace_x::message_context_t{TRACE_X_BODY, "", 1, _type_, trace_x::trace_timestamp(), 0}, ##__VA_ARGS__)
#define TRACE_X_TRACE_S_F(_type_, ...) trace_x::make_stream<trace_x::printf_writer_t>(trace_x::message_context_t{TRACE_X_FBODY, "", 1, _type_, trace_x::trace_timestamp(), 0}, ##__VA_ARGS__)

#define TRACE_X_IMAGE_(_image_, _image_name_, _message_, _m_) { int ____i_____; if(trace_x::trace_check(TRACE_X_ ##_m_ ##CHECK_BODY, trace_x::MESSAGE_IMAGE, &____i_____)) trace_x::trace_data(trace_x::message_context_t{TRACE_X_ ##_m_ ##BODY, _image_name_, sizeof(_image_name_), trace_x::MESSAGE_IMAGE, trace_x::trace_timestamp(), 0}, _message_, trace_x::image_stream(_image_).data , ____i_____); }

#define TRACE_X_IMAGE_1_(_image_) TRACE_X_IMAGE_(_image_, #_image_, "", )
#define TRACE_X_IMAGE_2_(_image_, _image_name_) TRACE_X_IMAGE_(_image_, _image_name_, "", )
#define TRACE_X_IMAGE_3_(_image_, _image_name_, _message_) TRACE_X_IMAGE_(_image_, _image_name_, _message_, )
#define TRACE_X_IMAGE_F_(_image_, _image_name_, _format_, ...) TRACE_X_IMAGE_(_image_, _image_name_, fmt::format(_format_, __VA_ARGS__), )

#define TRACE_X_F_IMAGE_1_(_image_) TRACE_X_IMAGE_(_image_, #_image_, "", F)
#define TRACE_X_F_IMAGE_2_(_image_, _image_name_) TRACE_X_IMAGE_(_image_, _image_name_, "", F)
#define TRACE_X_F_IMAGE_3_(_image_, _image_name_, _message_) TRACE_X_IMAGE_(_image_, _image_name_, _message_, "", F)
#define TRACE_X_F_IMAGE_F_(_image_, _image_name_,  _format_, ...) TRACE_X_IMAGE_(_image_, _image_name_, fmt::format(_format_, __VA_ARGS__), F)

#define TRACE_X_ASSERT_BODY(_x_, _m_) if(!(_x_)) TRACE_X_TRACE_M(trace_x::MESSAGE_ASSERT, _m_, "Assertion failed [" #_x_ "]")

#define TRACE_X_GUID_BODY(_x_, _m_) TRACE_X_TRACE_S_M(trace_x::MESSAGE_INFO, _m_, "GUID={%.8x-%.4hx-%.4hx-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}\n", _x_.Data1, _x_.Data2, _x_.Data3, (ULONG)_x_.Data4[0], (ULONG)_x_.Data4[1], (ULONG)_x_.Data4[2], (ULONG)_x_.Data4[3], (ULONG)_x_.Data4[4], (ULONG)_x_.Data4[5], (ULONG)_x_.Data4[6], (ULONG)_x_.Data4[7])

#define TRACE_X_INFO_VALUE_BODY(_x_, _printspec_, _m_) trace_x::trace_message(TRACE_X_ ##_m_ ##BODY, #_x_, sizeof(#_x_), trace_x::MESSAGE_VALUE, _printspec_, _x_)
#define TRACE_X_VALUE_F_BODY(_name_, _format_, _m_, ...) { int ____i_____; if(trace_x::trace_check(TRACE_X_ ##_m_ ##CHECK_BODY, trace_x::MESSAGE_VALUE, &____i_____)) trace_x::trace_message_s(TRACE_X_ ##_m_ ##BODY, _name_, sizeof(_name_), trace_x::MESSAGE_VALUE, fmt::format(_format_, __VA_ARGS__), ____i_____); }
#define TRACE_X_VALUE_BODY(_name_, _x_, _m_) { int ____i_____; if(trace_x::trace_check(TRACE_X_ ##_m_ ##CHECK_BODY, trace_x::MESSAGE_VALUE, &____i_____)) trace_x::trace_message_s(TRACE_X_ ##_m_ ##BODY, _name_, sizeof(_name_), trace_x::MESSAGE_VALUE, fmt::format("{}", _x_), ____i_____); }

#define TRACE_X_VALUE_1_(_x_) TRACE_X_VALUE_BODY(#_x_, _x_, )
#define TRACE_X_F_VALUE_1_(_x_) TRACE_X_VALUE_BODY(#_x_, _x_, F)
#define TRACE_X_VALUE_2_(_alias_, _x_) TRACE_X_VALUE_BODY(_alias_, _x_, )
#define TRACE_X_F_VALUE_2_(_alias_, _x_) TRACE_X_VALUE_BODY(_alias_, _x_, F)
#define TRACE_X_VALUE_F_(_alias_, _format_, ...) TRACE_X_VALUE_F_BODY(_alias_, _format_, , __VA_ARGS__)
#define TRACE_X_F_VALUE_F_(_alias_, _format_, ...) TRACE_X_VALUE_F_BODY(_alias_, _format_, F, __VA_ARGS__)

#define TRACE_X_TRACE_T(...) fmt::format(TRACE_X_ARG_FORMAT_STRINGINIZER(__VA_ARGS__), __VA_ARGS__)

#define TRACE_X_CAPTURE_ARRAY_1(array) trace_x_detail::print_array(array, sizeof(array) / sizeof(array[0]))
#define TRACE_X_CAPTURE_ARRAY_2(array, size) trace_x_detail::print_array(array, size)

#define TRACE_X_CAPTURE_ARRAY(...) TRACE_X_1N_MACRO(TRACE_X_CAPTURE_ARRAY_2, TRACE_X_CAPTURE_ARRAY_1, __VA_ARGS__)

namespace trace_x
{

#endif

//! Message types enum
enum MessageType
{
    MESSAGE_CALL,
    MESSAGE_RETURN,
    MESSAGE_INFO,
    MESSAGE_DEBUG,
    MESSAGE_IMPORTANT,
    MESSAGE_EVENT,
    MESSAGE_WARNING,
    MESSAGE_ERROR,
    MESSAGE_EXCEPTION,
    MESSAGE_ASSERT,
    MESSAGE_PARAMETERS,
    MESSAGE_VALUE,

    MESSAGE_SUSPEND,
    MESSAGE_RESUME,
    MESSAGE_SIGNAL,
    MESSAGE_DATA,
    MESSAGE_IMAGE,
    MESSAGE_CONNECTED,
    MESSAGE_DISCONNECTED,
    MESSAGE_CRASH,

    _MESSAGE_END_
};

enum ReceiverLogMode
{
    MODE_NO_SAVE,
    MODE_SAVE_CRASH,
    MODE_SAVE_ALL
};

enum CommandType
{
    COMMAND_STOP,
    COMMAND_SHOW
};

#ifdef __cplusplus

extern "C"
{

#endif

/// C/C++ API functions

TRACE_X_API
void trace_message(const char *module, size_t module_len, const char *source, size_t source_len,
                   int line, const char *function, size_t function_len,
                   const void *context, const char *label, size_t label_len, char message_type, const char *format, ...);

//! return true, if this message is allowed to transmitting. Used for lazy streaming.
TRACE_X_API
int trace_check(const char *module, const char *source, const char *function, const void *context, char message_type, int *status);

//! reconnect static transmitter to server with ID = server_name
TRACE_X_API
void trace_connect(const char *server_name);

//! send COMMAND_STOP to receiver with ID = server_name
TRACE_X_API
void trace_stop(const char *server_name);

//! return 1 if server with ID = server_name is runned(accepted connections)
TRACE_X_API
int is_server_runned(const char *server_name);

TRACE_X_API
void show_trace_receiver();

TRACE_X_API
void send_command(uint8_t command_code);

TRACE_X_API
uint64_t trace_timestamp();

#ifdef __cplusplus

}

#endif /// closing extern "C"

////////////////////////////////

#ifdef __cplusplus

TRACE_X_API x_logger cout();
TRACE_X_API logger * cerr();

TRACE_X_API void log_cerr(const std::string &message, char level);

TRACE_X_API extern const int x_intercept_index;
TRACE_X_API extern int x_disable_auto_connect;

//! Message context data
struct message_context_t
{
    const char *module;
    size_t module_len;
    const char *source;
    size_t source_len;
    int line;
    const char *function;
    size_t function_len;
    const void *object;
    const char *label;
    size_t label_len;
    char message_type;

    uint64_t timestamp;
    uint64_t extra_timestamp;

    message_context_t & typed(char type)
    {
        message_type = type;

        return *this;
    }

    bool is_critical() const
    {
        return (message_type == MESSAGE_ERROR) ||
                (message_type == MESSAGE_EXCEPTION) ||
                (message_type == MESSAGE_ASSERT);
    }
};

//! Class for CALL/RET message
class TRACE_X_API TraceScope
{
public:
    TraceScope(const message_context_t &context);
    TraceScope(message_context_t &&context);

    ~TraceScope();

private:
    message_context_t _context;
};

/// C++ API functions

TRACE_X_API
void trace_message(const message_context_t &context, const char *message, size_t message_size, int status);

TRACE_X_API
void trace_message(const message_context_t &context, const char *message, size_t message_size);

TRACE_X_API
void trace_data(const message_context_t &context, const std::string &message, const trace_x::data_array_t &data, int status);

TRACE_X_API
int trace_check(const message_context_t &context, int *status);

TRACE_X_API
void start_trace_receiver(int log_mode = MODE_NO_SAVE, const char *log_dir = "", const char *server_name = 0);

// TODO OFF support
TRACE_X_API
void set_trace_logger(const x_logger &logger = x_logger());

// TODO remove
TRACE_X_API
void trace_message_s(const char *module, size_t module_len, const char *source, size_t source_len,
                     int line, const char *function, size_t function_len,
                     const void *context, const char *label, size_t label_len, char message_type, const std::string &message,
                     int status);

///////////////////////////////////////////////////////////////

struct printf_writer_t
{
    static const char *single_arg_format()
    {
        return "%s";
    }

    static void write(fmt::MemoryWriter &writer, fmt::CStringRef format, fmt::ArgList args)
    {
        fmt::printf(writer, format, args);
    }

    FMT_VARIADIC(static void, write, fmt::MemoryWriter&, fmt::CStringRef)
};

struct fmt_writer_t
{
    static const char *single_arg_format()
    {
        return "{}";
    }

    static void write(fmt::MemoryWriter &writer, fmt::CStringRef format, fmt::ArgList args)
    {
        writer.write(format, args);
    }

    FMT_VARIADIC(static void, write, fmt::MemoryWriter&, fmt::CStringRef)
};

///////////////////////////////////////////////////////////////

template<class Sink>
struct sink_helper
{
    typedef Sink& sink_type;

    static void write(sink_type dest, const char *data, size_t size, char)
    {
        fwrite(data, 1, size, dest);
        fprintf(dest, "\n");
    }
};

struct cfile_write_helper_t
{
    typedef FILE* sink_type;

    static void write(sink_type dest, const char *data, size_t size, char)
    {
        fwrite(data, 1, size, dest);
        fprintf(dest, "\n");
    }
};

struct log_write_helper_t
{
    typedef x_logger& sink_type;

    static void write(sink_type dest, const char *data, size_t size, char message_type)
    {
        dest->log(data, size, message_type);
    }
};

struct ostream_write_helper_t
{
    typedef std::ostream& sink_type;

    static void write(sink_type dest, const char *data, size_t size, char)
    {
        dest.iword(x_intercept_index) = 0;

        dest.write(data, size);
        dest << std::endl;

        dest.iword(x_intercept_index) = 1;
    }
};

template<class Sink>
struct trace_x_write_helper_t
{
    typedef Sink& sink_type;

    static void write(sink_type dest, const char *data, size_t size, char message_type)
    {
        dest.trace(0, 0);
    }
};

typedef message_context_t&& message_context_ref;

//class trace_factory
//{
//public:
////    //! Multiple arguments
////    template<class Writer, typename... Args>
////    simple_stream<Writer>
////    operator () (message_context_ref context, fmt::CStringRef format, const Args & ... args)
////    {
////        return simple_stream<Writer>(std::move(context), format, args...);
////    }

////    trace_factory & module(const char *module)
////    {
////    }
//};

// X_INFO.module("m_1").label("l1")("lalala") <<

class trace_base
{
public:
    trace_base(message_context_ref context):
        _context(std::move(context)),
        _check_status(-1),
        _write_template("{} ")
    {}

    trace_base(trace_base &&other):
        _context(std::move(other._context)),
        _writer(std::move(other._writer)),
        _check_status(other._check_status),
        _write_template(other._write_template)
    {}

    template<class T>
    trace_base & operator << (const T &value)
    {
        if(_check_status != -1)
        {
            _writer.write(_write_template, value);
        }

        return *this;
    }

    trace_base & no_space()
    {
        _write_template = "{}";

        return *this;
    }

protected:
    message_context_t _context;
    fmt::MemoryWriter _writer;
    int _check_status;
    const char *_write_template;
};

template<class Writer>
class simple_stream : public trace_base
{
public:
    template <typename... Args>
    simple_stream(message_context_ref context, fmt::CStringRef format, const Args & ... args):
        trace_base(std::move(context))
    {
        if(trace_check(context.module, context.source, context.function, context.object, context.message_type, &_check_status) && format.c_str())
        {
            Writer::write(_writer, format, args...);
        }
    }

    simple_stream(simple_stream &&other):
        trace_base(std::move(other))
    {}

    ~simple_stream()
    {
        trace_message(_context, _writer.data(), _writer.size(), _check_status);
    }
};

template<class Writer, class SinkWriter>
class extra_stream : public trace_base
{
public:
    template <typename... Args>
    extra_stream(message_context_ref context, typename SinkWriter::sink_type sink, fmt::CStringRef format, const Args & ... args):
        trace_base(std::move(context)),
        _sink(sink)
    {
        _check_status = 0;

        if(format.c_str())
        {
            Writer::write(_writer, format, args...);
        }
    }

    ~extra_stream()
    {
        SinkWriter::write(_sink, _writer.data(), _writer.size(), _context.message_type);

        if(trace_check(_context.module, _context.source, _context.function, _context.object, _context.message_type, &_check_status))
        {
            trace_message(_context, _writer.data(), _writer.size(), _check_status);
        }
    }

    extra_stream(extra_stream &&other):
        trace_base(std::move(other)),
        _sink(other._sink)
    {
    }

private:
    typename SinkWriter::sink_type _sink;
};

// TODO bottom stuff pack to macro

//! Multiple arguments
template<class Writer, typename... Args>
simple_stream<Writer>
make_stream(message_context_ref context, fmt::CStringRef format, const Args & ... args)
{
    return simple_stream<Writer>(std::move(context), format, args...);
}

//! Single argument
template<class Writer, typename Arg>
simple_stream<Writer>
make_stream(message_context_ref context, const Arg &arg)
{
    return simple_stream<Writer>(std::move(context), Writer::single_arg_format(), arg);
}

//! Without arguments
template<class Writer>
simple_stream<Writer>
make_stream(message_context_ref context)
{
    return simple_stream<Writer>(std::move(context), nullptr);
}

/// With writing to logger

//! Multiple arguments
template<class Writer, typename... Args>
extra_stream<Writer, log_write_helper_t>
make_stream(message_context_ref context, x_logger &logger, fmt::CStringRef format, const Args & ... args)
{
    return extra_stream<Writer, log_write_helper_t>(std::move(context), logger, format, args...);
}

//! Single argument
template<class Writer, typename Arg>
extra_stream<Writer, log_write_helper_t>
make_stream(message_context_ref context, x_logger &logger, const Arg &arg)
{
    return extra_stream<Writer, log_write_helper_t>(std::move(context), logger, "{}", arg);
}

//! Without arguments
template<class Writer>
extra_stream<Writer, log_write_helper_t>
make_stream(message_context_ref context, x_logger &logger)
{
    return extra_stream<Writer, log_write_helper_t>(std::move(context), logger, nullptr);
}

/// With writing to FILE*

//! Multiple arguments
template<class Writer, typename... Args>
extra_stream<Writer, cfile_write_helper_t>
make_stream(message_context_ref context, FILE *file, fmt::CStringRef format, const Args & ... args)
{
    return extra_stream<Writer, cfile_write_helper_t>(std::move(context), file, format, args...);
}

//! Single argument
template<class Writer, typename Arg>
extra_stream<Writer, cfile_write_helper_t>
make_stream(message_context_ref context, FILE *file, const Arg &arg)
{
    return extra_stream<Writer, cfile_write_helper_t>(std::move(context), file, "{}", arg);
}

//! Without arguments
template<class Writer>
extra_stream<Writer, cfile_write_helper_t>
make_stream(message_context_ref context, FILE *file)
{
    return extra_stream<Writer, cfile_write_helper_t>(std::move(context), file, nullptr);
}

/// With writing to ostream

//! Multiple arguments
template<class Writer, typename... Args>
extra_stream<Writer, ostream_write_helper_t>
make_stream(message_context_ref context, std::ostream &stream, fmt::CStringRef format, const Args & ... args)
{
    return extra_stream<Writer, ostream_write_helper_t>(std::move(context), stream, format, args...);
}

//! Single argument
template<class Writer, typename Arg>
extra_stream<Writer, ostream_write_helper_t>
make_stream(message_context_ref context, std::ostream &stream, const Arg &arg)
{
    return extra_stream<Writer, ostream_write_helper_t>(std::move(context), stream, "{}", arg);
}

//! Without arguments
template<class Writer>
extra_stream<Writer, ostream_write_helper_t>
make_stream(message_context_ref context, std::ostream &stream)
{
    return extra_stream<Writer, ostream_write_helper_t>(std::move(context), stream, nullptr);
}

/// With writing to other supportable sink

template <typename T>
struct has_trace_method
{
    typedef char Yes;
    typedef char No[2];

    template<typename U, U> class checker;

    template<typename C>
    static Yes& test(checker<void (C::*)(int, int), &C::trace>*);

    template<typename>
    static No& test(...);

    static const bool value = sizeof(test<T>(nullptr)) == sizeof(Yes);
};

template <typename T>
struct has_trace_x_support
{
    typedef char Yes;
    typedef char No[2];

    template<typename U, U> class checker;

    template<typename C>
    static Yes& test(checker<void (C::*)(int, int), &C::trace>*);

    template<typename>
    static No& test(...);

    static const bool value = (sizeof(test<T>(nullptr)) == sizeof(Yes)) || std::is_base_of<std::ostream, T>::value;
};

//! Multiple arguments
template<class Writer, typename... Args, class Sink>
typename std::enable_if<has_trace_method<Sink>::value, extra_stream<Writer, trace_x_write_helper_t<Sink>>>::type
make_stream(message_context_ref context, Sink &sink, fmt::CStringRef format, const Args & ... args)
{
    return extra_stream<Writer, trace_x_write_helper_t<Sink>>(std::move(context), sink, format, args...);
}

////! Single argument
template<class Writer, typename Arg, class Sink>
typename std::enable_if<has_trace_method<Sink>::value, extra_stream<Writer, trace_x_write_helper_t<Sink>>>::type
make_stream(message_context_ref context, Sink &sink, const Arg &arg)
{
    return extra_stream<Writer, trace_x_write_helper_t<Sink>>(std::move(context), sink, "{}", arg);
}

//! Without arguments
template<class Writer, class Sink>
typename std::enable_if<has_trace_method<Sink>::value, extra_stream<Writer, trace_x_write_helper_t<Sink>>>::type
make_stream(message_context_ref context, Sink &sink)
{
    return extra_stream<Writer, trace_x_write_helper_t<Sink>>(std::move(context), sink, nullptr);
}

}

#endif

#endif /// #ifndef TRACE_X_TRACE_H
