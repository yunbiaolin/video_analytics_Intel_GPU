cmake_minimum_required(VERSION 3.4)
project (VAIT)

set(MSDK CACHE STRING "")
set(BUILD_TARGET CACHE STRING "")

if(BUILD_TARGET STREQUAL "BXT")
    message(STATUS "[INFO] Target platform is BXT (APL)")
    set(COMPILE_OPTION "-D${BUILD_TARGET}")
endif()

add_subdirectory(3rdparty/gflags)
add_subdirectory(ie_plugin)

add_subdirectory(common)
add_subdirectory(video_analytics_example)

if(DEFINED ENV{TBBROOT})
    add_subdirectory(vait_framework)
else()
    message(STATUS "[INFO] TBB env is not set, so skip vait_framework samples build for now")
endif()
