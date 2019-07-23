cmake_minimum_required(VERSION 3.0.0)
project(DSS_POC VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

#if(CONAN_LIBCXX STREQUAL "libstdc++11")
#    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
#elseif(CONAN_LIBCXX STREQUAL "libstdc++")
#    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
#endif()

#include(CTest)
#enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
message("aaaaaa: ${CMAKE_SOURCE_DIR}")
message("aaaaaa: ${CMAKE_MODULE_PATH}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)

message("----------- trying to find Jsoncpp -------------")
#find_package(jsoncpp REQUIRED)
#if(JSONCPP_FOUND)
#    get_target_property(JSON_INC_PATH jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)
#    set(JSONCPP_LIBRARIES jsoncpp_lib)
#    message("FOUND jsoncpp CONFIG: ${jsoncpp_CONFIG}")
#    message("INCLUDE jsoncpp DIRS: ${JSON_INC_PATH}")
    include_directories(/usr/local/include)
    set(LIBRARIES ${LIBRARIES} jsoncpp)
#endif()

#message("----------- trying to find boost -------------")
#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)
#find_package(Boost COMPONENTS system thread)
#if (Boost_FOUND)
#    include_directories(${Boost_INCLUDE_DIR})
#    set(LIBRARIES ${LIBRARIES} ${Boost_LIBRARIES})
#endif()

message("----------- trying to find ICE -------------")
find_package(Ice REQUIRED Ice++11)
if (Ice_FOUND)
    include_directories(${Ice_INCLUDE_DIR})
    set(LIBRARIES ${LIBRARIES} ${Ice_LIBRARIES})
endif(Ice_FOUND)

message("----------- trying to find gtest -------------")
find_package(GTest REQUIRED)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
    set(GTEST_LIBRARIES gtest gtest_main)
    message("GTest Libraries: ${GTEST_LIBRARIES}")
endif()

message("----------- trying to find pthread lib -------------")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    set(LIBRARIES ${LIBRARIES} Threads::Threads)
endif(THREADS_HAVE_PTHREAD_ARG)

#message("----------- trying to find opencv -------------")
#find_package(OpenCV REQUIRED )
#if(OpenCV_FOUND)
#    include_directories(${OpenCV_INCLUDE_DIRS})
#    message("OpenCV includes: ${OpenCV_INCLUDE_DIRS}")
#    set(OPENCV_LIBRARIES ${OpenCV_LIBS})
#    message("OpenCV Libraries: ${OpenCV_LIBS}")
#endif()

message("LIBRARIES: ${LIBRARIES}")

set(sources utility.cpp image.cpp scheduler.cpp configuration.cpp image_queue.cpp object.cpp object_queue.cpp feature_queue.cpp ./network/remote_processor.cpp ./network/remote_interface.cpp)
#set(sources image.cpp scheduler.cpp configuration.cpp image_queue.cpp ./network/remote_processor.cpp ./network/remote_processorI.h)
set(test_sources ${sources} configuration_test.cpp main_test.cpp scheduler_test.cpp image_queue_test.cpp object_test.cpp object_queue_test.cpp feature_queue_test.cpp ./network/remote_processor_test.cpp)

# target libary
add_library(libvapoc SHARED ${sources})
target_link_libraries(libvapoc ${LIBRARIES} )

# target exec for test
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/network)

add_executable(test main_test.cpp ${test_sources})
target_link_libraries(test ${LIBRARIES} ${GTEST_LIBRARIES})

add_executable(testserver ${sources} ./network/server_test.cpp) 
target_link_libraries(testserver ${LIBRARIES})

set_target_properties(libvapoc test testserver PROPERTIES COMPILE_DEFINITIONS "ICE_CPP11_MAPPING -Wl,-llibIce++11.so")

add_test(NAME UnitTests COMMAND test)
