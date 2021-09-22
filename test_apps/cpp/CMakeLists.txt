project(cpp_test_app)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} trace_x)

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

add_definitions(-DTRACE_X_WITH_C_FORMATTING)

## Installation

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
