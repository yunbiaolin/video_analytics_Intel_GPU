/*
// Copyright (c) 2018 Intel Corporation
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

#pragma once
#include <string>
#include <ie_cnn_net_reader.h>
#include <ie_device.hpp>
#include <ie_plugin_config.hpp>
#include <ie_plugin_dispatcher.hpp>
#include <ie_plugin_ptr.hpp>
#include <inference_engine.hpp>

#include <opencv2/opencv.hpp>
#include "../../include/types.h"
#include "../../include/worker.h"
#include "../../memories/SSD_ROI_memory.h"
#include "../../memories/OCV_RGB_Frame_Memory.h"

using namespace std;
using namespace cv;
using namespace InferenceEngine::details;
using namespace InferenceEngine;

namespace VAITFramework
{


class Inference: public Worker
{
public:
    Inference(float t, OCV_RGB_Frame_Memory *f, SSD_ROI_Memory *ROImemory, std::string modelpth)
    {
       thresh=t;
       frames=f;
       ROImem=ROImemory;
       modelpath=modelpth;
       batchSize=ROImem->GetSize(); //batchsize is configured by ROI memory
    }

    virtual void Init() {init_inference_engine();}
    virtual int Execute();

    ~Inference() {}

    //getters/setters

    void setThresh(float t) {thresh=t;}
    float* get_input_pointer(size_t mb);
    size_t getBatchSize()        {
        return batchSize;
    }
    size_t getInferWidth()       {
        return infer_width;
    }
    size_t getInferHeight()      {
        return infer_height;
    }
    size_t getInferChannelSize() {
        return infer_channel_size;
    }
    size_t getInferChannels()    {
        return infer_channels;
    }
    int    getMPC()              {
        return maxProposalCount;
    }

    int convertFrameToBlob(Mat *frame,int input_number);

private:

    float thresh;
    size_t batchSize;
    OCV_RGB_Frame_Memory *frames;
    SSD_ROI_Memory *ROImem;

    std::string modelpath;



    InferenceEngine::TBlob<float>::Ptr output;
    InferenceEngine::OutputsDataMap out;
    InferenceEngine::BlobMap outputBlobs;

    void init_inference_engine();
    InferenceEngine::InferenceEnginePluginPtr _plugin;
    InferenceEngine::CNNNetReader network;
    InputsDataMap inputs;
    InferenceEngine::BlobMap inputBlobs;

    InferenceEngine::StatusCode sts;
    InferenceEngine::SizeVector inputDims;

    InferenceEngine::TBlob<float>::Ptr pInputBlobData;

    InferenceEngine::ResponseDesc dsc;

    int maxProposalCount = -1;

    //get inference resolution
    size_t infer_channels;
    size_t infer_width;
    size_t infer_height;
    size_t infer_channel_size;
    size_t infer_input_size;


};

}
