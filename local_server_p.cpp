#include "local_server_p.h"

#include "local_connection_p.h"

#include "trace_x/trace_x.h"

#include <boost/interprocess/permissions.hpp>

#ifdef Q_OS_WIN

#include <windows.h>

#define TRACEPIPE_BUFFER_SIZE           4096
#define TRACEPIPE_RETURNCH_BUFFER_SIZE  1
#define TRACEPIPE_TIMEOUT               10

LocalServerPrivate::LocalServerPrivate(QObject *parent) :
    QObject(parent)
{
    X_CALL;

    connect(&_listener_thread, &QThread::started, this, &LocalServerPrivate::listener, Qt::DirectConnection);
}

LocalServerPrivate::~LocalServerPrivate()
{
    X_CALL;

    close();
}

bool LocalServerPrivate::listen(const QString &name)
{
    X_CALL;

    _name = name;

    _listener_thread.start();

    return _listener_thread.isRunning();
}

void LocalServerPrivate::close()
{
    X_CALL;

    _listener_thread.requestInterruption();
    _listener_thread.quit();
    _listener_thread.wait();
}

bool LocalServerPrivate::is_listening() const
{
    return _listener_thread.isRunning();
}

void LocalServerPrivate::listener()
{
    X_CALL;

    qRegisterMetaType<quintptr>("quintptr");

    while (!QThread::currentThread()->isInterruptionRequested())
    {
        tTraceServerState state = TS_STATE_READY;

        LPSECURITY_ATTRIBUTES unrestricted_access = LPSECURITY_ATTRIBUTES(&boost::interprocess::ipcdetail::unrestricted_permissions_holder<0>::unrestricted);

        HANDLE pipe = CreateNamedPipeA(QString("\\\\.\\pipe\\%1").arg(_name).toLocal8Bit().data(),
                                       PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                       PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                       PIPE_UNLIMITED_INSTANCES, TRACEPIPE_RETURNCH_BUFFER_SIZE,
                                       TRACEPIPE_BUFFER_SIZE, TRACEPIPE_TIMEOUT, unrestricted_access);

        OVERLAPPED overlapped;
        overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

        ResetEvent(overlapped.hEvent);

        // connect to pipe
        BOOL is_connected = ConnectNamedPipe(pipe, &overlapped);

        DWORD error = GetLastError();

        X_INFO("GetLastError() = {}", error);

        if ((0 == is_connected) && (ERROR_PIPE_CONNECTED == error))
        {
            // connected - client has connected between call to CreatePipe and ConnectPipe
            state = TS_STATE_CONNECTED;

            emit new_connection(quintptr(pipe));
        }
        else if (((0 == is_connected) && ((ERROR_PIPE_LISTENING == error) || (ERROR_IO_PENDING == error))) || (1 == is_connected))
        {
            // waiting for connection loop
            while ((state == TS_STATE_READY) && (!QThread::currentThread()->isInterruptionRequested()))
            {
                if (WaitForSingleObject(overlapped.hEvent, 10) == WAIT_OBJECT_0)
                {
                    state = TS_STATE_CONNECTED;

                    emit new_connection(quintptr(pipe));
                }
            }

            if(state != TS_STATE_CONNECTED)
            {
                DisconnectNamedPipe(pipe);
                CloseHandle(overlapped.hEvent);
            }
        }
        else
        {
            // error
            state = TS_STATE_ERROR;

            emit server_error();
        }
    }
}

#else

#include <QDir>

LocalServerPrivate::LocalServerPrivate(QObject *parent) :
    QLocalServer(parent)
{
    X_CALL;
}

LocalServerPrivate::~LocalServerPrivate()
{
    X_CALL;
}

bool LocalServerPrivate::listen(const QString &name)
{
    X_CALL;

    if(!QLocalServer::listen("/tmp/" + name))
    {
        X_ERROR("can`t run server [{}]: {}", name, QLocalServer::errorString());
    }
    else
    {
        chmod(fullServerName().toLocal8Bit().data(), ALLPERMS);

        X_IMPORTANT("server listen at {}", fullServerName());
    }

    return is_listening();
}

bool LocalServerPrivate::is_listening() const
{
    return isListening();
}

void LocalServerPrivate::incomingConnection(quintptr socket_descriptor)
{
    X_CALL;

    emit new_connection(socket_descriptor);
}

#endif
