// NOTE: add definition QT_MESSAGELOGCONTEXT to provide context information in Release mode

#ifdef TRACE_X_ON

#include <QDebug>

#include "../trace_x.h"

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "qt"

#define QT_MESSAGE(type, context, message) if(context.line) trace_x::trace_message(TRACE_X_MODULE_NAME, sizeof(TRACE_X_MODULE_NAME), context.file, strlen(context.file) - 1, context.line, context.function, strlen(context.function) - 1, (const void *)(0), "", 1, type, "%s", message.toLocal8Bit().data()); else trace_x::trace_message(TRACE_X_MODULE_NAME, sizeof(TRACE_X_MODULE_NAME), TRACE_X_FILE_UNKNOWN, sizeof(TRACE_X_FILE_UNKNOWN), 0, TRACE_X_FUNCTION_UNKNOWN, sizeof(TRACE_X_FUNCTION_UNKNOWN), (const void *)(0), "", 1, type, "%s", message.toLocal8Bit().data())

static QtMessageHandler Default_Handler;

namespace
{
    void trace_x_handler_qt(QtMsgType type, const QMessageLogContext &context, const QString &message)
    {
        switch (type)
        {
        case QtDebugMsg:
            QT_MESSAGE(trace_x::MESSAGE_DEBUG, context, message);
            break;
        case QtWarningMsg:
            QT_MESSAGE(trace_x::MESSAGE_WARNING, context, message);
            break;
        case QtCriticalMsg:
            QT_MESSAGE(trace_x::MESSAGE_ERROR, context, message);
            break;
        case QtFatalMsg:
            QT_MESSAGE(trace_x::MESSAGE_ERROR, context, message);
            abort();
        }

        Default_Handler(type, context, message);
    }
}

class TraceQtHandler
{
public:
    TraceQtHandler()
    {
        Default_Handler = qInstallMessageHandler(::trace_x_handler_qt);
    }
};

static const TraceQtHandler Handler;

#endif
