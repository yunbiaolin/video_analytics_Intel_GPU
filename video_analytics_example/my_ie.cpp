/*
// Copyright (c) 2017 Intel Corporation
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

#include "my_ie.h"

#define DEFAULT_PATH_P "./lib"
#define NUMLABELS 20

#ifndef OS_LIB_FOLDER
#define OS_LIB_FOLDER "/"
#endif

CMyIE::CMyIE()
{
    m_BatchSize = 0;
    maxProposalCount = -1;
}


CMyIE::CMyIE(vector<std::string> PluginDirs, string Plugin, string Device,string Model, int batchSize, bool pcflag)
{
   //TODO: check return values 
   LoadPlugIn(PluginDirs, Plugin, Device);


// Enable performance counters
    if (pcflag) {
        EnablePerformanceCounters();
    }

// Read model (network, weights)
    ReadModel(Model);
    //todo: check return
    
// Set the target device
    SetTargetDevice(Device);
    
// Set batch size
    SetBatchSize(batchSize);


// Load Network to plugin
    LoadNetwork();
    
 m_pIEPlugIn->SetConfig({ { PluginConfigParams::KEY_SINGLE_THREAD, PluginConfigParams::YES } }, nullptr);
 m_pIEPlugIn->SetConfig({ { PluginConfigParams::KEY_CPU_BIND_THREAD, PluginConfigParams::YES } }, nullptr);

}

CMyIE::~CMyIE()
{

}

float CMyIE::overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1 / 2;
    float l2 = x2 - w2 / 2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1 / 2;
    float r2 = x2 + w2 / 2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

float CMyIE::boxIntersection(DetectedObject a, DetectedObject b)
{
    float w = overlap(a.xmin, (a.xmax - a.xmin), b.xmin, (b.xmax - b.xmin));
    float h = overlap(a.ymin, (a.ymax - a.ymin), b.ymin, (b.ymax - b.ymin));

    if (w < 0 || h < 0) {
        return 0;
    }

    float area = w * h;

    return area;
}

float CMyIE::boxUnion(DetectedObject a, DetectedObject b)
{
    float i = boxIntersection(a, b);
    float u = (a.xmax - a.xmin) * (a.ymax - a.ymin) + (b.xmax - b.xmin) * (b.ymax - b.ymin) - i;

    return u;
}

float CMyIE::boxIoU(DetectedObject a, DetectedObject b)
{
    return boxIntersection(a, b) / boxUnion(a, b);
}

void CMyIE::doNMS(std::vector<DetectedObject>& objects, float thresh)
{
    int i, j;
    for (i = 0; i < objects.size(); ++i) {
    int any = 0;

    any = any || (objects[i].objectType > 0);

    if (!any) {
        continue;
    }

    for (j = i + 1; j < objects.size(); ++j) {
        if (boxIoU(objects[i], objects[j]) > thresh) {
            if (objects[i].prob < objects[j].prob) {
                objects[i].prob = 0;
            }
            else {
                objects[j].prob = 0;
            }
        }
    }
    }
}

/**
* \brief This function analyses the YOLO net output for a single class
* @param net_out - The output data
* @param class_num - The class number
* @return a list of found boxes
*/
std::vector < DetectedObject > CMyIE::yoloNetParseOutput(float *net_out, int class_num,	int modelWidth,	int modelHeight, float threshold)
{
    int C = 20;         // classes
    int B = 2;          // bounding boxes
    int S = 7;          // cell size

    std::vector < DetectedObject > boxes;
    std::vector < DetectedObject > boxes_result;
    int SS = S * S;     // number of grid cells 7*7 = 49
                        // First 980 values correspons to probabilities for each of the 20 classes for each grid cell.
                        // These probabilities are conditioned on objects being present in each grid cell.
    int prob_size = SS * C; // class probabilities 49 * 20 = 980
                            // The next 98 values are confidence scores for 2 bounding boxes predicted by each grid cells.
    int conf_size = SS * B; // 49*2 = 98 confidences for each grid cell

    float *probs = &net_out[0];
    float *confs = &net_out[prob_size];
    float *cords = &net_out[prob_size + conf_size]; // 98*4 = 392 coords x, y, w, h

    for (int grid = 0; grid < SS; grid++)
    {
        int row = grid / S;
        int col = grid % S;
        for (int b = 0; b < B; b++)
        {
            float conf = confs[(grid * B + b)];
            float prob = probs[grid * C + class_num] * conf;
            prob *= 3;  //TODO: probabilty is too low... check.

            if (prob < threshold) continue;

            float xc = (cords[(grid * B + b) * 4 + 0] + col) / S;
            float yc = (cords[(grid * B + b) * 4 + 1] + row) / S;
            float w = pow(cords[(grid * B + b) * 4 + 2], 2);
            float h = pow(cords[(grid * B + b) * 4 + 3], 2);

            DetectedObject bx(class_num,
                (xc - w / 2)*modelWidth,
                (yc - h / 2)*modelHeight,
                (xc + w / 2)*modelWidth,
                (yc + h / 2)*modelHeight,
                prob);

            boxes_result.push_back(bx);
        }
    }

    return boxes_result;
}

