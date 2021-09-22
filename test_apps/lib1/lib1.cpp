#include "lib1.h"

#define TRACE_X_MODULE_NAME "lib1"

#include <iostream>
#include "trace_x/trace_x.h"

#include "test_apps/lib0/lib0.h"

namespace libone
{

void foo()
{
    X_CALL_F;

    std::cerr << "from libone " << libzero::Singletone::instance() << std::endl;
}
}

namespace libzero
{

void foo()
{
    X_CALL_F;

    X_INFO_F("this is lib1");
}

}

