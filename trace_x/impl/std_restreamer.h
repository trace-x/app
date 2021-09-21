//
// Copyright(c) 2014-2016 Nikita Kozhanov.
//

#include "../detail/trace.h"

#include <boost/scoped_ptr.hpp>
#include <sstream>

//! std::ostream output redirection class
class intercept_stream : public std::streambuf
{
public:
    intercept_stream(std::ostream &stream, const std::string &module, char message_type):
        _message_type(message_type),
        _module(module),
        _source_stream(stream),
        _restream(nullptr)
    {
        _source_stream.iword(trace_x::x_intercept_index) = 1;

        /// swap the the old buffer in ostream with this buffer.
        _source_buf = _source_stream.rdbuf(this);

        /// create a new ostream that we set the old buffer in
        boost::scoped_ptr<std::ostream> os(new std::ostream(_source_buf));

        _restream.swap(os);
    }

    ~intercept_stream()
    {
        /// restore old buffer

        _source_stream.rdbuf(_source_buf);
    }

protected:
    virtual std::streamsize xsputn(const char *msg, std::streamsize count)
    {
        /// output to new stream with old buffer (to e.g. screen [std::cout])
        _restream->write(msg, count);

        if(_source_stream.iword(trace_x::x_intercept_index))
        {
            /// write to internal buffer
            _writer << fmt::StringRef(msg, size_t(count));
        }

        return count;
    }

    int_type overflow(int_type c = traits_type::eof())
    {
        if(!traits_type::eq_int_type(c, traits_type::eof()))
        {
            char_type const t = traits_type::to_char_type(c);

            this->xsputn(&t, 1);

            _last_char = t;
        }

        return !traits_type::eof();
    }

    virtual int sync()
    {
        if(_last_char == '\n')
        {
            _restream->flush();

            if(_writer.size() && _source_stream.iword(trace_x::x_intercept_index))
            {
                /// send message to trace_x

                trace_x::trace_message(trace_x::message_context_t {_module.c_str(), _module.length() + 1, "", 1,
                                                                  0, "", 1,
                                                                  0, "", 1, _message_type, trace_x::trace_timestamp(), 0}, _writer.data(), _writer.size());
                _writer.clear();
            }

            _last_char = 0;
        }

        return std::streambuf::sync();
    }

private:
    char _message_type;
    char_type _last_char;

    std::string _module;
    std::streambuf *_source_buf;
    std::ostream &_source_stream;
    boost::scoped_ptr<std::ostream> _restream;
    fmt::MemoryWriter _writer;
};

class StdRestreamer
{
public:
    StdRestreamer():
        _cout_intercept(std::cout, "cout", trace_x::MESSAGE_INFO),
        _clog_intercept(std::clog, "clog", trace_x::MESSAGE_DEBUG),
        _cerr_intercept(std::cerr, "cerr", trace_x::MESSAGE_ERROR)
    {
    }

    intercept_stream _cout_intercept;
    intercept_stream _clog_intercept;
    intercept_stream _cerr_intercept;
};
