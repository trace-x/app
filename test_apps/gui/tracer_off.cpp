#include "tracer.h"

#define TRACE_X_OFF

#define TRACE_X_MODULE_NAME "tracer_module"

#include "trace_x/trace_x.h"

void Tracer::off_test()
{
    X_CALL;
    X_DEBUG("1");
    X_DEBUG_F("1");
    X_DEBUG_S("1");
    X_DEBUG_S_F("1");
    X_INFO("1");
    X_INFO_F("1");
    X_INFO_S("1");
    X_INFO_S_F("1");
    X_WARNING("1");
    X_WARNING_F("1");
    X_WARNING_S("1");
    X_WARNING_S_F("1");
    X_ERROR("1");
    X_ERROR_F("1");
    X_ERROR_S("1");
    X_ERROR_S_F("1");
    X_EXCEPTION("1");
    X_EXCEPTION_F("1");
    X_EXCEPTION_S("1");
    X_EXCEPTION_S_F("1");
    X_IMPORTANT("1");
    X_IMPORTANT_F("1");
    X_IMPORTANT_S("1");
    X_IMPORTANT_S_F("1");
    X_ASSERT(1 == 2);
    X_ASSERT_F(1 == 2);

    X_VALUE(1);
    X_VALUES("numbers", 1, 2, 3);
    X_PARAMS("1");
    X_PARAMS_F("1");
    X_PARAMS_S("1");
    X_PARAMS_S_F("1");

    X_SUSPEND("1");
    X_SUSPEND_F("1");
    X_SUSPEND_S("1");
    X_SUSPEND_S_F("1");

    X_RESUME("1");
    X_RESUME_F("1");
    X_RESUME_S("1");
    X_RESUME_S_F("1");

    X_SIGNAL("1");
    X_SIGNAL_F("1");
    X_SIGNAL_S("1");
    X_SIGNAL_S_F("1");

//    X_GUID(1);
//    X_GUID_F(1);
}
