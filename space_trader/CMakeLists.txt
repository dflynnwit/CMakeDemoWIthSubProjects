cmake_minimum_required(VERSION 3.21)


add_executable(space_trader main.cpp)
target_link_libraries(space_trader PRIVATE sfml-graphics)
target_compile_features(space_trader PRIVATE cxx_std_17)
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET space_trader POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:space_trader> $<TARGET_FILE_DIR:space_trader> COMMAND_EXPAND_LISTS)
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
# Add a POST_BUILD command to copy arial.ttf after building space_trader
add_custom_command(TARGET space_trader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/bytecode.txt"
        $<TARGET_FILE_DIR:space_trader>/bytecode.txt
        COMMENT "Copying arial.ttf to the executable directory"
)


install(TARGETS space_trader)