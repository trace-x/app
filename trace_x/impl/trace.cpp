#include "../detail/trace.h"

#include "../trace_x.h"

#include <iostream>
#include <cstdlib>
#include <cstdarg>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <chrono>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "platform.h"
#include "types.h"
#include "stack_trace.h"
#include "std_restreamer.h"

namespace trace_x
{

const int x_intercept_index = std::ios::xalloc();
int x_disable_auto_connect = 0;

static const size_t TxBufferSize = 65504;
static const uint8_t ProtocolVersion = 1;

//! Class for connection and transmitting
class TraceTransmitter
{
    struct config_t
    {
        std::string server_name = "trace_x";
        std::string crash_report_prefix;

        bool crash_report_use_gdb = true;
    };

public:
    TraceTransmitter(bool do_init = true):
        _default_flag(0),
        _buffer_offset(sizeof(uint64_t)),
        _rx_flag(&_default_flag),
        _srv_flag(0)
    {
        if(!x_disable_auto_connect && do_init)
        {
            read_config();

            init(_config.server_name.c_str());
        }
    }

    void read_config()
    {
        boost::filesystem::path process_path(get_process_name());
        boost::filesystem::path process_dir(process_path.parent_path());

        std::string json_config;

        std::vector<std::string> check_list;

        if(const char* trace_config_env = std::getenv(TraceXConfigEnvID))
        {
            if(boost::filesystem::exists(trace_config_env))
            {
                // read config from json file

                check_list.push_back(trace_config_env);
            }
            else
            {
                json_config = trace_config_env;
            }
        }

        if(json_config.empty())
        {
            check_list.push_back((process_dir / "trace_x.json").string());
            check_list.push_back((process_dir / "config/trace_x.json").string());

            for(const std::string &file : check_list)
            {
                if(boost::filesystem::exists(file))
                {
                    std::ifstream ifs(file);

                    json_config.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());

                    break;
                }
            }
        }

        if(!json_config.empty())
        {
            boost::property_tree::ptree config;

            try
            {
                std::istringstream cfg_stream(json_config);

                boost::property_tree::read_json(cfg_stream, config);

                _config.server_name = config.get("name", _config.server_name);
                _config.crash_report_prefix = config.get("crash_report_prefix", _config.crash_report_prefix);

#ifndef _WIN32
                _stack_trace._use_gdb = config.get("crash_report_use_gdb", _stack_trace._use_gdb);
                _stack_trace._use_gdb_full = config.get("crash_report_use_gdb_full", _stack_trace._use_gdb_full);
#endif
            }
            catch(std::exception &e)
            {
                ::fprintf(stderr, "trace_x: can`t parse config : %s", e.what());
            }
        }

        if(_config.crash_report_prefix.empty())
        {
            std::vector<std::string> prefixes;

            prefixes.push_back((process_dir / "log/crash_report").string());

#ifdef _WIN32
            // TODO application data dir
#else
            prefixes.push_back("/var/log/" + process_path.stem().string() + "/crash_report");
#endif

            prefixes.push_back((boost::filesystem::temp_directory_path() / process_path.stem() / "crash_report").string());

            for(const std::string &prefix : prefixes)
            {
                try
                {
                    boost::filesystem::create_directories(prefix);

                    _config.crash_report_prefix = prefix;

                    break;
                }
                catch(...)
                {
                }
            }
        }

#ifndef _WIN32
        _stack_trace._crash_report_prefix = _config.crash_report_prefix;
#endif

        if(const char* server_name = std::getenv(TraceServerNameEnvID))
        {
            _config.server_name = server_name;
        }
    }

    ~TraceTransmitter()
    {
        delete _filter_index.mutex;

        // TODO удаление на приёмнике!

        boost::interprocess::named_mutex::remove(_filter_mutex_name.c_str());
        boost::interprocess::shared_memory_object::remove(_filter_shm_name.c_str());
    }

