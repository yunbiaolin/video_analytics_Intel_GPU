# Copyright (c) 2017 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.4)

## function definitions -------------------------------

function(collect_arch)
  if(__ARCH MATCHES ia32)
    set(ia32 true PARENT_SCOPE)
    set(CMAKE_OSX_ARCHITECTURES i386 PARENT_SCOPE)
  else()
    set(intel64 true PARENT_SCOPE)
    set(CMAKE_OSX_ARCHITECTURES x86_64 PARENT_SCOPE)
  endif()
endfunction()

# ----------------------

function(collect_oses)
  if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    set(Windows    true PARENT_SCOPE)
    set(NotLinux   true PARENT_SCOPE)

  elseif(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    set(Linux      true PARENT_SCOPE)
    set(NotWindows true PARENT_SCOPE)

  endif()
endfunction()

## =======================================
## start

collect_arch()
collect_oses()

if( ia32 )
	message(FATAL_ERROR "This version supports Win64 only !!")
endif()

if(Windows)

  message(STATUS "OS is Windows")

  set(MFX_INCLUDE $ENV{INTELMEDIASDKROOT}include)

  file(GLOB MAIN_SRC common_utils.cpp common_utils_windows.cpp)
  file(GLOB LIBRARY_HEADERS common_utils.h bits/windows_defs.h)

elseif(Linux)

  message(STATUS "OS is Linux")

  set(MFX_INCLUDE $ENV{MFX_HOME}/include)

  file(GLOB MAIN_SRC common_utils.cpp common_utils_linux.cpp common_vaapi.cpp)
  file(GLOB LIBRARY_HEADERS common_utils.h common_vaapi.h bits/linux_defs.h)

else()

  message(FATAL_ERROR "This OS is not supported !")

endif()

set (TARGET_NAME "VAITutils")

# Create named folders for the sources within the .vcproj
# Empty name lists them directly under the .vcproj
#source_group("src" FILES ${LIBRARY_SRC})
#source_group("include" FILES ${LIBRARY_HEADERS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -g -O0 -I\"${MFX_INCLUDE}\"")
include_directories($ENV{DL_SDK_INCLUDE})

# Create library file from sources.
add_library(${TARGET_NAME} STATIC ${MAIN_SRC} ${LIBRARY_HEADERS})
