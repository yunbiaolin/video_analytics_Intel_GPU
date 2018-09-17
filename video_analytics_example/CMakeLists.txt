cmake_minimum_required(VERSION 3.4)
project (video_analytics_example)

set(OPENCV_INCLUDE $ENV{INTEL_CVSDK_DIR}/opencv/include)
set(OPENCV_LIB $ENV{INTEL_CVSDK_DIR}/opencv/lib)
set(IE_PLUGIN_INCLUDE ${CMAKE_SOURCE_DIR}/ie_plugin/ie_plugin)
set(MFX_INCLUDE $ENV{MFX_HOME}/include)
set(MFX_LIB_OPENSOURCE $ENV{MFX_HOME}/lib)
set(MFX_LIB $ENV{MFX_HOME}/lib/lin_x64)
set(CPU_EXENTION_INCLUDE $ENV{INTEL_CVSDK_DIR}/deployment_tools/extension_generator/templates)
set(CPU_EXENTION_LIB ${CMAKE_SOURCE_DIR}/lib)


include(cmake/FindLibYuv.cmake) 
include(cmake/FindXCB.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O0 -I${CMAKE_SOURCE_DIR}/common -I${MFX_INCLUDE} -I${IE_PLUGIN_INCLUDE}  -I${CPU_EXENTION_INCLUDE} -I$ENV{DL_SDK_INCLUDE} -I${OPENCV_INCLUDE} -I../../common")
set(ENABLE_EXPORTS false)
set(CMAKE_EXE_LINKER_FLAGS "-L$ENV{DL_SDK_LIB} -L${CMAKE_SOURCE_DIR}/3rdparty/gflags/lib")

link_directories(../../common)
link_directories(${OPENCV_LIB} ${MFX_LIB_OPENSOURCE} ${MFX_LIB} ${CPU_EXENTION_LIB})
add_executable(video_analytics_example  main.cpp dpipe.cpp XCBShow.cpp detector.cpp)
target_link_libraries(video_analytics_example X11 VAITutils gflags_static mfx va va-drm pthread rt dl opencv_core opencv_video opencv_videoio opencv_imgproc opencv_photo opencv_highgui opencv_imgcodecs inference_engine cpu_extension  ${LIBYUV_LIBRARY} ${SDL_LIBRARY} ${XCB_LIBRARIES})