    //! Returns the elapsed time (in nanoseconds) since the last connection time
    uint64_t timestamp() const
    {
        if(_connect_time.time_since_epoch() != std::chrono::high_resolution_clock::time_point::duration::zero())
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - _connect_time).count();
        }

        return 0;
    }

    uint64_t system_time() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void init(const char *server_name)
    {
        if(server_name)
        {
            try
            {
                _server_name = server_name;

                boost::interprocess::permissions access;
                access.set_unrestricted();

                _rx_shm = boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, server_name, 1024, 0, access);

                int8_t *shm_rx_flag = _rx_shm.find_or_construct<int8_t>(RxFlagID)(0);

                if(shm_rx_flag)
                {
                    _rx_flag = shm_rx_flag;
                }

                _filter_shm_name = fmt::format("{}_pid_{}", _server_name, trace_x::get_pid()).c_str();
                _filter_mutex_name = _filter_shm_name + "_mutex";
            }
            catch(std::exception &e)
            {
                std::fprintf(stderr, "can`t open_or_create managed_shared_memory for rx_flag: %s\n", e.what());

                _rx_flag = &_default_flag;
            }
        }
        else
        {
            _rx_flag = &_default_flag;
        }
    }

    bool is_open(const message_context_t &context, int *status)
    {
        bool result = false;

        *status = -1;

        if(*_rx_flag == 1)
        {
            MutexLocker locker(_cs);

            if(!_socket.is_open())
            {
                connect();
            }
            //TODO описАть логику с этим флагом - лажа какая-то
            else if(_srv_flag && (*_srv_flag == 0))
            {
                _socket.close();

                connect();
            }

            if(_socket.is_open())
            {
                //boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*_filter_index.mutex);

                _filter_index.mutex->lock();

                filter_index_t::index<message_filter_t::ByKey>::type::iterator message_index =
                        _filter_index.index->get<message_filter_t::ByKey>().find(
                            boost::make_tuple(uint8_t(context.message_type),
                                              uint64_t(context.module),
                                              uint64_t(get_tid()),
                                              uint64_t(context.object),
                                              uint64_t(context.function),
                                              uint64_t(context.source)
                                              ));

                if(message_index == _filter_index.index->get<message_filter_t::ByKey>().end())
                {
                    result = true;
                    *status = 0;
                }
                else
                {
                    result = message_index->is_accepted;
                    *status = 1;
                }

                _filter_index.mutex->unlock();
            }
        }

        return result;
    }

    void write_header(char packet_type, const message_context_t &context, int status)
    {
        // [packet type][subtype][timestamp][thread id][context][module][function][source][line][label]

        write_value(uint8_t(packet_type));
        write_value(uint8_t(context.message_type));

        if(context.timestamp > timestamp())
        {
            write_value(timestamp());
        }
        else
        {
            write_value(context.timestamp);
        }

        write_value(context.extra_timestamp);
        write_value(get_tid());
        write_value(uint64_t(context.object));
        write_value(uint64_t(context.module));
        write_value(uint64_t(context.function));
        write_value(uint64_t(context.source));
        write_value(uint32_t(context.line));
        write_value(uint64_t(context.label));

        if(status == 1)
        {
            write_string(context.module, 1);
            write_string(context.source, 1);
            write_string(context.function, 1);
        }
        else
        {
            write_string(context.module, context.module_len);
            write_string(context.source, context.source_len);
            write_string(context.function, context.function_len);
        }

        write_string(context.label, context.label_len);
    }

    void write_data(const char *data, size_t size)
    {
        memcpy(_buffer + _buffer_offset, data, size);

        _buffer_offset += size;
    }

    // TODO remove
    void write_string(const std::string &string, bool write_size = true)
    {
        //[length][string]

        if(write_size)
        {
            write_value(uint32_t(string.size()));
        }

        memcpy(_buffer + _buffer_offset, string.c_str(), string.size());

        _buffer_offset += string.size();
    }

    void write_xstring(const char *string, size_t size, bool write_size = true)
    {
        //[length][string]

        if(write_size)
        {
            write_value(uint32_t(size));
        }

        memcpy(_buffer + _buffer_offset, string, size);

        _buffer_offset += size;
    }

    void send_command(uint8_t command_code)
    {
        if(!_socket.is_open())
        {
            connect(false);
        }
        //TODO описАть логику с этим флагом. Как-то странно всё
        else if(_srv_flag && (*_srv_flag == 0))
        {
            _socket.close();

            connect(false);
        }

        if(_socket.is_open())
        {
            write_value(uint8_t(COMMAND));
            write_value(command_code);

            send_buffer();
        }
    }

    void write_string(const char *string, size_t size, bool write_size = true)
    {
        //send without /0

        if(write_size)
        {
            write_value(uint32_t(size - 1));
        }

        memcpy(_buffer + _buffer_offset, string, size - 1);

        _buffer_offset += size - 1;
    }

    void write_string(const char *format, va_list parameters)
    {
        size_t length = vsnprintf(_buffer + _buffer_offset + sizeof(uint32_t), TxBufferSize - _buffer_offset - sizeof(uint32_t), format, parameters);

        write_value(uint32_t(length));

        _buffer_offset += (length < TxBufferSize - _buffer_offset) ? length : TxBufferSize - length - 1;
    }

    template<class T>
    void write_value(const T &value)
    {
        *(T*)(_buffer + _buffer_offset) = value;

        _buffer_offset += sizeof(T);
    }

    template<class T>
    void write_vector(const std::vector<T> &vector)
    {
        write_value(uint64_t(vector.size()));

        if(!vector.empty())
        {
            write_data((const char *)(&vector[0]), vector.size() * sizeof(T));
        }
        else
        {
            write_data(0, 0);
        }
    }

    bool send_buffer()
    {
        return send_header(_buffer_offset);
    }

    bool send_header(uint64_t message_size)
    {
        *(uint64_t*)(_buffer) = message_size;

        bool result = _socket.send(_buffer, _buffer_offset);

        _buffer_offset = sizeof(uint64_t);

        if(!result)
        {
            _socket.close();
        }

        return result;
    }

    bool send_data(const void* data, size_t data_size)
    {
        bool result = _socket.send(data, data_size);

        if(!result)
        {
            _socket.close();
        }

        return result;
    }

    void lock() { _cs.lock(); }
    void unlock() { _cs.unlock(); }

    bool is_connected() const
    {
        return _socket.is_open();
    }

    bool connect(bool register_process = true)
    {
        // MutexLocker locker(_cs);

        if(!_socket.is_open())
        {
            if(_socket.connect(_server_name))
            {
                if(register_process)
                {
                    _connect_time = std::chrono::high_resolution_clock::now();

                    uint64_t pid = trace_x::get_pid();

                    write_value(uint8_t(trace_x::CONNECT));
                    write_value(ProtocolVersion);

                    // <pid [uint64_t]> <system time [uint64_t]> <user name[string]> <process name [string]>

                    write_value(pid);
                    write_value(this->system_time());
                    write_string(get_user_name(), true);
                    write_string(get_process_name(), false);

                    if(send_buffer() && _socket.wait_ack())
                    {
                        try
                        {
                            _filter_shm = boost::interprocess::managed_shared_memory(boost::interprocess::open_read_only, _filter_shm_name.c_str());

                            _srv_flag = _filter_shm.find<int8_t>(TxFlagID).first;
                            _filter_index.index = _filter_shm.find<filter_index_t>(TxFilterIndexID).first;

                            _filter_index.mutex = new boost::interprocess::named_mutex(boost::interprocess::open_only, _filter_mutex_name.c_str());

                            //_filter_index.mutex = _filter_shm.find<boost::interprocess::interprocess_mutex>(TxFilterMutexID).first;
                        }
                        catch(std::exception &e)
                        {
                            disconnect();

                            std::fprintf(stderr, "can`t open managed_shared_memory for filter: %s\n", e.what());
                        }
                    }
                    else
                    {
                        disconnect();
                    }
                }
            }
            else
            {
                disconnect();
            }
        }

        return _socket.is_open();
    }

    void disconnect()
    {
        _connect_time = std::chrono::high_resolution_clock::time_point();

        _socket.close();

        *_rx_flag = 0;
    }

    size_t buffer_capacity()
    {
        return TxBufferSize - _buffer_offset;
    }

    size_t buffer_size()
    {
        return _buffer_offset;
    }

