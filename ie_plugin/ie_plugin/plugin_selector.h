#pragma once

#include <ie_plugin_ptr.hpp>
#include <ie_plugin_dispatcher.hpp>
#include <ie_plugin_config.hpp>

namespace InferencePlugin {
    class PluginSelector {
    public:
        PluginSelector(const std::vector<std::string> &pluginDirs);

    public:
        InferenceEngine::InferenceEnginePluginPtr GetPluginByDevice(InferenceEngine::TargetDevice device);
        InferenceEngine::InferenceEnginePluginPtr GetPluginByName(const char* pluginName);

    private:
        InferenceEngine::PluginDispatcher _dispatcher;
    };
}
