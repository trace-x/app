%module trace_x

%include "std_string.i"
%include "stdint.i"

%{
#include <traceint/trace_x.h>
%}

namespace trace_x
{

enum MessageType
{
    MESSAGE_CALL,
    MESSAGE_RETURN,
    MESSAGE_INFO,
    MESSAGE_DEBUG,
    MESSAGE_IMPORTANT,
    MESSAGE_EVENT,
    MESSAGE_WARNING,
    MESSAGE_ERROR,
    MESSAGE_EXCEPTION,
    MESSAGE_ASSERT,
    MESSAGE_PARAMETERS,
    MESSAGE_VALUE,
    MESSAGE_SUSPEND,
    MESSAGE_RESUME,
    MESSAGE_SIGNAL,
    MESSAGE_DATA,

    _MESSAGE_END_
};

}

%inline %{

#include <iostream>

void trace_message_h(const char* module, const char *source, int line, const char *function, uint64_t object, char message_type, const std::string &message)
{
    trace_x::message_context_t context { module, strlen(module) + 1, source, strlen(source) + 1, line, function, strlen(function) + 1, (const void *)(object), "", 1, message_type};

    int status;

    if(trace_x::trace_check(context, &status))
    {
        trace_x::trace_message(context, message.c_str(), message.length(), status);
    }
}

%}

%pythoncode %{

def trace_call(logger):
    def fun_wrapper(fun):
        def wrapper(*args):
            full_name = []
            context = 0

            if fun.func_code.co_argcount >= 1 and fun.func_code.co_varnames[0] == 'self':
                self_arg = args[0]
                context = id(self_arg)
                full_name = self_arg.__class__.__name__ + '::' + fun.__name__ + '()'
            else:
                if fun.__name__ == '<module>':
                    full_name = 'main()'
                else:
                    full_name = fun.__name__ + '()'

            trace_message_h(logger.module_name, fun.func_code.co_filename, fun.func_code.co_firstlineno, full_name, context, MESSAGE_CALL, '')

            fun(*args)

            trace_message_h(logger.module_name, fun.func_code.co_filename, fun.func_code.co_firstlineno, full_name, context, MESSAGE_RETURN, '')
        return wrapper
    return fun_wrapper

class Logger:
    def __init__(self, module_name):
        self.module_name = module_name

    def trace_message(self, type, message):
        import inspect

        (frame, filename, line_number, function_name, lines, index) = inspect.stack()[2]

        full_name = []
        context = 0
        if 'self' in frame.f_locals:
            context = id(frame.f_locals['self'])
            full_name = frame.f_locals['self'].__class__.__name__ + '::' + function_name + '()'
        else:
            if function_name == '<module>':
                function_name = 'main'

            full_name = function_name + '()'

        trace_message_h(self.module_name, filename, line_number, full_name, context, type, message)

    def info(self, message):
        self.trace_message(MESSAGE_INFO, message)

    def debug(self, message):
        self.trace_message(MESSAGE_DEBUG, message)

    def important(self, message):
        self.trace_message(MESSAGE_IMPORTANT, message)

    def warning(self, message):
        self.trace_message(MESSAGE_WARNING, message)

    def error(self, message):
        self.trace_message(MESSAGE_ERROR, message)

    def exception(self, message):
        self.trace_message(MESSAGE_EXCEPTION, message)

    def assertion(self, message):
        self.trace_message(MESSAGE_ASSERT, message)

    def value(self, message):
        self.trace_message(MESSAGE_VALUE, message)

    def signal(self, message):
        self.trace_message(MESSAGE_SIGNAL, message)
%}