private:
    config_t _config;

    stack_trace::StackTrace _stack_trace;

    CriticalSection _cs;
    LocalSocket _socket;

    char _buffer[TxBufferSize];

    int8_t _default_flag;

    size_t _buffer_offset;

    std::string _server_name;
    std::string _filter_mutex_name;
    std::string _filter_shm_name;

    boost::interprocess::managed_shared_memory _rx_shm;
    boost::interprocess::managed_shared_memory _filter_shm;

    int8_t *_rx_flag; //! Receiver gate flag
    int8_t *_srv_flag;
    filter_index _filter_index;

    std::chrono::high_resolution_clock::time_point _connect_time;
};

static TraceTransmitter _trace;
static StdRestreamer _std_restreamer;
static x_logger _logger;

TraceScope::TraceScope(const message_context_t &context): _context(context)
{
    int status;

    if(_trace.is_open(_context.typed(MESSAGE_CALL), &status))
    {
        trace_message(_context, nullptr, 0, status);
    }
    else
    {
        _context.module = 0;
    }

    _context.extra_timestamp = _trace.timestamp();
}

TraceScope::TraceScope(message_context_t &&context): _context(std::move(context))
{
    int status;

    if(_trace.is_open(_context.typed(MESSAGE_CALL), &status))
    {
        trace_message(_context, nullptr, 0, status);
    }
    else
    {
        _context.module = 0;
    }

    _context.extra_timestamp = _trace.timestamp();
}

