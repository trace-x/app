#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define TRACE_X_MODULE_NAME "test_app_2"

#include "trace_x/trace_x.h"

int main(void)
{
    int pi = 4;

    X_BEGIN;

    X_INFO("%s from c library", "hello");
    X_DEBUG("%s from c library", "hello");
    X_WARNING("%s from c %s", "hello", "library");
    X_ERROR("%s from c library", "hello");
    X_IMPORTANT("%s from c library", "hello");
    X_PARAMS("%s from c library", "hello");
    X_ASSERT(pi == 3);
    X_VALUE(pi, "%d");

    X_END;

    return 0;
}
