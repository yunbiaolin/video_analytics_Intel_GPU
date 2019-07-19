cmake_minimum_required(VERSION 3.4)
project (multichannel_decode)

add_subdirectory(../common common.out)

set(MFX_INCLUDE $ENV{MFX_HOME}/include/mfx)
set(MFX_LIB_OPENSOURCE $ENV{MFX_HOME}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O0 -I${CMAKE_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/rtsp -I${CMAKE_SOURCE_DIR}/rtsp/include/liveMedia -I${CMAKE_SOURCE_DIR}/rtsp/include/groupsock -I${CMAKE_SOURCE_DIR}/rtsp/include/UsageEnvironment -I${CMAKE_SOURCE_DIR}/rtsp/include/BasicUsageEnvironment -I${CMAKE_SOURCE_DIR}/../common -I${CMAKE_SOURCE_DIR}/../runtime/include -I${MFX_INCLUDE}")
set(ENABLE_EXPORTS false)

link_directories( ${MFX_LIB_OPENSOURCE})
link_directories(/usr/lib/x86_64-linux-gnu/)
link_directories(/opt/intel/common/mdf/lib64/)
link_directories(rtsp)
add_executable(multichannel_decode  main.cpp fastcopy.cpp image.cpp image_queue.cpp rtsp/rtspClient.cpp rtsp/rtspWrapper.cpp dualpipe.cpp)
target_link_libraries(multichannel_decode X11 VAITutils mfx igfxcmrt64 va va-drm pthread rt dl libliveMedia.a libBasicUsageEnvironment.a libgroupsock.a libUsageEnvironment.a)


