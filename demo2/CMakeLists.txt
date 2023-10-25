cmake_minimum_required(VERSION 3.21)


add_executable(demo2 main.cpp)
target_link_libraries(demo2 PRIVATE sfml-graphics)
target_compile_features(demo2 PRIVATE cxx_std_17)
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET demo2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:demo2> $<TARGET_FILE_DIR:demo2> COMMAND_EXPAND_LISTS)
endif()

#add_custom_target(copy_resources ALL COMMAND ${CMAKE_COMMAND}
#        -E copy_directory
#        "${PROJECT_SOURCE_DIR}/res"
#        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res
#)


install(TARGETS demo2)