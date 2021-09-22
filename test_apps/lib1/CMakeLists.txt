project(lib1)

add_definitions(-DLIB_API_EXPORT)

add_library(${PROJECT_NAME} SHARED lib1.h lib1.cpp)

target_link_libraries(${PROJECT_NAME} trace_x lib0)

if(CPP_STANDARD_FLAG)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
endif()

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION bin)
