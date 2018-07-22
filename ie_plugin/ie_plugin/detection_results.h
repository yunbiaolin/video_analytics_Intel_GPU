#pragma once

#include <stddef.h>
#include <vector>
#include <inference_engine.hpp>

namespace InferencePlugin {
    class DetecionObject {
    public:
        DetecionObject() {
        }

    public:
        float Overlap(float x1, float w1, float x2, float w2) const;
        float Intersection(const DetecionObject& object) const;
        float Union(const DetecionObject& object) const;
        float IoU(const DetecionObject& object) const;

        float Width() const {
            return (x2 - x1);
        }

        float Height() const {
            return (y2 - y1);
        }

    public:
        int batchId;
        int classId;

        float confidence;

        float x1;
        float y1;
        float x2;
        float y2;
    };

    class DetectionOutput {
    public:
        static void ParseSSDOutput(InferenceEngine::Blob::Ptr output,
                                   size_t inputWidth, size_t inputHeight,
                                   float threshold, std::vector<DetecionObject>& objects);
        static void ParseSSDOutput(float* data, size_t size,
                                   size_t inputWidth, size_t inputHeight,
                                   size_t detectionsCount, size_t regionValues, float threshold, std::vector<DetecionObject>& objects);

        static void ParseYoloOutput(InferenceEngine::Blob::Ptr output, size_t inputWidth, size_t inputHeight,
                                    size_t numClasses, float threshold, std::vector<DetecionObject>& objects);
        static void ParseYoloOutput(float* data, size_t size, size_t inputWidth, size_t inputHeight,
                                    size_t numClasses, float threshold, std::vector<DetecionObject>& objects);

        static void ParseClassificationResults(InferenceEngine::Blob::Ptr output,
                                        size_t topN, std::vector<DetecionObject>& objects);

        static void ApplyNMS(std::vector<DetecionObject>& objects, float threshold);
    };
}
