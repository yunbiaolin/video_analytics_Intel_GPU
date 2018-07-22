
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

#include "inference.h"
using namespace VAITFramework;



void Inference::init_inference_engine()
{
    //----------------------------------------------------------------------------
    // inference engine initialization
    //----------------------------------------------------------------------------
    _plugin=InferenceEngine::InferenceEnginePluginPtr("libMKLDNNPlugin.so");

    // ----------------
    // Read network
    // ----------------
    //InferenceEngine::CNNNetReader network;
    std::string modelfile(modelpath);

    network.ReadNetwork(modelfile);


    std::cout << "Network loaded." << std::endl;
    auto pos=modelfile.rfind('.');

    std::string binFileName=modelfile.substr(0,pos)+".bin";
    network.ReadWeights(binFileName.c_str());

    // ---------------
    // set the target device
    // ---------------
    network.getNetwork().setTargetDevice(TargetDevice::eCPU);

    // --------------------
    // Set batch size
    // --------------------
    network.getNetwork().setBatchSize(batchSize);
    batchSize = network.getNetwork().getBatchSize();

    //----------------------------------------------------------------------------
    //  Inference engine input setup
    //----------------------------------------------------------------------------

    // ---------------
    // set input configuration
    // ---------------
    inputs = network.getNetwork().getInputsInfo();

    inputDims = inputs.begin()->second->getDims();

    if (inputDims.size() != 4) {
        std::cout << "Not supported input dimensions size, expected 4, got "
                  << inputDims.size() << std::endl;
    }

    std::string imageInputName = inputs.begin()->first;
    printf("name of image input layer=%s\n\n",imageInputName.c_str());
    inputs[imageInputName]->setInputPrecision(FP32);

    // --------------------
    // Allocate input blobs
    // --------------------
    //InferenceEngine::BlobMap inputBlobs;
    pInputBlobData =
        InferenceEngine::make_shared_blob < float,
        const InferenceEngine::SizeVector >(FP32, inputDims);
    pInputBlobData->allocate();

    inputBlobs[imageInputName] = pInputBlobData;

    // --------------------------------------------------------------------------
    // Load model into plugin
    // --------------------------------------------------------------------------
    sts = _plugin->LoadNetwork(network.getNetwork(), &dsc);

    //----------------------------------------------------------------------------
    //  Inference engine output setup
    //----------------------------------------------------------------------------

    // --------------------
    // get output dimensions
    // --------------------

    out = network.getNetwork().getOutputsInfo();

    std::string outputName = out.begin()->first;
    printf("output layer name=%s\n",outputName.c_str());


    for (auto && item : out) {
        InferenceEngine::SizeVector outputDims = item.second->dims;

        output =
            InferenceEngine::make_shared_blob < float,
            const InferenceEngine::SizeVector >(InferenceEngine::FP32,
                                                outputDims);
        output->allocate();

        outputBlobs[item.first] = output;
        maxProposalCount = outputDims[1];

        std::cout << "maxProposalCount = " << maxProposalCount << std::endl;
    }

    //get inference resolution
    infer_channels = inputDims[2];  // channels for color format.  RGB=4
    infer_width    = inputDims[1];
    infer_height   = inputDims[0];
    infer_channel_size   = infer_width * infer_height;
    infer_input_size     = infer_channel_size * infer_channels;
}



float* Inference::get_input_pointer(size_t mb)
{
    return pInputBlobData.get()->data() + infer_input_size * mb;
}

int Inference::convertFrameToBlob(Mat *frame,int input_number)
{
    Mat frameInfer;
    resize(*frame, frameInfer, Size(infer_width, infer_height));

    //reorder
    float* inputPtr = get_input_pointer(input_number);

    for (size_t i = 0, imgIdx = 0, idx = 0; i < infer_channel_size; i++, idx++) {
        for (size_t ch = 0; ch < infer_channels; ch++, imgIdx++) {
            inputPtr[idx + ch * infer_channel_size] = frameInfer.data[imgIdx];
        }
    }

}


int Inference::Execute()
{
    //convert frame memory data to planar RGB
    for (size_t mb = 0; mb < batchSize; mb++) {
       convertFrameToBlob(frames->GetFramePtr(mb),mb);
    }

    // RUN INFERENCE
    sts = _plugin->Infer(inputBlobs, outputBlobs, &dsc);
    if (sts != 0) {
        std::cout << "An infer error occurred: " << dsc.msg << std::endl;
        return 1;
    }

    // parse inference output
    std::string outputName = out.begin()->first;
    InferenceEngine::Blob::Ptr detectionOutBlob = outputBlobs[outputName];
    const InferenceEngine::TBlob < float >::Ptr detectionOutArray =
        std::dynamic_pointer_cast <InferenceEngine::TBlob <
        float >>(detectionOutBlob);

    size_t outputSize = outputBlobs.cbegin()->second->size() / batchSize;


    for (size_t mb = 0; mb < batchSize; mb++) {
        //remove detections found previously
        ROImem->ClearDetections(mb);

        //---------------------------
        // parse SSD output
        //---------------------------

        //work with outputs for this batch
        float *box = detectionOutArray->data() + outputSize * mb;

        //loop through proposals
        //if a region's confidence is >= threshold, store the detection
        for (int c = 0; c < maxProposalCount; c++) {
            float *localbox=&box[c * 7];
            float confidence = localbox[2];

            if (confidence >= thresh)  {
              SSD_ROI_type ROItmp;
              ROItmp.labelnum=localbox[1];
              ROItmp.confidence=confidence;
              ROItmp.xmin=localbox[3];
              ROItmp.ymin=localbox[4];
              ROItmp.xmax=localbox[5];
              ROItmp.ymax=localbox[6];

              ROImem->StoreDetection(ROItmp,mb);
            }
        }
    }

    return 0;
}
