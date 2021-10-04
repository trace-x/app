project(app_trace_x)

set(CONFIG config.cmake CACHE STRING "User configuration")

if(EXISTS ${CMAKE_SOURCE_DIR}/${CONFIG})
    message(STATUS "Using config: ${CONFIG}")
    include(${CMAKE_SOURCE_DIR}/${CONFIG})
endif()

add_subdirectory(trace_x)

include_directories(third_party)

if(EXISTS ${CMAKE_BINARY_DIR}/build_info.h)
    add_definitions(-DWITH_BUILD_INFO)
    include_directories(${CMAKE_BINARY_DIR})
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED QUIET)
set(QT Qt${QT_VERSION_MAJOR})

find_package(${QT} COMPONENTS Core Widgets Gui Network PrintSupport)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
    README.md
    about_dialog.cpp
    about_dialog.h
    about_dialog.ui
    app.qrc
    base_item_views.cpp
    base_item_views.h
    callstack_model.cpp
    callstack_model.h
    code_browser.cpp
    code_browser.h
    color_generator.cpp
    color_generator.h
    common_ui_tools.cpp
    common_ui_tools.h
    completer.cpp
    completer.h
    cpp_highlighter.cpp
    cpp_highlighter.h
    data_parser.h
    data_storage.cpp
    data_storage.h
    entry_model.cpp
    entry_model.h
    extra_message_model.cpp
    extra_message_model.h
    filter_item_editor.cpp
    filter_item_editor.h
    filter_item_editor.ui
    filter_model.cpp
    filter_model.h
    filter_tree_view.cpp
    filter_tree_view.h
    general_setting_widget.cpp
    general_setting_widget.h
    general_setting_widget.ui
    issues_list_model.cpp
    issues_list_model.h
    local_connection.cpp
    local_connection.h
    local_connection_controller.cpp
    local_connection_controller.h
    local_connection_p.h
    local_server.cpp
    local_server.h
    local_server_p.cpp
    local_server_p.h
    main.cpp
    main_window.cpp
    main_window.h
    main_window.ui
    panel_container.cpp
    panel_container.h
    panel_container.ui
    panel_layout_settings.cpp
    panel_layout_settings.h
    panel_layout_settings.ui
    panel_manager.cpp
    panel_manager.h
    process_model.cpp
    process_model.h
    profile_model.cpp
    profile_model.h
    res.rc
    session_manager.cpp
    session_manager.h
    session_manager.ui
    settings.cpp
    settings.h
    settings_dialog.cpp
    settings_dialog.h
    settings_dialog.ui
    settings_option.cpp
    settings_option.h
    source_mapping_widget.cpp
    source_mapping_widget.h
    source_mapping_widget.ui
    text_input_dialog.cpp
    text_input_dialog.h
    text_input_dialog.ui
    trace_controller.cpp
    trace_controller.h
    trace_data_model.cpp
    trace_data_model.h
    trace_filter_model.cpp
    trace_filter_model.h
    trace_filter_widget.cpp
    trace_filter_widget.h
    trace_filter_widget.ui
    trace_model.cpp
    trace_model.h
    trace_model_service.cpp
    trace_model_service.h
    trace_server.cpp
    trace_server.h
    trace_service.cpp
    trace_service.h
    trace_table_model.cpp
    trace_table_model.h
    trace_table_view.cpp
    trace_table_view.h
    trace_tools.cpp
    trace_tools.h
    trace_view_widget.cpp
    trace_view_widget.h
    trace_view_widget.ui
    tree_view.cpp
    tree_view.h
    tx_filter_widget.cpp
    tx_filter_widget.h
    tx_filter_widget.ui
    tx_index.h
    tx_model_service.cpp
    tx_model_service.h

    image_lib/abstract_graphics_shape_item.cpp
    image_lib/abstract_graphics_shape_item.h
    image_lib/abstract_simple_item.cpp
    image_lib/abstract_simple_item.h
    image_lib/graphics_image_item.cpp
    image_lib/graphics_image_item.h
    image_lib/graphics_marker_item.cpp
    image_lib/graphics_marker_item.h
    image_lib/graphics_rect_item.cpp
    image_lib/graphics_rect_item.h
    image_lib/image.cpp
    image_lib/image.h
    image_lib/image_browser.cpp
    image_lib/image_browser.h
    image_lib/image_browser.ui
    image_lib/image_scene.cpp
    image_lib/image_scene.h
    image_lib/image_table_model.cpp
    image_lib/image_table_model.h
    image_lib/image_view.cpp
    image_lib/image_view.h
    image_lib/pixel_trace_graphics_item.cpp
    image_lib/pixel_trace_graphics_item.h
)

# link boost directory with libraries

find_package(Boost ${BOOST_VERSION})
link_directories(${BOOST_LIBRARYDIR})

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${uics_wrap} ${resources_wrap} res.rc app.qss)

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
set_target_properties(${PROJECT_NAME} PROPERTIES AUTORCC ON)
set_target_properties(${PROJECT_NAME} PROPERTIES AUTOUIC ON)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME app_trace_x)

target_compile_definitions(${PROJECT_NAME} PRIVATE TRACE_X_MODULE_NAME="${PROJECT_NAME}")

if(CPP_STANDARD_FLAG)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CPP_STANDARD_FLAG})
endif()

## libtrace_x linking

target_link_libraries(${PROJECT_NAME} trace_x)

if(${UNIX})
    target_link_libraries(${PROJECT_NAME} dl rt pthread)
endif()

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
  target_link_libraries(${PROJECT_NAME} ${QT}::WinMain)
endif()

## Qt linking

target_link_libraries(${PROJECT_NAME} ${QT}::Widgets ${QT}::Network ${QT}::Gui ${QT}::PrintSupport)

## Boost linking

set(Boost_USE_STATIC_LIBS ${LINK_STATIC_BOOST_TO_EXEC})
find_package(Boost ${BOOST_VERSION} COMPONENTS chrono system filesystem thread atomic)
add_definitions(-DBOOST_DATE_TIME_NO_LIB)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

## OpenMP linking

#find_package(OpenMP)

#if (OPENMP_FOUND)
#    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#endif()

# CImg support

find_package(TIFF QUIET)

if(TIFF_FOUND)
    include_directories(${TIFF_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${TIFF_LIBRARIES})
    add_definitions(-Dcimg_use_tiff)
endif()

add_definitions(-Dcimg_display=0)

## Installation

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
