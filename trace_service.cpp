#include "trace_service.h"
#include "settings.h"

#include "trace_x/trace_x.h"

#include <QApplication>

TraceService::TraceService(QObject *parent) :
    QObject(parent)
{
    X_CALL;

    _trace_controller = new TraceController;

    _trace_server = new TraceServer(*_trace_controller, x_settings().server_name_option->string_value());

    update_config();

    connect(&x_settings(), &XSettings::settings_changed, this, &TraceService::update_config);
}

TraceService::~TraceService()
{
    X_CALL;

    delete _trace_server;
    delete _trace_controller;
}

void TraceService::set_server_state(bool enable)
{
    X_CALL;

    if(_trace_server)
    {
        if(enable && !_trace_controller->loaded_file().isEmpty())
        {
            _trace_controller->clear();
        }

        _trace_server->enable(enable);
    }
}

void TraceService::update_config()
{
    X_CALL;

    _trace_controller->set_message_limit(x_settings().message_limit_option->uint_value());
    _trace_controller->set_file_data_limit(x_settings().file_data_limit_option->uint_value() * 1024ull * 1024ull);
    _trace_controller->data_storage().set_memory_limit(x_settings().memory_limit_option->uint_value() * 1024ull * 1024ull);
    _trace_controller->data_storage().set_swap_limit(x_settings().swap_limit_option->uint_value()  * 1024ull * 1024ull);
    _trace_controller->data_storage().set_swap_state(!x_settings().no_swap_option->bool_value());
    _trace_controller->data_storage().set_swap_dir(x_settings().swap_path_option->string_value());
}

TraceController &TraceService::trace_controller()
{
    return *_trace_controller;
}

TraceServer &TraceService::trace_server()
{
    return *_trace_server;
}
