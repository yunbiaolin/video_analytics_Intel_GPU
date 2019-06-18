cmake_minimum_required(VERSION 3.4)
project (multichannel_decode)

add_subdirectory(../common common.out)

set(MFX_INCLUDE $ENV{MFX_HOME}/include/mfx)
set(MFX_LIB_OPENSOURCE $ENV{MFX_HOME}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -O0 -I${CMAKE_SOURCE_DIR}/../common -I${CMAKE_SOURCE_DIR}/../runtime/include -I${MFX_INCLUDE}")
set(ENABLE_EXPORTS false)

message("----------- trying to find Jsoncpp -------------")
find_package(jsoncpp REQUIRED)
if(jsoncpp_FOUND)
    get_target_property(JSON_INC_PATH jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)
    set(JSONCPP_LIBRARIES jsoncpp_lib)
    message("FOUND jsoncpp CONFIG: ${jsoncpp_CONFIG}")
    message("INCLUDE jsoncpp DIRS: ${JSON_INC_PATH}")
    include_directories(${JSON_INC_PATH})
    message("ADD jsoncpp LIB: ${LIBRARIES}")
    set(LIBRARIES ${LIBRARIES} ${JSONCPP_LIBRARIES})
endif()

#include_directories ("${CMAKE_CURRENT_SOURCE_DIR}")
#include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/network/")

link_directories( ${MFX_LIB_OPENSOURCE})
link_directories(/usr/lib/x86_64-linux-gnu/)
link_directories(/opt/intel/common/mdf/lib64/)
add_executable(multichannel_decode  main.cpp fastcopy.cpp utility.cpp scheduler.cpp configuration.cpp image.cpp image_queue.cpp)
target_link_libraries(multichannel_decode X11 VAITutils mfx igfxcmrt64 va va-drm pthread rt dl jsoncpp)


