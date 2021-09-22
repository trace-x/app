#include "lib0.h"

#define TRACE_X_MODULE_NAME "lib1"

#include "trace_x/trace_x.h"

namespace libzero
{

void foo()
{
    X_CALL_F;

    X_INFO_F("this is lib0");
}

Singletone::Singletone():
    t(9)
{
}

}

