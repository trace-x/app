//! EVENT message for member and free functions
#define X_EVENT(...) TRACE_X_TRACE(trace_x::MESSAGE_EVENT, ##__VA_ARGS__)
#define X_EVENT_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_EVENT, ##__VA_ARGS__)

//! SUSPEND message for member and free functions
#define X_SUSPEND(...) TRACE_X_TRACE(trace_x::MESSAGE_SUSPEND, ##__VA_ARGS__)
#define X_SUSPEND_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_SUSPEND, ##__VA_ARGS__)

//! RESUME message for member and free functions
#define X_RESUME(...) TRACE_X_TRACE(trace_x::MESSAGE_RESUME, ##__VA_ARGS__)
#define X_RESUME_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_RESUME, ##__VA_ARGS__)

//! SIGNAL message for member and free functions
#define X_SIGNAL(...) TRACE_X_TRACE(trace_x::MESSAGE_SIGNAL, ##__VA_ARGS__)
#define X_SIGNAL_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_SIGNAL, ##__VA_ARGS__)

#define X_GUID(_x_) TRACE_X_GUID_BODY(_x_,)
#define X_GUID_F(_x_) TRACE_X_GUID_BODY(_x_, F)
