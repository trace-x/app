#ifndef TRACE_SERVER_H
#define TRACE_SERVER_H

#include <QThread>

#include <boost/interprocess/managed_shared_memory.hpp>

#include "local_server.h"

//! Класс сервера обработки сообщений
class TraceServer : public QObject
{
    Q_OBJECT
public:
    explicit TraceServer(TraceController &trace_controller, const QString &server_name, QObject *parent = 0);

    ~TraceServer();

    bool is_started() const;

    QString server_name() const;

public slots:
    void enable(bool enable);

signals:
    void state_changed(bool is_enabled);

private slots:
    void add_connection(quintptr descriptor);
    void connection_closed();
    void new_process_registered();

private:
    void start_server(bool force = false);
    void stop_server();

private:
    boost::interprocess::managed_shared_memory _rx_shm;

    int8_t *_rx_flag;

    LocalServer _local_server;
    TraceController &_trace_controller;

    QString _server_name;
};

#endif // TRACE_SERVER_H
