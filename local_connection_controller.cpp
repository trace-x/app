#include "local_connection_controller.h"

#include <utility>
#include <iostream>

#include <QThread>

#include "trace_controller.h"
#include "data_parser.h"

#include "trace_x/trace_x.h"

namespace
{

static const size_t ShmemSize = 6553600;

}

LocalConnectionController::LocalConnectionController(TraceController *trace_controller, const QString &name, QObject *parent):
    QObject(parent),
    _trace_controller(trace_controller),
    _process_model(0),
    _srv_flag(0),
    _disconnect_time(0),
    _srv_name(name),
    _drop_buffer(false)
{
    X_CALL;

    connect(&_thread, &QThread::started, this, &LocalConnectionController::controller_thread, Qt::DirectConnection);
}

void LocalConnectionController::stop_async()
{
    X_CALL;

    // Этот метод будет вызван при разрыве соединения

    if(!_disconnect_time)
    {
        _disconnect_time = boost::chrono::duration_cast<boost::chrono::nanoseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();
    }

    if(_srv_flag)
    {
        *_srv_flag = 0;
    }

    _thread.requestInterruption();

    // Не ждём завершения потока, т.к. программа подвиснет, если в буфере много сообщений.
    // Когда поток завершится, он сам уничтожит этот объект.
}

void LocalConnectionController::stop_sync()
{
    X_CALL;

    _drop_buffer = true;

    _thread.requestInterruption();

    _thread.quit();
    _thread.wait();

    stop_async();

    if(_process_model)
    {
        _process_model->disconnected(_disconnect_time);

        _process_model->_index_container.index = 0;
    }

    _process_model = 0;
}

void LocalConnectionController::clear()
{
    X_CALL;

    _process_model = 0;
}

LocalConnectionController::~LocalConnectionController()
{
    X_CALL;

    if(_process_model)
    {
        _process_model->_index_container.index = 0;

        delete _filter_index.mutex;

        _filter_index.mutex = 0;
    }

    stop_async();

    _thread.quit();
    _thread.wait();

    if(_process_model)
    {
        _process_model->disconnected(_disconnect_time);
    }

    qDeleteAll(_list);

    emit connection_closed();
}

void LocalConnectionController::process_packet(const QByteArray &frame, bool &ack)
{
    X_CALL;

    size_t offset = 0;

    uint8_t packet_type = get_value<uint8_t>(frame, offset);

    if(packet_type == trace_x::TRACE_MESSAGE)
    {
        raw_message_t *message = new raw_message_t;

        message->subtype   = get_value<uint8_t>(frame, offset);
        message->timestamp = get_value<uint64_t>(frame, offset);
        message->extra_timestamp = get_value<uint64_t>(frame, offset);
        message->tid       = get_value<uint64_t>(frame, offset);
        message->context   = get_value<uint64_t>(frame, offset);
        message->module    = get_value<uint64_t>(frame, offset);
        message->function  = get_value<uint64_t>(frame, offset);
        message->source    = get_value<uint64_t>(frame, offset);
        message->line      = get_value<uint32_t>(frame, offset);
        message->label     = get_value<uint64_t>(frame, offset);

        X_INFO("new message: subtype = {} ; size = {}; data offset = {}", message->subtype, frame.size(), offset);

        message->data = new char[frame.size() - offset];
        memcpy(message->data, frame.data() + offset, frame.size() - offset);

        _mutex.lock();

        _list.append(message);

        _mutex.unlock();
    }
    else if(packet_type == trace_x::CONNECT)
    {
        /*uint8_t version =*/get_value<uint8_t>(frame, offset);

        uint64_t pid = get_value<uint64_t>(frame, offset);
        uint64_t timestamp = get_value<uint64_t>(frame, offset);

        QString user_name = get_string(frame, offset);
        QString process_name = get_string(frame, offset, true);

        register_process(pid, timestamp, process_name, user_name);

        //sync for shared memory creation
        ack = true;
    }
    else
    {
        uint8_t command_type = get_value<uint8_t>(frame, offset);

        if(command_type == trace_x::COMMAND_STOP)
        {
            X_IMPORTANT("STOP command received");

            emit _trace_controller->stop_server();
        }
        else if(command_type == trace_x::COMMAND_SHOW)
        {
            X_IMPORTANT("SHOW command received");

            emit _trace_controller->show_gui();
        }
    }
}

void LocalConnectionController::register_process(uint64_t pid, uint64_t timestamp, const QString &process_name, const QString &user_name)
{
    X_CALL;

    emit process_registered();

    _shm_name = QString("%1_pid_%2").arg(_srv_name).arg(pid).toStdString();
    _mutex_name = _shm_name + "_mutex";

    boost::interprocess::shared_memory_object::remove(_shm_name.c_str());
    boost::interprocess::named_mutex::remove(_mutex_name.c_str());

    boost::interprocess::permissions access;
    access.set_unrestricted();

    _filter_shm = boost::interprocess::managed_shared_memory(boost::interprocess::create_only, _shm_name.c_str(), ShmemSize, 0, access);

    _filter_index.index = _filter_shm.construct<trace_x::filter_index_t>(TxFilterIndexID)
            (trace_x::filter_index_t::ctor_args_list(), trace_x::filter_index_t::allocator_type(_filter_shm.get_segment_manager()));

    try
    {
        _filter_index.mutex = new boost::interprocess::named_mutex(boost::interprocess::create_only, _mutex_name.c_str(), access);

#ifdef Q_OS_UNIX
        // TODO remove this
        int mutex_fd = ::open(("/dev/shm/sem." + _mutex_name).c_str(), 0);

        ::fchmod(mutex_fd, access.get_permissions());
#endif

        _srv_flag = _filter_shm.construct<int8_t>(TxFlagID)(1);

        _process_model = _trace_controller->register_process(pid, timestamp, process_name, user_name, _filter_index);

        _thread.start();
    }
    catch(const std::exception &e)
    {
        std::cerr << "can`t create mutex " << _mutex_name << " : " << e.what() << std::endl;
    }
}

void LocalConnectionController::controller_thread()
{
    X_CALL;

    raw_message_t *message = 0;

    while(!(QThread::currentThread()->isInterruptionRequested() && (_list.empty() || _drop_buffer)))
    {
        _mutex.lock();

        QLinkedList<raw_message_t*>::iterator begin = _list.begin();
        QLinkedList<raw_message_t*>::iterator end = _list.end() - 1;

        _mutex.unlock();

        for(QLinkedList<raw_message_t*>::iterator it = begin; it != end + 1; ++it)
        {
            message = *it;

            _process_model->append_message(message);

            delete [] message->data;
            delete message;
        }

        _mutex.lock();

        _list.erase(begin, end + 1);

        _mutex.unlock();

        QThread::currentThread()->msleep(100);
    }

    // Self desctruction only after processing of all messages(or dropping)
    this->deleteLater();
}
