#include "trace_server.h"

#include <QDir>
#include <QCoreApplication>

#include <iostream>

#include "settings.h"
#include "trace_service.h"
#include "local_connection.h"
#include "trace_controller.h"

#include "trace_x/impl/types.h"
#include "trace_x/trace_x.h"

TraceServer::TraceServer(TraceController &trace_controller, const QString &server_name, QObject *parent) :
    QObject(parent),
    _trace_controller(trace_controller),
    _rx_flag(0)
{
    X_CALL;

    connect(&trace_controller, &TraceController::stop_server, this, &TraceServer::stop_server);

    _server_name = server_name;

    X_VALUE(_server_name);

    try
    {
        boost::interprocess::permissions access;
        access.set_unrestricted();

        //this is an atomic operations, don`t worry

        _rx_shm = boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, _server_name.toLocal8Bit(), 1024, 0, access);

        _rx_flag = _rx_shm.find_or_construct<int8_t>(RxFlagID)();

        if(_rx_flag)
        {
            connect(&_local_server, &LocalServer::new_connection, this, &TraceServer::add_connection);

            X_VALUE("_rx_flag", "{}" , int(*_rx_flag));

            bool other_server_runned = trace_x::is_server_runned(_server_name.toLocal8Bit());

            X_INFO("server is runned: {}", other_server_runned);

            if(!other_server_runned)
            {
                *_rx_flag = 0;

#ifdef Q_OS_UNIX
                unlink(QDir("/tmp").absoluteFilePath(_server_name).toLocal8Bit());
#endif
                start_server();

                //

                // Оповещаем открытые процессы о необходимости переподключения

                //TODO всё-таки стоит сделать с одним файлом разделяемой памяти, т.к за ним легче следить

                //                for(auto it = _filter_shm.named_begin(); it != _filter_shm.named_end(); ++it)
                //                {
                //                    std::string nn = it->name();
                //                }

                std::string shared_dir;

#ifdef Q_OS_UNIX
                shared_dir = "/dev/shm/";
#else
                boost::interprocess::ipcdetail::get_shared_dir(shared_dir);
#endif

                X_VALUE(shared_dir);

                QDir dir(QString::fromStdString(shared_dir));

                foreach (QString trace_x_shm, dir.entryList(QStringList() << QString("%1_pid_*").arg(_server_name), QDir::Files))
                {
                    if(!trace_x_shm.endsWith("_mutex"))
                    {
                        boost::interprocess::managed_shared_memory filter_shm = boost::interprocess::managed_shared_memory(boost::interprocess::open_only, trace_x_shm.toLocal8Bit());

                        int8_t *srv_flag = filter_shm.find<int8_t>(TxFlagID).first;

                        if(srv_flag)
                        {
                            *srv_flag = 0;
                        }
                    }
                }
            }
        }
        else
        {
            std::cerr << "can`t find_or_construct " << RxFlagID << std::endl;
        }
    }
    catch(std::exception &e)
    {
        X_ERROR(e.what());

        std::cerr << e.what() << std::endl;
    }
}

TraceServer::~TraceServer()
{
    X_CALL;

    if(_rx_flag)
    {
        _rx_flag = 0;
    }
}

bool TraceServer::is_started() const
{
    return (_rx_flag != 0);
}

QString TraceServer::server_name() const
{
    return _server_name;
}

void TraceServer::enable(bool enable)
{
    X_CALL;

    if(_rx_flag)
    {
        if(enable && !_local_server.is_listening())
        {
            X_IMPORTANT("stopping other server [{}]", _server_name);

            trace_x::trace_stop(_server_name.toLocal8Bit());

            start_server(true);
        }

        *_rx_flag = enable ? 1 : 0;

        X_VALUE("_rx_flag", int(*_rx_flag));

        emit state_changed(enable);
    }
}

void TraceServer::add_connection(quintptr descriptor)
{
    X_CALL;

    X_VALUE(descriptor);

    LocalConnectionController *controller = new LocalConnectionController(&_trace_controller, _server_name);

    connect(controller, &LocalConnectionController::process_registered, this, &TraceServer::new_process_registered, Qt::BlockingQueuedConnection);

    connect(&_trace_controller, &TraceController::close_connections, controller, &LocalConnectionController::stop_sync);

    LocalConnection *connection = new LocalConnection(descriptor, *controller, this);

    controller->_connection = connection;

    connect(&_trace_controller, &TraceController::close_connections, connection, &QObject::deleteLater);
    connect(&_trace_controller, &TraceController::close_connections, controller, &LocalConnectionController::stop_async);

    connect(connection, &LocalConnection::finished, connection, &QObject::deleteLater);
    connect(connection, &LocalConnection::finished, controller, &LocalConnectionController::stop_async);

    connect(controller, &LocalConnectionController::connection_closed, this, &TraceServer::connection_closed);

    connection->start(QThread::HighestPriority);
}

void TraceServer::connection_closed()
{
    X_CALL;

    LocalConnectionController *controller = static_cast<LocalConnectionController*>(sender());

    X_IMPORTANT("connection {} closed", (void*)controller);

    QString auto_save = x_settings().auto_save_option->string_value();

    if(!auto_save.isEmpty())
    {
        bool save = (auto_save == "ALL") || ((auto_save == "CRASH") && controller->_process_model->_crash_received);

        if(save)
        {
            QString trace_file_name = QDir(x_settings().log_dir_option->string_value()).absoluteFilePath(QString("%1.trace_x").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));

            _trace_controller.save_trace(trace_file_name);
        }
    }

    if(x_settings().auto_exit_option->is_command_line_set())
    {
        X_IMPORTANT("auto_exit_option is set! I`m quit!");

        //TODO can i remove this always?
        boost::interprocess::shared_memory_object::remove(_server_name.toLatin1());

        qApp->quit();
    }
}

void TraceServer::new_process_registered()
{
    X_CALL;

    if(x_settings().auto_clean_option->bool_value())
    {
        LocalConnectionController *controller = static_cast<LocalConnectionController*>(sender());

        QObject::disconnect(&_trace_controller, &TraceController::close_connections, controller, 0);
        QObject::disconnect(&_trace_controller, &TraceController::close_connections, controller->_connection, 0);

        // inside close_connections() emited
        _trace_controller.clear();

        // restore TraceController::close_connections signal connections

        connect(&_trace_controller, &TraceController::close_connections, controller, &LocalConnectionController::stop_sync);
        connect(&_trace_controller, &TraceController::close_connections, controller->_connection, &QObject::deleteLater);
        connect(&_trace_controller, &TraceController::close_connections, controller, &LocalConnectionController::stop_async);
    }
}

void TraceServer::start_server(bool force)
{
    X_CALL;

    if(force)
    {
        int fail_counter = 0;

        //TODO if server is not starting ?

        // Мы не можем узнать, получится ли у нас стать сервером сразу, поэтому пробуем делать это в цикле
        // При этом мы уверены, что у нас получится сделать это, т.к. другой приёмник либо уже остановлен, либо ещё в процессе остановки
        while (!_local_server.start(_server_name))
        {
            fail_counter++;

            if(fail_counter == 10)
            {
                //may be other receiver is hung?

                X_ERROR("can`t start server {}", _server_name);

                break;
            }

            QThread::msleep(100);
        }
    }
    else
    {
        if(!_local_server.start(_server_name))
        {
            emit state_changed(false);
        }
    }
}

void TraceServer::stop_server()
{
    X_CALL;

    _local_server.stop();

    emit _trace_controller.close_connections();

    emit state_changed(false);
}
