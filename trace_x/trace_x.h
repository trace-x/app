/// Trace-X library interface
///
/// Copyright(c) 2014-2017 Nikita Kozhanov.

#ifndef TRACE_X_OFF
# ifndef TRACE_X_ON
#  define TRACE_X_ON
# endif
#endif

#include "detail/types.h"

#ifdef TRACE_X_ON

#ifndef TRACE_X_PROFILE

#include "detail/trace.h"

#include "detail/trace_x_undef.h"

#ifdef __cplusplus

//! CALL-RET message pair for member and free functions
#define X_CALL TRACE_X_CALL
#define X_CALL_F TRACE_X_CALL_F

#include "detail/trace_x_ex.h"

#ifdef TRACE_X_WITH_C_FORMATTING
    #include "detail/trace_x_s.h"
#endif

//! DEBUG message for member and free functions
#define X_DEBUG(...) TRACE_X_TRACE(trace_x::MESSAGE_DEBUG, ##__VA_ARGS__)
#define X_DEBUG_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_DEBUG, ##__VA_ARGS__)

//! INFO message for member and free functions
#define X_INFO(...) TRACE_X_TRACE(trace_x::MESSAGE_INFO, ##__VA_ARGS__)
#define X_INFO_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_INFO, ##__VA_ARGS__)

//! WARNING message for member and free functions
#define X_WARNING(...) TRACE_X_TRACE(trace_x::MESSAGE_WARNING, ##__VA_ARGS__)
#define X_WARNING_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_WARNING, ##__VA_ARGS__)

//! ERROR message for member and free functions
#define X_ERROR(...) TRACE_X_TRACE(trace_x::MESSAGE_ERROR, ##__VA_ARGS__)
#define X_ERROR_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_ERROR, ##__VA_ARGS__)

//! EXCEPTION message for member and free functions
#define X_EXCEPTION(...) TRACE_X_TRACE(trace_x::MESSAGE_EXCEPTION, ##__VA_ARGS__)
#define X_EXCEPTION_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_EXCEPTION, ##__VA_ARGS__)

//! IMPORTANT message for member and free functions
#define X_IMPORTANT(...) TRACE_X_TRACE(trace_x::MESSAGE_IMPORTANT, ##__VA_ARGS__)
#define X_IMPORTANT_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_IMPORTANT, ##__VA_ARGS__)

//! PARAMETERS message for member and free functions
#define X_PARAMS(...) TRACE_X_TRACE(trace_x::MESSAGE_PARAMETERS, ##__VA_ARGS__)
#define X_PARAMS_F(...) TRACE_X_TRACE_F(trace_x::MESSAGE_PARAMETERS, ##__VA_ARGS__)

//! ASSERT message, prints if expression `_x_` is NOT TRUE for member and free functions
#define X_ASSERT(_x_) TRACE_X_ASSERT_BODY(_x_,)
#define X_ASSERT_F(_x_) TRACE_X_ASSERT_BODY(_x_, F)

//! X_VALUE macro have 3 overloaded forms:
//! X_VALUE(<variable>) - prints VALUE message "<variable_name> : <value_of_variable>"
//! X_VALUE("<alias>", <variable>) - prints VALUE message "<alias> : <value_of_variable>"
//! X_VALUE("<variable_name>", "<format string>", ...) - prints VALUE message "<variable_name> : <formatted string>"

#define X_VALUE(...) TRACE_X_12N_MACRO(TRACE_X_VALUE_F_, TRACE_X_VALUE_2_, TRACE_X_VALUE_1_, ##__VA_ARGS__) //! print given value for member functions
#define X_VALUE_F(...) TRACE_X_12N_MACRO(TRACE_X_F_VALUE_F_, TRACE_X_F_VALUE_2_, TRACE_X_F_VALUE_1_, ##__VA_ARGS__) //! print given value for free functions

//! X_IMAGE macro have 3 overloaded forms:
//! X_IMAGE(<image object>) - send IMAGE message "<variable_name>" : <image data>
//! X_IMAGE(<image object>, "<alias>") - send IMAGE message "<alias>" : <image data>
//! X_IMAGE(<image object>, "<alias>", "<format>", ...) - send IMAGE message "<alias>" : "<formatted message>" : <image data>

#define X_IMAGE(...) TRACE_X_123N_MACRO(TRACE_X_IMAGE_F_, TRACE_X_IMAGE_3_, TRACE_X_IMAGE_2_, TRACE_X_IMAGE_1_, ##__VA_ARGS__)
#define X_IMAGE_F(...) TRACE_X_123N_MACRO(TRACE_X_F_IMAGE_F_, TRACE_X_F_IMAGE_3_, TRACE_X_F_IMAGE_2_, TRACE_X_F_IMAGE_1_, ##__VA_ARGS__)

///////////////////////////////////////////// helpers macros /////////////////////////////////////////////

//! Capture macro for arguments with automatic format: "<arg 1 name> : {}; <arg 2 name> : {}; ..."
//! example: X_INFO(X_T(r, g, b)) produce "r = 255; g = 120; b = 0" message

#define X_T TRACE_X_TRACE_T

//! Capture macro for raw array pointers
//! example:
//! int sarr[] = { 1, 4, 9, 16, 32 };
//! X_INFO("this is raw c-array: {}", X_CAPTURE_ARRAY(sarr));
//! int *darr = new int[darr_size];
//! X_INFO("this is raw c-array: {}", X_CAPTURE_ARRAY(darr, darr_size));

#define X_CAPTURE_ARRAY TRACE_X_CAPTURE_ARRAY

//! Capture macro for named argument formatting
//! example: X_INFO("coordinates: {x} {y}", X_CAPTURE_ARG("x", point_1), X_CAPTURE_ARG("y", point_2));

#define X_CAPTURE_ARG(id, index) ::fmt::arg(id, index)

//! Capture macro for multiple named arguments formatting
//! example: X_INFO("coordinates: {x}, {y}", X_CAPTURE_ARGS(x, y));

#define X_CAPTURE_ARGS FMT_CAPTURE

#else /// #ifdef __cplusplus
  #include "detail/trace_x_c.h"
#endif /// #ifdef __cplusplus

#else /// ifndef TRACE_X_PROFILE

  #include "detail/trace.h"
  #include "detail/trace_x_off.h"

  #undef X_CALL
  #undef X_CALL_F

  #define X_CALL TRACE_X_CALL
  #define X_CALL_F TRACE_X_CALL_F

#endif /// ifndef TRACE_X_PROFILE

#else /// #ifdef TRACE_X_ON

#include "detail/trace_x_off.h"

#endif /// #ifdef TRACE_X_ON
