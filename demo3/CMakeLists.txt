cmake_minimum_required(VERSION 3.21)


add_executable(demo3 main.cpp)
target_link_libraries(demo3 PRIVATE sfml-graphics)
target_compile_features(demo3 PRIVATE cxx_std_17)
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET demo3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:demo3> $<TARGET_FILE_DIR:demo3> COMMAND_EXPAND_LISTS)
endif()

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/arial.ttf
#        ${CMAKE_CURRENT_BINARY_DIR}/arial.ttf
#        COPYONLY)
#add_custom_target(copy_resources ALL COMMAND ${CMAKE_COMMAND}
#        -E copy_directory
#        "${PROJECT_SOURCE_DIR}/res"
#        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res
#)
# Create a custom command that will copy the resource files if needed
# Custom command to copy the font file only if it has changed
# Define the source and destination paths
# Add a POST_BUILD command to copy arial.ttf after building demo3
add_custom_command(TARGET demo3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/arial.ttf"
        $<TARGET_FILE_DIR:demo3>/arial.ttf
        COMMENT "Copying arial.ttf to the executable directory"
)


install(TARGETS demo3)