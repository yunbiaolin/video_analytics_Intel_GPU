#pragma once

#include <string>

#include <inference_engine.hpp>
#include <ie_plugin_ptr.hpp>
#include <mkldnn/mkldnn_extension_ptr.hpp>

namespace InferencePlugin {
    using MKLDNNExtensionPtr = std::shared_ptr<InferenceEngine::MKLDNNPlugin::IMKLDNNExtension>;

    class Network;

    using NetworkPtr = std::shared_ptr<Network>;
    using NetworkWeakPtr = std::weak_ptr<Network>;

    using NetworkDataMap = std::map<std::string, InferenceEngine::Blob::Ptr>;

    typedef enum {
        Unknown,
        Classification,
        SSD,
        YOLO_V1,
        YOLO_V2
    }
    NetworkType;
    
    class Network {
    public:
        Network(InferenceEngine::InferenceEnginePluginPtr plugin);
        ~Network();

    public:
        void LoadNetwork(const std::string& pathXml, const std::string& pathBin, size_t batch);
        void LoadNetwork(const std::string& pathXml, size_t batch);

        void AddExtensionToPlugin(MKLDNNExtensionPtr& extension) {
            _plugin->AddExtension(std::dynamic_pointer_cast<InferenceEngine::IExtension>(extension), nullptr);
        }

        void Infer();

        InferenceEngine::BlobMap& GetInputMap() {
            return _inputMap;
        }

        InferenceEngine::BlobMap& GetOutputMap() {
            return _outputMap;
        }

        std::vector<size_t> GetInputDims(const std::string& layerName = "");
        std::vector<size_t> GetOutputDims(const std::string& layerName = "");

        size_t GetInputSize(const std::string& layerName = "") {
            return AccumDims(GetInputDims(layerName));
        }

        size_t GetOutputSize(const std::string& layerName = "") {
            return AccumDims(GetOutputDims(layerName));
        }

        void AddOutput(const std::string& layerName);

        NetworkType GetNetworkType() {
            return _networkType;
        }

    protected:
        void LoadNetworkToPlugin();

        bool IsNetworkLoaded() const { return (_network != nullptr && _network->isParseSuccess()); }

        void InitInputs();
        void InitOutputs();

        size_t AccumDims(const std::vector<size_t>& dims);
        InferenceEngine::Blob::Ptr MakeSharedBlob(InferenceEngine::Precision precision, InferenceEngine::SizeVector dims);

        void DetectNetworkType();

    private:
        InferenceEngine::CNNNetReader::Ptr _network;
        InferenceEngine::InferenceEnginePluginPtr _plugin;
        bool _networkLoadedToPlugin;
        
        InferenceEngine::BlobMap _inputMap;
        InferenceEngine::BlobMap _outputMap;

        NetworkType _networkType;
    };
}
