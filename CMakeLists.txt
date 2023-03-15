cmake_minimum_required(VERSION 3.14)

project(teaio CXX)
set(CMAKE_EXPORT_COMPILE_COMANDS on)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_BUILD_TYPE "Release")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-O3")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-O5")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/O2")
endif()

add_executable(
    ${PROJECT_NAME}
    src/Ayanami.cpp
)

# I've got no idea on how to do this in a smarter way lmao
add_subdirectory(lib)
target_link_libraries(${PROJECT_NAME} PRIVATE ffmpeg)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/avcodec-60.dll ${CMAKE_CURRENT_BINARY_DIR}/avcodec-60.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/avdevice-60.dll ${CMAKE_CURRENT_BINARY_DIR}/avdevice-60.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/avfilter-9.dll ${CMAKE_CURRENT_BINARY_DIR}/avfilter-9.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/avformat-60.dll ${CMAKE_CURRENT_BINARY_DIR}/avformat-60.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/avutil-58.dll ${CMAKE_CURRENT_BINARY_DIR}/avutil-58.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/swresample-4.dll ${CMAKE_CURRENT_BINARY_DIR}/swresample-4.dll
)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/ffmpeg/bin/swscale-7.dll ${CMAKE_CURRENT_BINARY_DIR}/swscale-7.dll
)

add_custom_target(copy_files ALL DEPENDS 
    ${CMAKE_CURRENT_BINARY_DIR}/avcodec-60.dll
    ${CMAKE_CURRENT_BINARY_DIR}/avdevice-60.dll
    ${CMAKE_CURRENT_BINARY_DIR}/avfilter-9.dll
    ${CMAKE_CURRENT_BINARY_DIR}/avformat-60.dll
    ${CMAKE_CURRENT_BINARY_DIR}/avutil-58.dll
    ${CMAKE_CURRENT_BINARY_DIR}/swresample-4.dll
    ${CMAKE_CURRENT_BINARY_DIR}/swscale-7.dll
)