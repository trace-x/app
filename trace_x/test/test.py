import traceint

# def trace_call(logger):
#     def fun_wrapper(fun):
#         def wrapper(*args):
#             full_name = []
#             context = 0
#
#             if fun.func_code.co_argcount >= 1 and fun.func_code.co_varnames[0] == 'self':
#                 self_arg = args[0]
#                 context = id(self_arg)
#                 full_name = self_arg.__class__.__name__ + '::' + fun.__name__ + '()'
#             else:
#                 if fun.__name__ == '<module>':
#                     full_name = 'main()'
#                 else:
#                     full_name = fun.__name__ + '()'
#
#             traceint.trace_message_h(logger.module_name, fun.func_code.co_filename, fun.func_code.co_firstlineno, full_name, context, traceint.MESSAGE_CALL, '')
#
#             fun(*args)
#
#             traceint.trace_message_h(logger.module_name, fun.func_code.co_filename, fun.func_code.co_firstlineno, full_name, context, traceint.MESSAGE_RETURN, '')
#         return wrapper
#     return fun_wrapper
#
# class Logger:
#     def __init__(self, module_name):
#         self.module_name = module_name
#
#     def trace_message(self, type, message):
#         import inspect
#
#         (frame, filename, line_number, function_name, lines, index) = inspect.stack()[2]
#
#         full_name = []
#         context = 0
#         if 'self' in frame.f_locals:
#             context = id(frame.f_locals['self'])
#             full_name = frame.f_locals['self'].__class__.__name__ + '::' + function_name + '()'
#         else:
#             if function_name == '<module>':
#                 function_name = 'main'
#
#             full_name = function_name + '()'
#
#         traceint.trace_message_h(self.module_name, filename, line_number, full_name, context, type, message)
#
#     def info(self, message):
#         self.trace_message(traceint.MESSAGE_INFO, message)
#
#     def debug(self, message):
#         self.trace_message(traceint.MESSAGE_DEBUG, message)
#
#     def important(self, message):
#         self.trace_message(traceint.MESSAGE_IMPORTANT, message)
#
#     def warning(self, message):
#         self.trace_message(traceint.MESSAGE_WARNING, message)
#
#     def error(self, message):
#         self.trace_message(traceint.MESSAGE_ERROR, message)
#
#     def exception(self, message):
#         self.trace_message(traceint.MESSAGE_EXCEPTION, message)
#
#     def assertion(self, message):
#         self.trace_message(traceint.MESSAGE_ASSERT, message)
#
#     def value(self, message):
#         self.trace_message(traceint.MESSAGE_VALUE, message)
#
#     def signal(self, message):
#         self.trace_message(traceint.MESSAGE_SIGNAL, message)

logger = traceint.Logger('logger_module')

class classik:
    @traceint.trace_call(logger)
    def meth(self):
        logger.info('mes')

@traceint.trace_call(logger)
def function_1(par):
    logger.info('mes')
    logger.error('mes')

function_1(3)

logger.info('{0}, {1}, {2}'.format('a', 'b', 'c'))

clas1 = classik()
clas2 = classik()

clas1.meth()
clas1.meth()
clas2.meth()