#pragma one

#include "network.h"
#include "image_to_input.h"
#include "detection_results.h"

namespace InferencePlugin {
    class ObjectDetector {
    public:
        ObjectDetector(InferenceEngine::InferenceEnginePluginPtr plugin, const std::string& modelPath, const size_t& batch = 1);

    public:
        void SetInputParams(ImageFormat format, size_t width, size_t height, size_t stride);

        void GetInputSize(size_t& width, size_t& height);

        void Infer(const std::vector<uint8_t*>& input, std::vector<DetecionObject>& objects);

        NetworkPtr GetNetwork() {
            return _network;
        }

        void SetOutputThreshold(float val) {
            _outputThreshold = val;
        }

        void SetOutputTopN(size_t val) {
            _outputTopN = val;
        }

    private:
        NetworkPtr _network;
        ImageToInputPtr _converter;

        InferenceEngine::InferenceEnginePluginPtr _plugin;

        std::vector<uint8_t*> _inputData;

        size_t _inputWidth;
        size_t _inputHeight;

        float _outputThreshold;
        size_t _outputTopN;
    };
}
