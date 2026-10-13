cmake_minimum_required(VERSION 3.10)
project(VideoProcessor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(fmt REQUIRED)

pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
pkg_check_modules(SPDLOG REQUIRED spdlog)

add_executable(video_processor
    src/main.cpp
    src/video_processor.cpp
)

target_include_directories(video_processor PRIVATE include ${FFMPEG_INCLUDE_DIRS} ${SPDLOG_INCLUDE_DIRS})
target_link_libraries(video_processor 
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${SPDLOG_LIBRARIES}
    fmt::fmt
)
