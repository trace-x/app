#define X_INFO_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_INFO, ##__VA_ARGS__) //! INFO message for member functions with printf formatting
#define X_INFO_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_INFO, ##__VA_ARGS__) //! INFO message for global functions with printf formatting

#define X_DEBUG_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_DEBUG, ##__VA_ARGS__) //! DEBUG message for member functions with printf formatting
#define X_DEBUG_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_DEBUG, ##__VA_ARGS__) //! DEBUG message for global functions with printf formatting

#define X_WARNING_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_WARNING, ##__VA_ARGS__) //! WARNING message for member functions with printf formatting
#define X_WARNING_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_WARNING, ##__VA_ARGS__) //! WARNING message for global functions with printf formatting

#define X_ERROR_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_ERROR, ##__VA_ARGS__) //! ERROR message for member functions with printf formatting
#define X_ERROR_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_ERROR, ##__VA_ARGS__) //! ERROR message for global functions with printf formatting

#define X_EXCEPTION_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_EXCEPTION, ##__VA_ARGS__) //! EXCEPTION message for member functions with printf formatting
#define X_EXCEPTION_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_EXCEPTION, ##__VA_ARGS__) //! EXCEPTION message for global functions with printf formatting

#define X_IMPORTANT_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_IMPORTANT, ##__VA_ARGS__) //! IMPORTANT message for member functions with printf formatting
#define X_IMPORTANT_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_IMPORTANT, ##__VA_ARGS__) //! IMPORTANT message for global functions with printf formatting

#define X_VALUE_S(_x_, _printspec_) TRACE_X_INFO_VALUE_BODY(_x_, _printspec_,) //! print given value `_x_` for member functions with given printf specificator `_printspec_`
#define X_VALUE_S_F(_x_, _printspec_) TRACE_X_INFO_VALUE_BODY(_x_, _printspec_, F) //! print given value `_x_` for global functions with printf specificator `_printspec_`

#define X_PARAMS_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_PARAMETERS, ##__VA_ARGS__) //! PARAMETERS message for member functions with printf formatting
#define X_PARAMS_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_PARAMETERS, ##__VA_ARGS__) //! PARAMETERS message for global functions with printf formatting

#define X_EVENT_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_EVENT, ##__VA_ARGS__) //! EVENT message for member functions with printf formatting
#define X_EVENT_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_EVENT, ##__VA_ARGS__) //! EVENT message for global functions with printf formatting

#define X_SUSPEND_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_SUSPEND, ##__VA_ARGS__) //! SUSPEND message for member functions with printf formatting
#define X_SUSPEND_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_SUSPEND, ##__VA_ARGS__) //! SUSPEND message for global functions with printf formatting

#define X_RESUME_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_RESUME, ##__VA_ARGS__) //! RESUME message for member functions with printf formatting
#define X_RESUME_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_RESUME, ##__VA_ARGS__) //! RESUME message for global functions with printf formatting

#define X_SIGNAL_S(...) TRACE_X_TRACE_S(trace_x::MESSAGE_SIGNAL, ##__VA_ARGS__) //! SIGNAL message for member functions with printf formatting
#define X_SIGNAL_S_F(...) TRACE_X_TRACE_S_F(trace_x::MESSAGE_SIGNAL, ##__VA_ARGS__) //! SIGNAL message for global functions with printf formatting
