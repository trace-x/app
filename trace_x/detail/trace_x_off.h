#include "trace_x_undef.h"
#include "types.h"

#ifdef __cplusplus

#define TRACE_X_MESSAGE_NOOP(...) while(false) trace_x::trace_stream_noop()

#define X_CALL
#define X_CALL_F

#define X_INFO TRACE_X_MESSAGE_NOOP
#define X_INFO_F TRACE_X_MESSAGE_NOOP

#define X_DEBUG TRACE_X_MESSAGE_NOOP
#define X_DEBUG_F TRACE_X_MESSAGE_NOOP

#define X_WARNING TRACE_X_MESSAGE_NOOP
#define X_WARNING_F TRACE_X_MESSAGE_NOOP

#define X_ERROR TRACE_X_MESSAGE_NOOP
#define X_ERROR_F TRACE_X_MESSAGE_NOOP

#define X_EXCEPTION TRACE_X_MESSAGE_NOOP
#define X_EXCEPTION_F TRACE_X_MESSAGE_NOOP

#define X_IMPORTANT TRACE_X_MESSAGE_NOOP
#define X_IMPORTANT_F TRACE_X_MESSAGE_NOOP

#define X_ASSERT(_x_) (void)(_x_)
#define X_ASSERT_F(_x_) (void)(_x_)

#define X_VALUE
#define X_VALUE_F
#define X_VALUES(_Froup_name_, ...)
#define X_VALUES_F(_Froup_name_, ...)

#define X_PARAMS
#define X_PARAMS_F

#define X_EVENT TRACE_X_MESSAGE_NOOP
#define X_EVENT_F TRACE_X_MESSAGE_NOOP

#define X_SUSPEND TRACE_X_MESSAGE_NOOP
#define X_SUSPEND_F TRACE_X_MESSAGE_NOOP

#define X_RESUME TRACE_X_MESSAGE_NOOP
#define X_RESUME_F TRACE_X_MESSAGE_NOOP

#define X_SIGNAL TRACE_X_MESSAGE_NOOP
#define X_SIGNAL_F TRACE_X_MESSAGE_NOOP

#define X_GUID
#define X_GUID_F

#define X_T
#define X_CAPTURE_ARG
#define X_CAPTURE_ARGS
#define X_CAPTURE_ARRAY

//! X_IMAGE(image object)
//! X_IMAGE(image object, "alias")
//! X_IMAGE(image object, "alias", "formatted message {}", ...)
#define X_IMAGE
#define X_IMAGE_F

#ifdef TRACE_X_WITH_C_FORMATTING

#define X_INFO_S TRACE_X_MESSAGE_NOOP
#define X_INFO_S_F TRACE_X_MESSAGE_NOOP

#define X_DEBUG_S TRACE_X_MESSAGE_NOOP
#define X_DEBUG_S_F TRACE_X_MESSAGE_NOOP

#define X_WARNING_S TRACE_X_MESSAGE_NOOP
#define X_WARNING_S_F TRACE_X_MESSAGE_NOOP

#define X_ERROR_S TRACE_X_MESSAGE_NOOP
#define X_ERROR_S_F TRACE_X_MESSAGE_NOOP

#define X_EXCEPTION_S TRACE_X_MESSAGE_NOOP
#define X_EXCEPTION_S_F TRACE_X_MESSAGE_NOOP

#define X_IMPORTANT_S TRACE_X_MESSAGE_NOOP
#define X_IMPORTANT_S_F TRACE_X_MESSAGE_NOOP

#define X_VALUE_S(_x_, _printspec_)
#define X_VALUE_S_F(_x_, _printspec_)

#define X_PARAMS_S TRACE_X_MESSAGE_NOOP
#define X_PARAMS_S_F TRACE_X_MESSAGE_NOOP

#define X_EVENT_S TRACE_X_MESSAGE_NOOP
#define X_EVENT_S_F TRACE_X_MESSAGE_NOOP

#define X_SUSPEND_S TRACE_X_MESSAGE_NOOP
#define X_SUSPEND_S_F TRACE_X_MESSAGE_NOOP

#define X_RESUME_S TRACE_X_MESSAGE_NOOP
#define X_RESUME_S_F TRACE_X_MESSAGE_NOOP

#define X_SIGNAL_S TRACE_X_MESSAGE_NOOP
#define X_SIGNAL_S_F TRACE_X_MESSAGE_NOOP

#endif //#ifdef TRACE_X_WITH_C_FORMATTING

#else //#ifdef __cplusplus

#define X_BEGIN
#define X_END
#define X_INFO
#define X_DEBUG
#define X_WARNING
#define X_ERROR
#define X_IMPORTANT
#define X_ASSERT(_x_) (void)(_x_)
#define X_VALUE
#define X_PARAMS

#endif //#ifdef __cplusplus
