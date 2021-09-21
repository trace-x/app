#include "../trace_x.h"
#include "../detail/types.h"

#include <boost/filesystem.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/null_sink.h"

#include <memory>
#include <array>

namespace trace_x
{

class logger::logger_impl
{
public:
    std::shared_ptr<spdlog::logger> spd_logger;

    // move to static
    std::array<int, _MESSAGE_END_> spd_levels;
};

logger::logger():
    _p(new logger_impl())
{
    _p->spd_levels[trace_x::MESSAGE_CALL]         = spdlog::level::trace;
    _p->spd_levels[trace_x::MESSAGE_RETURN]       = spdlog::level::trace;
    _p->spd_levels[trace_x::MESSAGE_INFO]         = spdlog::level::info;
    _p->spd_levels[trace_x::MESSAGE_DEBUG]        = spdlog::level::debug;
    _p->spd_levels[trace_x::MESSAGE_IMPORTANT]    = spdlog::level::alert;
    _p->spd_levels[trace_x::MESSAGE_EVENT]        = spdlog::level::alert;
    _p->spd_levels[trace_x::MESSAGE_WARNING]      = spdlog::level::warn;
    _p->spd_levels[trace_x::MESSAGE_ERROR]        = spdlog::level::err;
    _p->spd_levels[trace_x::MESSAGE_EXCEPTION]    = spdlog::level::critical;
    _p->spd_levels[trace_x::MESSAGE_ASSERT]       = spdlog::level::critical;
    _p->spd_levels[trace_x::MESSAGE_PARAMETERS]   = spdlog::level::critical;
    _p->spd_levels[trace_x::MESSAGE_VALUE]        = spdlog::level::critical;
    _p->spd_levels[trace_x::MESSAGE_SUSPEND]      = spdlog::level::notice;
    _p->spd_levels[trace_x::MESSAGE_RESUME]       = spdlog::level::notice;
    _p->spd_levels[trace_x::MESSAGE_SIGNAL]       = spdlog::level::notice;
    _p->spd_levels[trace_x::MESSAGE_DATA]         = spdlog::level::info;
    _p->spd_levels[trace_x::MESSAGE_IMAGE]        = spdlog::level::info;
    _p->spd_levels[trace_x::MESSAGE_CONNECTED]    = spdlog::level::info;
    _p->spd_levels[trace_x::MESSAGE_DISCONNECTED] = spdlog::level::info;
    _p->spd_levels[trace_x::MESSAGE_CRASH]        = spdlog::level::critical;
}

logger::~logger()
{
    delete _p;

    _p = nullptr;
}

void logger::log(const char *data, size_t size, char level)
{
    if(_p && _p->spd_logger)
    {
        _p->spd_logger->_log_if_enabled(spdlog::level::level_enum(_p->spd_levels[level]), fmt::StringRef(data, size));
    }
}

void logger::set_pattern(const std::string &pattern)
{
    _p->spd_logger->set_pattern(pattern);
}

std::shared_ptr<logger> logger::complex(const std::vector<std::shared_ptr<logger> > &loggers, const std::string &logger_name)
{
    std::shared_ptr<logger> new_logger = std::make_shared<logger>();

    std::vector<spdlog::sink_ptr> sinks;

    for(const std::shared_ptr<logger> &logger : loggers)
    {
        sinks.insert(sinks.end(), logger->_p->spd_logger->_sinks.begin(), logger->_p->spd_logger->_sinks.end());
    }

    new_logger->_p->spd_logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());

    return new_logger;
}

std::shared_ptr<logger> logger::null()
{
    std::shared_ptr<logger> new_logger = std::make_shared<logger>();

    new_logger->_p->spd_logger = std::make_shared<spdlog::logger>("", std::make_shared<spdlog::sinks::null_sink_mt>());

    return new_logger;
}

std::shared_ptr<logger> logger::file(const std::string &filename, bool append, const std::string &logger_name)
{
    try
    {
        boost::filesystem::path parent_path = boost::filesystem::path(filename).parent_path();

        if(!parent_path.empty())
        {
            boost::filesystem::create_directories(boost::filesystem::path(filename).parent_path());
        }
    }
    catch(const std::exception &e)
    {
        X_EXCEPTION_F(e.what());
    }
    catch(...)
    {
        X_EXCEPTION_F("unknown exception");
    }

    std::shared_ptr<logger> new_logger = std::make_shared<logger>();

    new_logger->_p->spd_logger = std::make_shared<spdlog::logger>(logger_name, std::make_shared<spdlog::sinks::simple_file_sink_mt>(filename, append));

    return new_logger;
}

std::shared_ptr<logger> logger::simple_file(const std::string &filename, bool append, const std::string &logger_name)
{
    std::shared_ptr<logger> new_logger = file(filename, append, logger_name);

    new_logger->_p->spd_logger->set_pattern(std::string());

    return new_logger;
}

std::shared_ptr<logger> logger::cout(bool colored, const std::string &logger_name)
{
    std::shared_ptr<logger> new_logger = std::make_shared<logger>();

    spdlog::sink_ptr sink = spdlog::sinks::stdout_sink_mt::instance();

    if(colored)
    {
        new_logger->_p->spd_logger = std::make_shared<spdlog::logger>(logger_name, std::make_shared<spdlog::sinks::ansicolor_sink>(sink));
    }
    else
    {
        new_logger->_p->spd_logger = std::make_shared<spdlog::logger>(logger_name, sink);
    }

    return new_logger;
}

std::shared_ptr<logger> logger::cerr(bool colored, const std::string &logger_name)
{
    std::shared_ptr<logger> new_logger = std::make_shared<logger>();

    spdlog::sink_ptr sink = spdlog::sinks::stderr_sink_mt::instance();

    if(colored)
    {
        new_logger->_p->spd_logger = std::make_shared<spdlog::logger>(logger_name, std::make_shared<spdlog::sinks::ansicolor_sink>(sink));
    }
    else
    {
        new_logger->_p->spd_logger = std::make_shared<spdlog::logger>(logger_name, sink);
    }

    return new_logger;
}

std::shared_ptr<logger> cout()
{
    static x_logger log = logger::cout(true);

    return log;
}

logger * cerr()
{
    static x_logger log = logger::cerr(true);

    return log.get();
}

void log_cerr(const std::string &message, char level)
{
    spdlog::sinks::stderr_sink_st *errsink = new spdlog::sinks::stderr_sink_st();

  //  static std::weak_ptr<spdlog::sinks::stderr_sink_st> errsink = spdlog::sinks::stderr_sink_st::instance();

  //  static std::weak_ptr<spdlog::sinks::stderr_sink_st> sink;

  //  auto spt = sink.lock();

    spdlog::sink_ptr shsink(errsink);

    spdlog::logger loger("cerr", shsink);

    loger.error() << message;

   // errsink->log(
//    static std::weak_ptr<spdlog::logger> cerlog = spdlog::stderr_logger_st("cerr", true);
//    //auto cerlog = spdlog::stderr_logger_st("cerr", true);

//    auto cerlogs = cerlog.lock();

    //spdlog::stderr_logger_st("cerr", true)->error() << message;
}

}
