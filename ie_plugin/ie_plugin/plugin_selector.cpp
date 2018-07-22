#include "plugin_selector.h"

using namespace InferenceEngine;
using namespace InferencePlugin;

PluginSelector::PluginSelector(const std::vector<std::string> &pluginDirs) : _dispatcher(pluginDirs) {
}

InferenceEnginePluginPtr PluginSelector::GetPluginByDevice(InferenceEngine::TargetDevice device) {
    return _dispatcher.getSuitablePlugin(device);
}

InferenceEnginePluginPtr PluginSelector::GetPluginByName(const char* pluginName) {
    return _dispatcher.getPluginByName(pluginName);
}
