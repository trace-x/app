#include "local_server.h"
#include "local_server_p.h"

#include "trace_x/trace_x.h"

LocalServer::LocalServer(QObject *parent) :
    QObject(parent),
    _p(new LocalServerPrivate)
{
    X_CALL;

    connect(_p, &LocalServerPrivate::new_connection, this, &LocalServer::new_connection);
}

LocalServer::~LocalServer()
{
    X_CALL;

    delete _p;
}

bool LocalServer::start(const QString &name)
{
    X_CALL;

    return _p->listen(name);
}

void LocalServer::stop()
{
    X_CALL;

    _p->close();
}

bool LocalServer::is_listening() const
{
    return _p->is_listening();
}