/**
* \brief This function prints performance counters
* @param perfomanceMap - map of rerformance counters
*/
void CMyIE::printPerformanceCounters(const std::map<std::string, InferenceEngine::InferenceEngineProfileInfo>& perfomanceMap)
{
    long long totalTime = 0;

    std::cout << std::endl << "> Perfomance counts:" << std::endl << std::endl;

    std::map<std::string, InferenceEngine::InferenceEngineProfileInfo>::const_iterator it;

    for (it = perfomanceMap.begin(); it != perfomanceMap.end(); ++it) {
        std::cout << std::setw(30) << std::left << it->first + ":";
        switch (it->second.status) {
        case InferenceEngine::InferenceEngineProfileInfo::EXECUTED:
            std::cout << std::setw(15) << std::left << "EXECUTED";
            break;
        case InferenceEngine::InferenceEngineProfileInfo::NOT_RUN:
            std::cout << std::setw(15) << std::left << "NOT_RUN";
            break;
        case InferenceEngine::InferenceEngineProfileInfo::OPTIMIZED_OUT:
            std::cout << std::setw(15) << std::left << "OPTIMIZED_OUT";
            break;
        }

        std::cout << std::setw(20) << std::left << "realTime: " + std::to_string(it->second.realTime_uSec);
        std::cout << std::setw(20) << std::left << " cpu: " + std::to_string(it->second.cpu_uSec);
        //std::cout << " type: " << it->second.type << std::endl;

        if (it->second.realTime_uSec > 0) {
            totalTime += it->second.realTime_uSec;
        }
    }

    std::cout << std::setw(20) << std::left << "Total time: " + std::to_string(totalTime) << " microseconds" << std::endl;
}

void CMyIE::LoadPlugIn(std::vector<std::string> PluginDirs, std::string Plugin, std::string Device)
{
    m_pIEPlugIn = InferenceEngine::InferenceEnginePluginPtr(selectPlugin(PluginDirs, Plugin, Device));
   
    const PluginVersion *pluginVersion;
    m_pIEPlugIn->GetVersion((const InferenceEngine::Version*&)pluginVersion);
    std::cout << "\t. Plugin Info:";
    std::cout << pluginVersion << std::endl;

    return;
}

void CMyIE::EnablePerformanceCounters(void)
{
    m_pIEPlugIn->SetConfig({ { PluginConfigParams::KEY_PERF_COUNT, PluginConfigParams::YES } }, nullptr);
    return;
}

bool CMyIE::ReadModel(std::string Model)
{
    // ----------------
    // Read network
    // ----------------	
    try {
    	m_Network.ReadNetwork(Model);
    }
    catch (InferenceEngineException ex) {
        std::cerr << "\t\t. Failed to load network: " << ex.what() << std::endl;
        return false;
    }

    // ----------------
    // Read weights
    // ----------------

    std::string binFileName = fileNameNoExt(Model) + ".bin";

    try {
        m_Network.ReadWeights(binFileName.c_str());
    }
    catch (InferenceEngineException ex) {
        std::cerr << "\t\t. Failed to load weights: " << ex.what() << std::endl;
        return false;
    }

    std::cout << "\t. Done reading network/ weights."<<"Model:"<<Model<<"  binFileName:"<<binFileName << std::endl;

    return true;
}

void CMyIE::SetTargetDevice(std::string Device)
{
    m_Network.getNetwork().setTargetDevice(getDeviceFromStr(Device));
    return;
}

void CMyIE::SetBatchSize(size_t batchsize)
{
    m_Network.getNetwork().setBatchSize(batchsize);
    m_BatchSize = batchsize;
    return;
}

size_t CMyIE::GetBatchSize(void)
{
    return m_Network.getNetwork().getBatchSize();
}

