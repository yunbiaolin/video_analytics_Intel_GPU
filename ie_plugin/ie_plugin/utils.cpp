/*
// Copyright (c) 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <algorithm>
#include <fstream>

#if defined(_MSC_VER)
    // N/A
#else
    #include <dirent.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <cmath>

#include "utils.h"

using namespace InferenceEngine;

static TargetDevice GetDeviceFromStr(const std::string& deviceName) {
    static std::map<std::string, InferenceEngine::TargetDevice> deviceFromNameMap = {
        { "CPU", InferenceEngine::TargetDevice::eCPU },
        { "GPU", InferenceEngine::TargetDevice::eGPU }/*,
        { "FPGA", InferenceEngine::TargetDevice::eFPGA }*/
    };

    auto val = deviceFromNameMap.find(deviceName);
    return val != deviceFromNameMap.end() ? val->second : InferenceEngine::TargetDevice::eDefault;
}

std::string GetArchPath() {
#ifndef OS_LIB_FOLDER
    #define OS_LIB_FOLDER "/"
#endif
    std::string archPath = "../../../lib/" OS_LIB_FOLDER "ia32";

    if (sizeof(void*) == 8) {
        archPath = "../../../lib/" OS_LIB_FOLDER "intel64";
    }

    return archPath;
}

InferenceEnginePluginPtr SelectPlugin(const std::vector<std::string> &pluginDirs,
                                      const std::string &plugin, TargetDevice device) {
    InferenceEngine::PluginDispatcher dispatcher(pluginDirs);

    if (!plugin.empty()) {
        return dispatcher.getPluginByName(plugin);
    } 
    else {
        return dispatcher.getSuitablePlugin(device);
    }
}

InferenceEnginePluginPtr SelectPlugin(const std::vector<std::string> &pluginDirs,
                                      const std::string &plugin, const std::string& device) {
    return SelectPlugin(pluginDirs, plugin, GetDeviceFromStr(device));
}

char FilePathSeparator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

std::string FileNameNoExt(const std::string& filePath) {
    auto pos = filePath.rfind('.');

    if (pos == std::string::npos) {
        return filePath;
    }

    return filePath.substr(0, pos);
}

std::string FileNameNoPath(const std::string& filePath) {
    auto pos = filePath.rfind(FilePathSeparator());

    if (pos == std::string::npos) {
        return filePath;
    }

    return filePath.substr(pos + 1);
}

std::string FilePath(const std::string& filePath) {
    auto pos = filePath.rfind(FilePathSeparator());

    if (pos == std::string::npos) {
        return filePath;
    }

    return filePath.substr(0, pos + 1);
}

std::string FileExt(const std::string& filePath) {
    int dot = (int)filePath.find_last_of('.');

    if (dot == -1) {
        return "";
    }
    
    return filePath.substr(dot);
}

std::string GetOSSpecificLibraryName(const std::string& lib) {
#ifdef _WIN32
    return lib + ".dll";
#elif __APPLE__
    return "lib" + lib + ".dylib";
#else
    return "lib" + lib + ".so";
#endif
}
