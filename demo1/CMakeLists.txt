cmake_minimum_required(VERSION 3.21)


add_executable(demo1 main.cpp)
target_link_libraries(demo1 PRIVATE sfml-graphics)
target_compile_features(demo1 PRIVATE cxx_std_17)
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET demo1 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:demo1> $<TARGET_FILE_DIR:demo1> COMMAND_EXPAND_LISTS)
endif()

#add_custom_target(copy_resources ALL COMMAND ${CMAKE_COMMAND}
#        -E copy_directory
#        "${PROJECT_SOURCE_DIR}/res"
#        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res
#)

## copy arial.ttf to the demo1 executable directory (there's no res dir, just arial.ttf)
#add_custom_command(TARGET demo1 POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        "${PROJECT_SOURCE_DIR}/arial.ttf"
#        $<TARGET_FILE_DIR:demo1>
#)

install(TARGETS demo1)