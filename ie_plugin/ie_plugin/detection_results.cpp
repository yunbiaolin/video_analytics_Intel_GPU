#include <math.h>

#include "detection_results.h"

using namespace InferenceEngine;
using namespace InferencePlugin;

float DetecionObject::Overlap(float x1, float w1, float x2, float w2) const {
    float l1 = x1 - w1 / 2;
    float l2 = x2 - w2 / 2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1 / 2;
    float r2 = x2 + w2 / 2;
    float right = r1 < r2 ? r1 : r2;

    return right - left;
}

float DetecionObject::Intersection(const DetecionObject& object) const {
    float w = Overlap(x1, Width(), object.x1, object.Width());
    float h = Overlap(y1, Height(), object.y1, object.Height());

    if (w < 0 || h < 0) {
        return 0;
    }

    float area = w * h;

    return area;
}

float DetecionObject::Union(const DetecionObject& object) const {
    float i = Intersection(object);

    float u = Width() * Height() + object.Width() * object.Height() - i;

    return u;
}

float DetecionObject::IoU(const DetecionObject& object) const {
    return Intersection(object) / Union(object);
}

void DetectionOutput::ParseSSDOutput(InferenceEngine::Blob::Ptr output, size_t inputWidth, size_t inputHeight, float threshold, std::vector<DetecionObject>& objects) {
    SizeVector dims = output->dims();

    size_t detectionsCount = dims[1];
    size_t regionValues = dims[0];
    size_t batchSize = dims[3];

    size_t singleDetSize = detectionsCount * regionValues;
    float* data = output->buffer();

    for (size_t mb = 0; mb < batchSize; mb++) {
        ParseSSDOutput(data + singleDetSize * mb, singleDetSize, inputWidth, inputHeight, detectionsCount, regionValues, threshold, objects);
    }
}

void DetectionOutput::ParseSSDOutput(float* data, size_t size, size_t inputWidth, size_t inputHeight,
                                     size_t detectionsCount, size_t regionValues,
                                     float threshold, std::vector<DetecionObject>& objects) {
    float* box = data;

    for (int c = 0; c < detectionsCount; c++, box += regionValues) {
        float confidence = box[2];

        if (confidence < threshold) {
            continue;
        }

        DetecionObject object;

        object.batchId = box[0];
        object.classId = box[1];
        object.confidence = confidence;

        object.x1 = box[3] * inputWidth;
        object.y1 = box[4] * inputHeight;
        object.x2 = box[5] * inputWidth;
        object.y2 = box[6] * inputHeight;

        objects.push_back(object);
    }
}

void DetectionOutput::ParseYoloOutput(InferenceEngine::Blob::Ptr output, size_t inputWidth, size_t inputHeight,
                     size_t numClasses, float threshold, std::vector<DetecionObject>& objects) {
    SizeVector dims = output->dims();

    size_t singleDetSize = 1;
    const size_t batchSize = dims[dims.size() - 1];

    for (size_t i = 0; i < (dims.size() - 1); i++) {
        singleDetSize *= output->dims()[i];
    }

    float* data = output->buffer();

    for (size_t mb = 0; mb < batchSize; mb++) {
        ParseYoloOutput(data + singleDetSize * mb, singleDetSize, inputWidth, inputHeight, numClasses, threshold, objects);
    }
}

void DetectionOutput::ParseYoloOutput(float* data, size_t size, size_t inputWidth, size_t inputHeight,
                                      size_t numClasses, float threshold, std::vector<DetecionObject>& objects) {
    const int boundingBoxes = 2;
    const int cellSize = 7;

    int cells = cellSize * cellSize;

    int probSize = (int)(cells * numClasses);
    int confSize = (int)(cells * boundingBoxes);

    float* box = data;

    for (int c = 0; c < numClasses; c++) {
        float* probs = &box[0];
        float* confs = &box[probSize];
        float* cords = &box[probSize + confSize];

        for (int grid = 0; grid < cells; grid++) {
            int row = grid / cellSize;
            int col = grid % cellSize;

            for (int b = 0; b < boundingBoxes; b++) {
                float conf = confs[(grid * boundingBoxes + b)];
                float prob = probs[grid * numClasses + c] * conf;

                prob *= 3;

                if (prob < threshold) {
                    continue;
                }

                float xc = (cords[(grid * boundingBoxes + b) * 4 + 0] + col) / cellSize;
                float yc = (cords[(grid * boundingBoxes + b) * 4 + 1] + row) / cellSize;
                float w = pow(cords[(grid * boundingBoxes + b) * 4 + 2], 2);
                float h = pow(cords[(grid * boundingBoxes + b) * 4 + 3], 2);

                DetecionObject object;

                object.batchId = 0;
                object.classId = c;
                object.confidence = prob;

                object.x1 = (xc - w / 2) * inputWidth;
                object.y1 = (yc - h / 2) * inputHeight;
                object.x2 = object.x1 + w * inputWidth;
                object.y2 = object.y1 + h * inputHeight;

                objects.push_back(object);
            }
        }
    }

}

void DetectionOutput::ParseClassificationResults(InferenceEngine::Blob::Ptr output,
                                                 size_t topN, std::vector<DetecionObject>& objects) {
    SizeVector outDims = output->dims();

    std::vector<unsigned> results;
    std::vector<float> outResults;

    const TBlob<float>::Ptr fOutput = std::dynamic_pointer_cast<TBlob<float>>(output);
    InferenceEngine::TopResults(topN, *fOutput, results);

    for (int i = 0; i < fOutput->size(); i++) {
        outResults.push_back(fOutput->data()[i]);
    }

    size_t batchSize;

    if (outDims.size() == 2) {
        batchSize = outDims[1];
        topN = std::min<unsigned int>((unsigned)outDims[0], topN);
    } else {
        batchSize = outDims[3];
        topN = std::min<unsigned int>((unsigned)outDims[2], topN);
    }

    for (size_t i = 0; i < batchSize; i++) {
        DetecionObject object;
        object.batchId = i;
        object.x1 = 0;
        object.y1 = 0;
        object.x2 = 0;
        object.y2 = 0;

        for (size_t j = 0; j < topN; j++) {
            object.classId = results[i * topN + j];
            object.confidence = static_cast<float>(outResults[object.classId + i * (outResults.size() / batchSize)]);

            objects.push_back(object);
        }
    }
}

void DetectionOutput::ApplyNMS(std::vector<DetecionObject>& objects, float threshold) {
    for(int i = 0; i < objects.size(); ++i){
        int any = 0;

        any = any || (objects[i].classId > 0);

        if(!any) {
            continue;
        }

        for(int j = i + 1; j < objects.size(); ++j) {
            if (objects[i].IoU(objects[j]) > threshold) {
                if (objects[i].confidence < objects[j].confidence) {
                    objects[i].confidence = 0;
                }
                else {
                    objects[j].confidence = 0;
                }
            }
        }
    }

    objects.erase(std::remove_if(
                           objects.begin(), objects.end(),
                           [](const DetecionObject& obj) {
                               return (obj.confidence == 0);
                           }), objects.end());
}
