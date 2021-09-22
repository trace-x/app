#ifndef LOCAL_CONNECTION_CONTROLLER_H
#define LOCAL_CONNECTION_CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QLinkedList>

#include <boost/interprocess/managed_shared_memory.hpp>

#include "process_model.h"

class LocalConnection;

//! Класс управления локальным соединением с _одним_ процессом
//! Занимается первичной обработкой входящих пакетов
class LocalConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit LocalConnectionController(TraceController *trace_controller, const QString &name, QObject *parent = 0);

    ~LocalConnectionController();

    void process_packet(const QByteArray &data, bool &ack);
    void stop_async();
    void stop_sync();

    void clear();

signals:
    void process_registered();
    void connection_closed();

private:
    void register_process(uint64_t pid, uint64_t timestamp, const QString &process_name, const QString &user_name);
    void controller_thread();

private:
    friend class TraceServer;

    ProcessModel *_process_model;
    TraceController *_trace_controller;
    LocalConnection *_connection;

    QString _srv_name;
    std::string _shm_name;
    std::string _mutex_name;
    boost::interprocess::managed_shared_memory _filter_shm;
    trace_x::filter_index _filter_index;

    int8_t *_srv_flag;

    QThread _thread;

    QLinkedList<raw_message_t*> _list;

    QMutex _mutex;

    uint64_t _disconnect_time;

    bool _drop_buffer;
};

#endif // LOCAL_CONNECTION_CONTROLLER_H