bool CMyIE::IOBlobsSetting(int *w_o, int *h_o)
{
    // ---------------
    // set input configuration
    // ---------------

    inputs = m_Network.getNetwork().getInputsInfo();

    if (inputs.size() != 1) {
        std::cout << "\t. This sample accepts networks having only one input." << std::endl;
        return 1;
    }

    inputDims = inputs.begin()->second->getDims();

    if (inputDims.size() != 4) {
        std::cout << "\t. Not supported input dimensions size, expected 4, got "
            << inputDims.size() << std::endl;
    }

    inputName = inputs.begin()->first;

    // look for the input == imageData
    DataPtr image = inputs[inputName]->getInputData();

    inputs[inputName]->setInputPrecision(Precision::FP32);

    m_InputChannels     = inputDims[2];  // channels for color format.  RGB = 4
    size_t input_height = inputDims[1];
    size_t input_width  = inputDims[0];

    m_ChannelSize = input_width * input_height;

    m_InputSize = m_ChannelSize * m_InputChannels;
    std::cout << "\t. m_InputChannels: " << m_InputChannels << std::endl;
    std::cout << "\t. input_width: " << input_width << std::endl;
    std::cout << "\t. input_height: " << input_height << std::endl;
    std::cout << "\t. Input size: " << m_InputSize << std::endl;

    // --------------------
    // Allocate input blobs
    // --------------------
    input = InferenceEngine::make_shared_blob < float, const InferenceEngine::SizeVector >(Precision::FP32, inputDims);

    input->allocate();

    inputBlobs[inputName] = input;

    //uncomment below for video output
    *w_o = frameWidth = inputDims[1];	// output width
    *h_o = frameHeight = inputDims[0];	// output height

    // --------------------
    // get output dimensions
    // --------------------
    InferenceEngine::OutputsDataMap out;
    out = m_Network.getNetwork().getOutputsInfo();

    outputName = out.begin()->first;
    
    for (auto && item : out) {
        InferenceEngine::SizeVector outputDims = item.second->dims;
        InferenceEngine::TBlob<float>::Ptr output;
        output = InferenceEngine::make_shared_blob < float,	const InferenceEngine::SizeVector >(Precision::FP32,	outputDims);
        output->allocate();

        outputBlobs[item.first] = output;
        maxProposalCount     = outputDims[1];
        size_t output_width  = outputDims[1];
        size_t output_height = outputDims[0];
        std::cout << "\t. output_width: " << output_width << " output_height"<<output_height << std::endl;
        std::cout << "\t. maxProposalCount = " << maxProposalCount << std::endl;
    }

    outputDims   = outputBlobs.cbegin()->second->dims();
    m_OutputSize = outputBlobs.cbegin()->second->size() / m_BatchSize;

    std::cout << "\t. Output size: " << m_OutputSize << std::endl;


    return true;
}

bool CMyIE::LoadNetwork(void)
{
    // --------------------------------------------------------------------------
    // Load model into plugin
    // --------------------------------------------------------------------------
    std::cout << "\t. Loading model to plugin..." << std::endl;

    InferenceEngine::ResponseDesc dsc;

    InferenceEngine::StatusCode sts = m_pIEPlugIn->LoadNetwork(m_Network.getNetwork(), &dsc);
    if (sts != 0) {
        std::cout << "\t\t. Error loading model into plugin: " << dsc.msg << std::endl;
        return false;
    }

    return true;
}

void CMyIE::PreAllocateInputBuffer(size_t cnt)
{
    m_InputPtr = input.get()->data() + m_InputSize * cnt;
}

float* CMyIE::getInputPtr(size_t cnt)
{
    return input.get()->data() + m_InputSize * cnt;
}


void CMyIE::PreAllocateBoxBuffer(size_t cnt)
{
    detectionOutBlob = outputBlobs[outputName];
    detectionOutArray = std::dynamic_pointer_cast <InferenceEngine::TBlob <float>>(detectionOutBlob);

    m_BoxPtr = detectionOutArray->data() + m_OutputSize * cnt;
}

int CMyIE::GetInputSize(void)
{
    return m_InputSize;
}
void CMyIE::AllocteInputData2(unsigned char *pRGBPData, float normalize_factor)
{
    // imgIdx: image pixel counter
    // m_ChannelSize: size of a channel, computed as image size in bytes divided by number of channels, or image width * image height
    // m_InputPtr: a pointer to pre-allocated inout buffer

    chrono::high_resolution_clock::time_point time11,time21;
    std::chrono::duration<double> diff1;
    time11 = std::chrono::high_resolution_clock::now();

    for (size_t ch = 0; ch < m_InputChannels; ch++ ) {
        for (size_t idx = 0; idx < m_ChannelSize;  idx++) {
             m_InputPtr[idx + ch * m_ChannelSize] = pRGBPData[idx + ch*m_ChannelSize] / normalize_factor;
         }
    }
    time21 = std::chrono::high_resolution_clock::now();
    diff1 = time21-time11;	
    std::cout<<"\t PrepareData2 takes:"<< diff1.count()*1000.0 << "ms/frame\t"<<std::endl;	
}

