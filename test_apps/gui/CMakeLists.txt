project(gui_test_app)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

file(GLOB SRC_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h *.ui *.rcc)

file(GLOB UICS_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.ui)
qt5_wrap_ui(uics_wrap ${UICS_LIST})

qt5_add_resources(resources_wrap app.qrc)

add_executable(${PROJECT_NAME} WIN32 ${SRC_LIST} ${uics_wrap} ${resources_wrap})

target_link_libraries(${PROJECT_NAME} lib0 lib1 lib2)

if(CPP_STANDARD_FLAG)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
endif()

## libtrace_x linking

include_directories(${CMAKE_SOURCE_DIR}/traceint/tools)

target_link_libraries(${PROJECT_NAME} trace_x)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  #/GF - remove string literals duplicates
  #/FC - full path for __FILE__ macro
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/GF /FC")
  target_link_libraries(${PROJECT_NAME} Qt5::WinMain)
endif()

## Qt 5 linking

link_qt5()

target_link_libraries(${PROJECT_NAME} Qt5::Widgets)

## Boost linking

find_package(Boost)

include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

add_definitions(-DTRACE_X_WITH_C_FORMATTING)

# CImg TIFF support [optional]

#find_package(TIFF QUIET)

#if(TIFF_FOUND)
#    include_directories(${TIFF_INCLUDE_DIR})
#    target_link_libraries(${PROJECT_NAME} ${TIFF_LIBRARIES})
#    add_definitions(-Dcimg_use_tiff)
#endif()

add_definitions(-Dcimg_display=0)

# OpenCV support [optional]

find_package(OpenCV QUIET)

if(OpenCV_FOUND)

    add_definitions(-DWITH_OPENCV)

    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

endif()

# OpenMP support

find_package(OpenMP)

if(OPENMP_FOUND)

    add_definitions(-DWITH_OPENMP)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

endif()

# unilog support [optional]

find_library(UNILOG_LIB NAMES unilog)

if(UNILOG_LIB)

    add_definitions(-DWITH_UNILOG)

    target_link_libraries(${PROJECT_NAME} ${UNILOG_LIB})

endif()

## Installation

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
