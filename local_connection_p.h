#ifndef LOCAL_CONNECTION_PRIVATE_H
#define LOCAL_CONNECTION_PRIVATE_H

#include <qsystemdetection.h>

typedef enum _tTraceServerState // internal state of server
{
    TS_STATE_READY = 0,     // waiting for connection
    TS_STATE_CONNECTED,     // connected, receiving messages
    TS_STATE_ERROR,         // error occured
    TS_STATE_DISCONNECTED   // finally disconncted (either error or normal condition), must reinitialize
} tTraceServerState;

#endif // LOCAL_CONNECTION_PRIVATE_H
