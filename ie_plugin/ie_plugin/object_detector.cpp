#include "object_detector.h"
#include "plugin_selector.h"

using namespace InferencePlugin;
using namespace InferenceEngine;

ObjectDetector::ObjectDetector(InferenceEnginePluginPtr plugin, const std::string& modelPath, const size_t& batch) :
    _plugin(plugin), _outputThreshold(0.5), _outputTopN(10) {
    _network = NetworkPtr(new Network(_plugin));
    _network->LoadNetwork(modelPath, batch);
}

void ObjectDetector::SetInputParams(ImageFormat format, size_t width, size_t height, size_t stride) {
    std::vector<size_t> inputDims = _network->GetInputDims();

    const size_t inputWidth = inputDims[0];
    const size_t inputHeight = inputDims[1];

    _converter = ImageToInputPtr(new ImageToInput(format, width, height, stride, inputWidth, inputHeight));

    _inputWidth = width;
    _inputHeight = height;
}

void ObjectDetector::GetInputSize(size_t& width, size_t& height) {
    std::vector<size_t> inputDims = _network->GetInputDims();

    width = inputDims[0];
    height = inputDims[1];
}

void ObjectDetector::Infer(const std::vector<uint8_t*>& input, std::vector<DetecionObject>& objects) {
    objects.clear();
    
    Blob::Ptr inputBlobPtr = _network->GetInputMap().begin()->second;
    Blob::Ptr outputBlobPtr = _network->GetOutputMap().begin()->second;

    float* inputPtr = inputBlobPtr->buffer();

    _converter->Convert(input, inputPtr);

    _network->Infer();

    switch (_network->GetNetworkType()) {
        case Unknown:
        case YOLO_V2:
            break;

        case Classification:
            DetectionOutput::ParseClassificationResults(outputBlobPtr, _outputTopN, objects);
            break;

        case SSD:
            DetectionOutput::ParseSSDOutput(outputBlobPtr, _inputWidth, _inputHeight, _outputThreshold, objects);
            break;

        case YOLO_V1:
            DetectionOutput::ParseYoloOutput(outputBlobPtr, _inputWidth, _inputHeight, 20, _outputThreshold, objects);
            DetectionOutput::ApplyNMS(objects, 0.25);
        default:
            break;
    }
}

