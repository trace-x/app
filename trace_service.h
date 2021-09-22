#ifndef TRACE_SERVICE_H
#define TRACE_SERVICE_H

#include <QObject>

#include "trace_server.h"
#include "trace_model.h"
#include "trace_controller.h"

//! Main trace system class
class TraceService : public QObject
{
    Q_OBJECT
public:
    explicit TraceService(QObject *parent = 0);

    ~TraceService();

    TraceController & trace_controller();
    TraceServer & trace_server();

public slots:
    void set_server_state(bool enable);

private:
    void update_config();

private:
    TraceController *_trace_controller;
    TraceServer *_trace_server;
};

#endif // TRACE_SERVICE_H