TraceScope::~TraceScope()
{
    _context.timestamp = _trace.timestamp();

    int status;

    if(_context.module && _trace.is_open(_context, &status))
    {
        trace_message(_context.typed(MESSAGE_RETURN), nullptr, 0, status);
    }
}

//////////////////////////////////////////////////////////////////////

//std::string format_string(const char *const format, ...)
//{
//    auto temp = std::vector<char> {};
//    auto length = std::size_t {63};
//    std::va_list args;
//    while (temp.size() <= length)
//    {
//        temp.resize(length + 1);
//        va_start(args, format);
//        const auto status = std::vsnprintf(temp.data(), temp.size(), format, args);
//        va_end(args);
//        if (status < 0)
//            throw std::runtime_error {"string formatting error"};
//        length = static_cast<std::size_t>(status);
//    }

//    return std::string {temp.data(), length};
//}

void trace_message(const char *module, size_t module_len, const char *source, size_t source_len, int line,
                   const char *function, size_t function_len, const void *object, const char *label, size_t label_len,
                   char message_type, const char *format, ...)
{
    //    if(_logger)
    //    {
    //        va_list parameters;
    //        va_start(parameters, format);

    //        _logger->log(format_string(format, parameters), message_type);

    //        va_end(parameters);
    //    }

    message_context_t context { module, module_len, source, source_len, line, function, function_len, object, label, label_len, message_type, _trace.timestamp(), 0};

    int status;

    if(_trace.is_open(context, &status))
    {
        _trace.lock();

        _trace.write_header(trace_x::TRACE_MESSAGE, context, status);

        va_list parameters;
        va_start(parameters, format);

        _trace.write_string(format, parameters);

        va_end(parameters);

        _trace.send_buffer();

        _trace.unlock();
    }
}

void trace_message(const message_context_t &context, const char *message, size_t message_size)
{
    int status = -1;

    if(trace_check(context, &status))
    {
        trace_message(context, message, message_size, status);
    }
}

void trace_message(const message_context_t &context, const char *message, size_t message_size, int status)
{
    if(status != -1)
    {
        _trace.lock();

        _trace.write_header(trace_x::TRACE_MESSAGE, context, status);

        if(message_size > _trace.buffer_capacity() - sizeof(uint32_t))
        {
            _trace.write_value(uint32_t(message_size));

            _trace.send_header(_trace.buffer_size() + message_size);

            _trace.send_data(message, message_size);
        }
        else
        {
            _trace.write_xstring(message, message_size);

            _trace.send_buffer();
        }

        _trace.unlock();
    }
}

void trace_message_s(const char *module, size_t module_len, const char *source, size_t source_len, int line,
                     const char *function, size_t function_len, const void *object,
                     const char *label, size_t label_len,
                     char message_type, const std::string &message, int status)
{
    uint64_t timestamp = _trace.timestamp();

    if(_logger)
    {
        _logger->log(message.data(), message.size(), message_type);
    }

    if(status != -1)
    {
        _trace.lock();

        _trace.write_header(trace_x::TRACE_MESSAGE,
                            message_context_t { module, module_len, source, source_len, line, function, function_len, object, label, label_len, message_type, timestamp, 0},
                            status);

        if(message.size() > _trace.buffer_capacity() - sizeof(uint32_t))
        {
            _trace.write_value(uint32_t(message.size()));

            _trace.send_header(_trace.buffer_size() + message.size());

            _trace.send_data(message.data(), message.size());
        }
        else
        {
            _trace.write_string(message);

            _trace.send_buffer();
        }

        _trace.unlock();
    }
}

