#ifndef LOCAL_SERVER_PRIVATE_H
#define LOCAL_SERVER_PRIVATE_H

#include <QObject>
#include <QThread>

#include "local_connection.h"

#ifdef Q_OS_WIN

class LocalServerPrivate : public QObject
{
    Q_OBJECT

public:
    explicit LocalServerPrivate(QObject *parent = 0);

    virtual ~LocalServerPrivate();

    bool listen(const QString &_name);
    void close();

    bool is_listening() const;

signals:
    void new_connection(quintptr descriptor);
    void server_error();

private slots:
    void listener();

private:
    QString _name;
    QThread _listener_thread;
};

#else

#include <QLocalServer>

class LocalServerPrivate : public QLocalServer
{
    Q_OBJECT
public:
    explicit LocalServerPrivate(QObject *parent = 0);

    virtual ~LocalServerPrivate();

    bool listen(const QString &name);
    bool is_listening() const;

protected:
    void incomingConnection(quintptr socket_descriptor);

signals:
    void new_connection(quintptr descriptor);
    void server_error();
};

#endif //Q_OS_WIN

#endif // LOCAL_SERVER_PRIVATE_H
