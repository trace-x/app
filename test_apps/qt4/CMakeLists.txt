project(qt4_test_app)

find_package(Qt4 COMPONENTS QtCore QtGui)

if(NOT Qt4_FOUND)
    return()
endif()

include(${QT_USE_FILE})

qt4_add_resources(resources_wrap app.qrc)

add_executable(${PROJECT_NAME} main.cpp ${resources_wrap})

target_link_libraries(${PROJECT_NAME} trace_x ${QT_LIBRARIES})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  #/GF - remove string literals duplicates
  #/FC - full path for __FILE__ macro
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/GF /FC")
endif()

if(CPP_STANDARD_FLAG)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
