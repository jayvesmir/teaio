cmake_minimum_required(VERSION 3.14)

project(ffmpeg)

add_library(${PROJECT_NAME} INTERFACE IMPORTED GLOBAL)
target_include_directories(${PROJECT_NAME} INTERFACE ffmpeg/include)
target_link_directories(${PROJECT_NAME} INTERFACE ffmpeg/lib)
target_link_libraries(${PROJECT_NAME} INTERFACE
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    swresample
    swscale
)