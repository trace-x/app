project(c_test_app)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} trace_x)
