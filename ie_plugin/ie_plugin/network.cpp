#include "network.h"

#include "utils.h"

using namespace InferenceEngine;
using namespace InferenceEngine::details;

using namespace InferencePlugin;

Network::Network(InferenceEngine::InferenceEnginePluginPtr plugin) : _plugin(plugin), _networkLoadedToPlugin(false) {
}

Network::~Network() {
}

std::vector<size_t> Network::GetInputDims(const std::string& layerName) {
    if (!IsNetworkLoaded()) {
        THROW_IE_EXCEPTION << "Network not loaded.";
    }

    InferenceEngine::Blob::Ptr data;

    if (layerName.empty()) {
        data = _inputMap.begin()->second;
    }
    else {
        data = _inputMap[layerName];
    }

    return data->dims();
}

std::vector<size_t> Network::GetOutputDims(const std::string& layerName) {
    if (!IsNetworkLoaded()) {
        THROW_IE_EXCEPTION << "Network not loaded.";
    }

    InferenceEngine::Blob::Ptr data;

    if (layerName.empty()) {
        data = _outputMap.begin()->second;
    }
    else {
        data = _outputMap[layerName];
    }

    return data->dims();
}

void Network::AddOutput(const std::string& layerName) {
    if (!IsNetworkLoaded()) {
        THROW_IE_EXCEPTION << "Network not loaded.";
    }

    _network->getNetwork().addOutput(layerName);

    InitOutputs();
}

void Network::LoadNetwork(const std::string& pathXml, const std::string& pathBin, size_t batch) {
    if (IsNetworkLoaded()) {
        THROW_IE_EXCEPTION << "Network already loaded.";
    }

    _network = CNNNetReader::Ptr(new CNNNetReader());

    _network->ReadNetwork(pathXml);

    if (!IsNetworkLoaded()) {
        THROW_IE_EXCEPTION << "Cannot load model " << pathXml;
    }

    _network->ReadWeights(pathBin);

    _network->getNetwork().setBatchSize(batch);

    InitInputs();
    InitOutputs();

    DetectNetworkType();
}

void Network::LoadNetwork(const std::string& pathXml, size_t batch) {
    std::string pathBin = FileNameNoExt(pathXml) + ".bin";

    LoadNetwork(pathXml, pathBin, batch);
}

void Network::Infer() {
    if (!IsNetworkLoaded()) {
        THROW_IE_EXCEPTION << "Network is not loaded.";
    }

    if (!_networkLoadedToPlugin) {
        LoadNetworkToPlugin();
    }

    ResponseDesc dsc;
    StatusCode sts = _plugin->Infer(_inputMap, _outputMap, &dsc);

    if (sts == InferenceEngine::GENERAL_ERROR) {
        THROW_IE_EXCEPTION << "Inference failed! Critical error: " << dsc.msg;
    }
    else if (sts == InferenceEngine::NOT_IMPLEMENTED) {
        THROW_IE_EXCEPTION << "Inference failed! Input data is incorrect and not supported!";
    }
    else if (sts == InferenceEngine::NETWORK_NOT_LOADED) {
        THROW_IE_EXCEPTION << "Inference failed! " << dsc.msg;
    }
}

void Network::DetectNetworkType() {
    _networkType = Unknown;

    BlobMap output = GetOutputMap();

    ICNNNetwork& inetwork = _network->getNetwork();

    CNNLayerPtr out;

    inetwork.getLayerByName(output.begin()->first.c_str(), out, nullptr);

    if (out->type == "Softmax") {
        _networkType = Classification;
    }
    else if (out->type == "DetectionOutput") {
        _networkType = SSD;
    }
    else if (out->type == "Region") {
        _networkType = YOLO_V2;
    }
    else if (out->type == "FullyConnected") {
        _networkType = YOLO_V1;
    }
}

void Network::InitInputs() {
    InputsDataMap inputs;

    inputs = _network->getNetwork().getInputsInfo();

    for (auto it : inputs) {
        InferenceEngine::Blob::Ptr input = MakeSharedBlob(it.second->getInputPrecision(), it.second->getDims());

        input->allocate();

        _inputMap[it.first] = input;
    }
}

void Network::InitOutputs() {
    OutputsDataMap outputs;
    CNNLayerPtr layer;

    SizeVector dims;

    outputs = _network->getNetwork().getOutputsInfo();

    for (auto it : outputs) {
        if (_outputMap.find(it.first) != _outputMap.end()) {
            continue;
        }

        dims = it.second->dims;

        InferenceEngine::Blob::Ptr output = MakeSharedBlob(it.second->precision, dims);

        output->allocate();

        _outputMap[it.first] = output;
    }
}

void Network::LoadNetworkToPlugin() {
    InferenceEngine::ResponseDesc dsc;
    InferenceEngine::StatusCode sts = _plugin->LoadNetwork(_network->getNetwork(), &dsc);

    if (sts == InferenceEngine::GENERAL_ERROR) {
        THROW_IE_EXCEPTION << "Failed to load network to plugin: " << dsc.msg;
    } else if (sts == InferenceEngine::NOT_IMPLEMENTED) {
        THROW_IE_EXCEPTION << "Model cannot be loaded! Plugin doesn't support this model!";
    }

    _networkLoadedToPlugin = true;
}

size_t Network::AccumDims(const std::vector<size_t> &dims) {
    size_t result = 1;

    for (size_t i = 0; i < dims.size(); i++) {
        result *= dims[i];
    }

    return result;
}

Blob::Ptr Network::MakeSharedBlob(Precision precision, SizeVector dims) {
    switch (precision) {
        case Precision::FP16:
        case Precision::FP32:
            return InferenceEngine::make_shared_blob<float, const InferenceEngine::SizeVector>(precision, dims);
        case Precision::Q78:
        case Precision::I16:
            return InferenceEngine::make_shared_blob<short, const InferenceEngine::SizeVector>(precision, dims);
        case Precision::U8:
            return InferenceEngine::make_shared_blob<uint8_t, const InferenceEngine::SizeVector>(precision, dims);
        case Precision::I8:
            return InferenceEngine::make_shared_blob<int8_t, const InferenceEngine::SizeVector>(precision, dims);
        default:
            THROW_IE_EXCEPTION << "Unsupported network precision: " << precision;
    }
}