void trace_data(const message_context_t &context, const std::string &message, const data_array_t &data, int status)
{
    // [header][metadata: sizes, elem size, channels, unit_type, data format, palette_size, byte_size][palette][data]

    _trace.lock();

    _trace.write_header(trace_x::TRACE_MESSAGE, context, status);

    uint64_t data_size = data.data_byte_size();

    _trace.write_vector(data.size);
    _trace.write_value(data.elem_bit_count);
    _trace.write_value(data.channels);
    _trace.write_value<int8_t>(data.unit_type);
    _trace.write_value(data.data_format);
    _trace.write_vector(data.band_map);
    _trace.write_vector(data.axes);
    _trace.write_value(uint32_t(message.size()));
    _trace.write_value(data.palette_size);
    _trace.write_value(data_size);

    _trace.send_header(_trace.buffer_size() + message.size() + data_size + data.palette_size);

    _trace.send_data(message.data(), message.size());

    if(data.palette_size)
    {
        _trace.send_data(data.palette, data.palette_size);
    }

    if(data.is_continuous())
    {
        _trace.send_data(data.data, size_t(data_size));
    }
    else
    {
        //TODO for each data slice

        for(size_t i = 0; i < data.size[0]; ++i)
        {
            _trace.send_data((const char*)(data.data) + data.step[0] * i, size_t(align_bits(data.size[1] * data.elem_bit_count) / 8));
        }
    }

    _trace.unlock();
}

void send_command(uint8_t command_code)
{
    _trace.send_command(command_code);
}

int trace_check(const char *module, const char *source, const char *function, const void *object, char message_type, int *status)
{
    // _logger check levels

    message_context_t context { module, 1, source, 1, 0, function, 1, object, 0, 0, message_type, 0, 0};

    return _trace.is_open(context, status) || _logger;
}

int trace_check(const message_context_t &context, int *status)
{
    // _logger check levels

    return _trace.is_open(context, status) || _logger;
}

void trace_connect(const char *server_name)
{
    _trace.init(server_name);
}

void trace_stop(const char *server_name)
{
    TraceTransmitter tx(false);

    tx.init(server_name);

    tx.send_command(COMMAND_STOP);
}

int is_server_runned(const char *server_name)
{
    TraceTransmitter tx(false);

    tx.init(server_name);

    tx.connect(false);

    return tx.is_connected();
}

void start_trace_receiver(int log_mode, const char *, const char *server_name_)
{
    // generate unique server_name

    std::string server_name;

    if(server_name_)
    {
        server_name = server_name_;
    }

    if(server_name.empty())
    {
        boost::uuids::basic_random_generator<boost::mt19937> gen_uid;

        server_name = boost::uuids::to_string(gen_uid());
    }

    // start detached with server_name, non-gui? (gui, but hidden), with auto-exit, with log mode, with log path, with filter?, with config name?

    std::string xargs;

    if(log_mode == MODE_SAVE_ALL)
    {
        xargs += "--auto_save ALL";
    }
    else if(log_mode == MODE_SAVE_CRASH)
    {
        xargs += "--auto_save CRASH";
    }

    int check_result = trace_x::execute("trace_x --check");

    if(check_result == 42)
    {
        if(trace_x::start_detached(fmt::format("trace_x --auto_exit --no_show -n {} {}", server_name, xargs)))
        {
            // connect to this server and wait for connection

            _trace.init(server_name.c_str());

            do
            {
                trace_x::sleep(100);
            }
            while(!_trace.connect(true));
        }
    }
}

void show_trace_receiver()
{
    _trace.send_command(COMMAND_SHOW);
}

void set_trace_logger(const x_logger &logger)
{
    _logger = logger;
}

uint64_t trace_timestamp()
{
    return _trace.timestamp();
}

}
