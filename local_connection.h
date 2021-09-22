#ifndef LOCAL_CONNECTION_H
#define LOCAL_CONNECTION_H

#include <stdint.h>

#include <QThread>

#include <boost/atomic.hpp>

#include "local_connection_controller.h"

class LocalConnectionPrivate;

//! Класс, реализующий соединение и приём сообщений от процесса
class LocalConnection : public QThread
{
    Q_OBJECT

public:
    explicit LocalConnection(quintptr descriptor, LocalConnectionController &controller, QObject *parent = 0);

    virtual ~LocalConnection();

signals:
    void frame_received(const QByteArray &frame, bool &ack_needed);

    void disconnected(LocalConnection *connection);
    void connection_error(LocalConnection *connection);

protected:
    friend class LocalServerPrivate;

    virtual void run();

private:
    void ack();

private:
    quintptr _descriptor;

    LocalConnectionController &_controller;
};

#endif // LOCAL_CONNECTION_H
