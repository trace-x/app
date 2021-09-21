#
# Try to find the TraceX library and include path.
# Once done this will define
#
# TRACEX_FOUND
# TRACEX_INCLUDE_PATH
# TRACEX_LIBRARY
# TRACEX_LIBRARIES
# 

if(WIN32)
    find_path( TRACEX_INCLUDE_PATH trace_x/traceint.h
        $ENV{TRACEX_ROOT}/include
        $ENV{TRACEX_ROOT}
        DOC "The directory where trace_x/traceint.h resides")
    find_library( TRACEX_LIBRARY
        NAMES trace_x
        PATHS
        $ENV{TRACEX_ROOT}/lib
        $ENV{TRACEX_ROOT}
        DOC "The trace_x library")
else()
    find_path( TRACEX_INCLUDE_PATH trace_x/traceint.h
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where trace_x/traceint.h resides")
    find_library( TRACEX_LIBRARY
        NAMES trace_x
        PATHS
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /sw/lib
        /opt/local/lib
        DOC "The trace_x library")
endif()

set(TRACEX_LIBRARIES ${TRACEX_LIBRARY})

if(TRACEX_INCLUDE_PATH AND TRACEX_LIBRARY)
    set( TRACEX_FOUND TRUE CACHE BOOL "Set to TRUE if TraceX is found, FALSE otherwise")
else()
    set( TRACEX_FOUND FALSE CACHE BOOL "Set to TRUE if TraceX is found, FALSE otherwise")
endif()

MARK_AS_ADVANCED(
    TRACEX_FOUND
    TRACEX_LIBRARY
    TRACEX_LIBRARIES
    TRACEX_INCLUDE_PATH)
