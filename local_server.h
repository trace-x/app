#ifndef LOCAL_SERVER_H
#define LOCAL_SERVER_H

#include <QObject>

#include "local_connection.h"

class LocalServerPrivate;

class LocalServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalServer(QObject *parent = 0);
    virtual ~LocalServer();

    bool start(const QString &name);
    void stop();

    bool is_listening() const;

signals:
    void new_connection(quintptr descriptor);
    void disconnected(LocalConnection *receiver);

private:
    LocalServerPrivate *_p;
};

#endif // LOCAL_SERVER_H
