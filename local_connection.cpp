#include "local_connection.h"
#include "local_connection_p.h"

#include <stdint.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "trace_x/trace_x.h"

LocalConnection::LocalConnection(quintptr descriptor, LocalConnectionController &controller, QObject *parent):
    QThread(parent),
    _descriptor(descriptor),
    _controller(controller)
{
    X_CALL;
}

LocalConnection::~LocalConnection()
{
    X_CALL;

    requestInterruption();
    quit();
    wait();
}

#ifdef Q_OS_WIN

void LocalConnection::run()
{
    X_CALL;

    QByteArray message_buffer;

    OVERLAPPED overlapped;
    overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    ResetEvent(overlapped.hEvent);

    HANDLE pipe = HANDLE(_descriptor);

    tTraceServerState state = TS_STATE_CONNECTED;

    char message[4096];

    overlapped.Internal = 0;
    overlapped.InternalHigh = 0;
    overlapped.Offset = 0;
    overlapped.OffsetHigh = 0;

    //

    qintptr bytes_in_buffer = 0;
    qintptr current_frame_size = 0;
    size_t offset = 0;

    //

    while ((state == TS_STATE_CONNECTED) && (!QThread::currentThread()->isInterruptionRequested()))
    {
        // read message in async manner
        bool bReadSuccess = ReadFile(pipe, message + bytes_in_buffer, sizeof(message) - bytes_in_buffer, NULL, &overlapped);

        bool bReadPending = (GetLastError() == ERROR_IO_PENDING) ? true : false;

        if (!bReadSuccess && !bReadPending)
        {
            // error condition
            state = TS_STATE_ERROR;

            emit connection_error(this);
        }
        else
        {
            // wait for one of 3 conditions:
            // -write on client side of pipe
            // -error on client side of pipe
            // -abort request
            while (!QThread::currentThread()->isInterruptionRequested())
            {
                DWORD wait_result = WaitForSingleObject(overlapped.hEvent, 10);

                if (wait_result == WAIT_OBJECT_0)
                {
                    // success waiting
                    // query IO result
                    DWORD bytes_readed;

                    if (!GetOverlappedResult(pipe, &overlapped, &bytes_readed, FALSE))
                    {
                        // IO error
                        state = TS_STATE_ERROR;

                        emit connection_error(this);
                    }
                    else
                    {
                        offset = 0;

                        bytes_in_buffer += bytes_readed;

                        while(true)
                        {
                            if(current_frame_size != 0)
                            {
                                qintptr chunk = qMin(current_frame_size, qintptr(bytes_in_buffer));

                                message_buffer.append(message + offset, chunk);

                                offset += chunk;
                                current_frame_size -= chunk;
                                bytes_in_buffer -= chunk;
                            }

                            if((current_frame_size == 0) && (!message_buffer.isEmpty()))
                            {
                                bool ack_needed = false;

                                _controller.process_packet(message_buffer, ack_needed);

                                message_buffer.clear();

                                current_frame_size = 0;

                                if(ack_needed)
                                {
                                    this->ack();
                                }
                            }

                            if(bytes_in_buffer >= sizeof(quint64))
                            {
                                //read frame size(minus size field)
                                current_frame_size = *(quint64*)(message + offset) - sizeof(quint64);

                                offset += sizeof(quint64); //rest - is frame data
                                bytes_in_buffer -= sizeof(quint64);

                                message_buffer.reserve(current_frame_size);
                            }
                            else
                            {
                                if(bytes_in_buffer != 0)
                                {
                                    memcpy(message, message + offset, bytes_in_buffer);
                                }

                                break;
                            }
                        }
                    }

                    break;
                }
                else if (wait_result == WAIT_ABANDONED)
                {
                    // error waiting
                    state = TS_STATE_ERROR;

                    emit connection_error(this);

                    break;
                }
            }

            if ((!QThread::currentThread()->isInterruptionRequested()) && (state == TS_STATE_CONNECTED))
            {
#ifdef USE_RETURN_CHANNEL
                BYTE reply_ack = 0x07;
                DWORD n_write;
                n_transact++;
                if (n_transact == ACK_PERIOD)
                {
                    n_transact = 0;
                    WriteFile(m_hTracePipe, &reply_ack, 1, &n_write, NULL);
                }
#endif
            }
            else
            {
                CancelIo(pipe);
            }
        }
    }

    DisconnectNamedPipe(pipe);

    if (TS_STATE_CONNECTED == state)
    {
        emit disconnected(this);
    }

    CloseHandle(overlapped.hEvent);
}

void LocalConnection::ack()
{
    BYTE replyACK = 0x07;
    DWORD nWr;

    WriteFile(HANDLE(_descriptor), &replyACK, 1, &nWr, NULL);
}

#else

#include <QLocalSocket>

void LocalConnection::run()
{
    X_CALL;

    QLocalSocket socket;

    if(!socket.setSocketDescriptor(_descriptor))
    {
        return;
    }

    QByteArray message_buffer;
    qint64 current_frame_size = 0;

    while(!QThread::currentThread()->isInterruptionRequested())
    {
        if(socket.waitForReadyRead(10))
        {
            while(true)
            {
                if(current_frame_size != 0)
                {
                    qint64 chunk = qMin(current_frame_size, socket.bytesAvailable());

                    message_buffer.append(socket.read(chunk));

                    current_frame_size -= chunk;
                }

                if((current_frame_size == 0) && (!message_buffer.isEmpty()))
                {
                    bool ack_needed = false;

                    _controller.process_packet(message_buffer, ack_needed);

                    message_buffer.clear();

                    current_frame_size = 0;

                    if(ack_needed)
                    {
                        uint8_t data = 0;

                        socket.write((const char*)(&data), sizeof(data));
                        socket.waitForBytesWritten();
                    }
                }

                if(socket.bytesAvailable() >= sizeof(quint64))
                {
                    //read frame size(minus size field)
                    current_frame_size = *(quint64*)(socket.read(sizeof(quint64)).constData()) - sizeof(quint64);

                    message_buffer.reserve(current_frame_size);
                }
                else
                {
                    break;
                }
            }
        }

        if(socket.state() != QLocalSocket::ConnectedState)
        {
            emit disconnected(this);

            break;
        }
    }

    socket.readAll();
    socket.abort();
}

#endif