void CMyIE::AllocteInputData3(unsigned char *B, unsigned char *G, unsigned char *R, unsigned int pitch, unsigned int width, unsigned int height,  float normalize_factor)
{
    // imgIdx: image pixel counter
    // m_ChannelSize: size of a channel, computed as image size in bytes divided by number of channels, or image width * image height
    // m_InputPtr: a pointer to pre-allocated inout buffer
    chrono::high_resolution_clock::time_point time11,time21;
    std::chrono::duration<double> diff1;
    time11 = std::chrono::high_resolution_clock::now();

    for (int w = 0; w < width;  w++) {
        for (int h = 0; h < height;  h++) {
            m_InputPtr[w+h*width ] = B[w+h*pitch] / normalize_factor;
            m_InputPtr[w+h*width + width * height] = G[w+h*pitch] / normalize_factor;
            m_InputPtr[w+h*width + 2 * width * height] = R[w+h*pitch] / normalize_factor;
        }
    }
    time21 = std::chrono::high_resolution_clock::now();
    diff1 = time21-time11;		  
     
    std::cout<<"\t PrepareData3 takes:"<< diff1.count()*1000.0 << "ms/frame\t"<<std::endl;

}

void CMyIE::AllocteInputData(cv::Mat resized, float normalize_factor)
{
    // imgIdx: image pixel counter
    // m_ChannelSize: size of a channel, computed as image size in bytes divided by number of channels, or image width * image height
    // m_InputPtr: a pointer to pre-allocated inout buffer

    printf("channelsize=%d inputchannels=%d\n",m_ChannelSize,m_InputChannels);
    for (size_t i = 0, imgIdx = 0, idx = 0; i < m_ChannelSize; i++, idx++) {
        for (size_t ch = 0; ch < m_InputChannels; ch++, imgIdx++) {
            m_InputPtr[idx + ch * m_ChannelSize] = resized.data[imgIdx] / normalize_factor;
        }
    }
}

bool CMyIE::Infer(void)
{
	InferenceEngine::ResponseDesc dsc;
    InferenceEngine::StatusCode sts;

    sts = m_pIEPlugIn->Infer(inputBlobs, outputBlobs, &dsc);

    if (sts != 0)
    {
        std::cout << "\t\t .Error loading model into plugin: " << dsc.msg << std::endl;
        return false;
    }

    return true;
}

void CMyIE::ReadPerformanceCounters(void)
{
    std::map<std::string, InferenceEngine::InferenceEngineProfileInfo> perfomanceMap;
    m_pIEPlugIn->GetPerformanceCounts(perfomanceMap, nullptr);

    printPerformanceCounters(perfomanceMap);
    return;
}

void CMyIE::ReadyForOutputProcessing(void)
{
    //---------------------------
    // POSTPROCESS STAGE:
    // parse output
    // Output layout depends on network topology
    // so there are different paths for YOLO and SSD
    //---------------------------
    detectionOutBlob = outputBlobs[outputName];
    detectionOutArray = std::dynamic_pointer_cast <InferenceEngine::TBlob <float>>(detectionOutBlob);

    return;
}

std::vector<BB_INFO> CMyIE::DetectValidBoundingBox(std::string anyTopology, float threshold, int labelIndex)
{
    std::vector<BB_INFO> rcBB_result;

    if (anyTopology.compare("SSD") == 0)
    {
        for (int c = 0; c < maxProposalCount; c++)
        {
            float image_id = m_BoxPtr[c * 7 + 0];
            float label = m_BoxPtr[c * 7 + 1];
            float confidence = m_BoxPtr[c * 7 + 2];
            float xmin = m_BoxPtr[c * 7 + 3] * inputDims[0];
            float ymin = m_BoxPtr[c * 7 + 4] * inputDims[1];
            float xmax = m_BoxPtr[c * 7 + 5] * inputDims[0];
            float ymax = m_BoxPtr[c * 7 + 6] * inputDims[1];

            if (confidence > threshold) 
            {
                //std::cout << "> " << c << " label=" << label << " confidence " << confidence << " (" << xmin << ", " << ymin
                //<< ") - (" << xmax << ", " << ymax << ")" << std::endl;

                BB_INFO rcBB;
                rcBB.xmin = xmin;
                rcBB.ymin = ymin;
                rcBB.xmax = xmax;
                rcBB.ymax = ymax;
                rcBB.prob = 0.5;	// not use

                rcBB_result.push_back(rcBB);
            }
        }
    }
    else if (anyTopology.compare("YOLO-tiny") == 0)
    {
        std::vector < DetectedObject > result = yoloNetParseOutput(m_BoxPtr, labelIndex, frameWidth, frameHeight, threshold);

        if (result.size() > 0)
        {
            doNMS(result, .45);

            for (int i = 0; i < result.size(); i++)
            {
                BB_INFO rcBB;

                rcBB.xmin = result[i].xmin;
                rcBB.ymin = result[i].ymin;
                rcBB.xmax = result[i].xmax;
                rcBB.ymax = result[i].ymax;
                rcBB.prob = result[i].prob;

                rcBB_result.push_back(rcBB);
            }
        }
    }

    return rcBB_result;
}

