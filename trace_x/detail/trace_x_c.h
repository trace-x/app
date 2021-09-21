///////////////////////////////////////////// c interface macros /////////////////////////////////////////////

#define TRACE_X_PRINT_(_type_, ...) trace_message(TRACE_X_F_BODY, "", 1, _type_, __VA_ARGS__)

#define X_BEGIN TRACE_X_PRINTF(MESSAGE_CALL, "")
#define X_END TRACE_X_PRINTF(MESSAGE_RETURN, "")
#define X_INFO(...) TRACE_X_PRINTF(MESSAGE_INFO, __VA_ARGS__)
#define X_DEBUG(...) TRACE_X_PRINTF(MESSAGE_DEBUG, __VA_ARGS__)
#define X_WARNING(...) TRACE_X_PRINTF(MESSAGE_WARNING, __VA_ARGS__)
#define X_ERROR(...) TRACE_X_PRINTF(MESSAGE_ERROR, __VA_ARGS__)
#define X_IMPORTANT(...) TRACE_X_PRINTF(MESSAGE_IMPORTANT, __VA_ARGS__)
#define X_ASSERT(_x_) if(!(_x_)) TRACE_X_PRINTF(MESSAGE_ASSERT, "Assertion failed [" #_x_ "]")
#define X_VALUE(_x_, _printspec_) trace_message(TRACE_X_FBODY, #_x_, sizeof(#_x_), MESSAGE_VALUE, _printspec_, _x_) //! print given value `_x_` with given printf specificator `_printspec_`
#define X_PARAMS(...) TRACE_X_PRINTF(MESSAGE_PARAMETERS, __VA_ARGS__)
