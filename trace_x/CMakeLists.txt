# Trace-X library

cmake_minimum_required(VERSION 3.12)

project(trace_x)

set(CONFIG config.cmake CACHE STRING "User configuration")

if(EXISTS ${CMAKE_SOURCE_DIR}/${CONFIG})
    message(STATUS "Using config: ${CONFIG}")
    include(${CMAKE_SOURCE_DIR}/${CONFIG})
endif()

set(PUBLIC_HEADERS
    trace_x.h
)

set(DETAIL_HEADERS
    detail/api.h
    detail/print_helper.h
    detail/stl_print.h
    detail/trace.h
    detail/types.h
    detail/traits.h
    detail/trace_x_s.h
    detail/trace_x_off.h
    detail/trace_x_ex.h
    detail/trace_x_c.h
    detail/trace_x_undef.h
)

set(FMT_HEADERS
    detail/fmt/format.h
    detail/fmt/ostream.h
    detail/fmt/printf.h
)

set(TOOL_HEADERS
    tools/cv_image_trace_x.h
    tools/qt_image_trace_x.h
    tools/qt_print.h
    tools/qt_trace_handler.h
)

set(SOURCES
    ${PUBLIC_HEADERS}
    ${DETAIL_HEADERS}
    ${FMT_HEADERS}
    ${TOOL_HEADERS}
    impl/trace.cpp
    impl/logger.cpp
    impl/platform.h
    impl/types.h
    impl/stack_trace.h
    impl/std_restreamer.h
)

if(${WIN32})
    set(SOURCES ${SOURCES}
        impl/stack_walker.h
        impl/stack_walker.cpp)
endif()

macro(declare_compile_definitions target)

    # prevent warnings
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS=1)

    target_compile_definitions(${target} PRIVATE BOOST_DATE_TIME_NO_LIB=1)
    target_compile_definitions(${target} PRIVATE TRACE_X_WITH_C_FORMATTING=1)
    target_compile_definitions(${target} PRIVATE TRACE_X_MODULE_NAME=\"libtrace_x\")

    # for spdlog support

    target_compile_definitions(${target} PRIVATE _GLIBCXX_USE_NANOSLEEP=1)
    target_compile_definitions(${target} PRIVATE _GLIBCXX_USE_SCHED_YIELD=1)

endmacro()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_DEBUG_POSTFIX d)

# define libraries

add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_library(${PROJECT_NAME}_static ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE TRACE_API_EXPORT=1)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE TRACE_STATICLIB=1)

declare_compile_definitions(${PROJECT_NAME})
declare_compile_definitions(${PROJECT_NAME}_static)

if(CPP_STANDARD_FLAG)
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
    set_target_properties(${PROJECT_NAME}_static PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
endif()

#

if(${UNIX})
    target_link_libraries(${PROJECT_NAME} PRIVATE rt dl pthread)
elseif(${WIN32})
endif()

set(Boost_USE_STATIC_LIBS ${LINK_STATIC_BOOST_TO_ARCHIVE})
find_package(Boost ${BOOST_VERSION} COMPONENTS chrono system filesystem)

message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Boost libraries dir: ${Boost_LIBRARY_DIRS}")

link_directories(${Boost_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME}_static PRIVATE ${Boost_LIBRARIES})

set(Boost_USE_STATIC_LIBS ${LINK_STATIC_BOOST_TO_SHARED})
find_package(Boost COMPONENTS chrono system filesystem)

target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_LINK_DIRECTORIES "${Boost_LIBRARY_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})
include_directories(detail)
include_directories(impl)

####################### installation ###########################

include(CMakePackageConfigHelpers)

set(CONFIG_INSTALL_DIR lib/TraceX/cmake)

set(INCLUDE_INSTALL_DIR include)
set(TARGETS_EXPORT_NAME TraceXTargets)
set(PROJECT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/TraceXConfig.cmake")

configure_package_config_file(
    "cmake/Config.cmake.in"
    "${PROJECT_CONFIG}"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(
    TARGETS
    ${PROJECT_NAME}
    ${PROJECT_NAME}_static
    EXPORT ${TARGETS_EXPORT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
)

install(
    EXPORT ${TARGETS_EXPORT_NAME}
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install(
    FILES ${PROJECT_CONFIG}
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install(FILES ${PUBLIC_HEADERS} DESTINATION  ${INCLUDE_INSTALL_DIR}/trace_x)
install(FILES ${DETAIL_HEADERS} DESTINATION  ${INCLUDE_INSTALL_DIR}/trace_x/detail)
install(FILES ${FMT_HEADERS} detail/fmt/format.cc detail/fmt/ostream.cc detail/fmt/printf.cc DESTINATION  ${INCLUDE_INSTALL_DIR}/trace_x/detail/fmt)

install(FILES ${TOOL_HEADERS} DESTINATION include/trace_x/tools)

if(${WIN32})
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION lib OPTIONAL)
endif()

## install pkgconfig

set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
set(INCLUDE_INSTALL_ROOT_DIR ${CMAKE_INSTALL_PREFIX}/include)

set(PC_FILE ${CMAKE_BINARY_DIR}/TraceX.pc)

configure_file("cmake/TraceX.pc.cmake" ${PC_FILE} @ONLY)

install(FILES ${PC_FILE} DESTINATION lib/TraceX/pkgconfig)

####################### bindings ###########################

option(TRACE_X_WITH_PYTHON "Build with python" TRUE)

find_package(SWIG QUIET)

if(WITH_PYTHON AND SWIG_FOUND)
    message(STATUS "SWIG ${SWIG_VERSION} found")
    message(STATUS "SWIG_EXECUTABLE = ${SWIG_EXECUTABLE}")

    include(${SWIG_USE_FILE})

    set(SWIG_SOURCES impl/trace_swig.i)

    set_source_files_properties(${SWIG_SOURCES} PROPERTIES CPLUSPLUS ON)
    set_source_files_properties(${SWIG_SOURCES} PROPERTIES CMAKE_SWIG_FLAGS "-includeall")

    if(NOT CMAKE_BUILD_TYPE MATCHES Debug)

        ##TODO сборка биндингов на набор питоновых версий
        ##TODO не собирается с первого раза под linux
        ##TODO ruby биндинг

        find_package(Python QUIET)

        message(STATUS "PYTHONLIBS_FOUND = ${PYTHONLIBS_FOUND}")

        if(NOT PYTHONLIBS_FOUND)
            find_package(PythonLibs QUIET)
        endif()

        if(PYTHONLIBS_FOUND)

            message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
            message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
            message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")

            include_directories(${PYTHON_INCLUDE_DIRS})

            set(CMAKE_SWIG_OUTDIR ${LIBRARY_OUTPUT_PATH}/python/2.7)
            set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/python/2.7)

            swig_add_module(trace_x python ${SWIG_SOURCES})
            swig_link_libraries(trace_x ${PROJECT_NAME} ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})

        endif()

        if(CPP_STANDARD_FLAG)
            set_source_files_properties(${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
        endif()

    endif()

else()
    message(STATUS "SWIG NOT FOUND")
endif()