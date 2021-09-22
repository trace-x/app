#include "lib1.h"

#define TRACE_X_MODULE_NAME "lib2"

#include <iostream>
#include "trace_x/trace_x.h"

#include "test_apps/lib0/lib0.h"

namespace libone
{

inline void foo()
{
    X_CALL_F;
}

}

namespace libtwo
{

void foo()
{
    X_CALL_F;

    std::cerr << "from libtwo " << libzero::Singletone::instance() << std::endl;

    libone::foo();
}

}

