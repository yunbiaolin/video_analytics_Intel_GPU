#pragma once

#include "common.hpp"
#include <ie_plugin_ptr.hpp>
#include <ie_plugin_config.hpp>
//#include <ie_cnn_net_reader.h>
#include <inference_engine.hpp>
#include <ie_input_info.hpp>

#include <opencv2/opencv.hpp>

using namespace InferenceEngine::details;
using namespace InferenceEngine;
using namespace std;

typedef struct tagBB_INFO
{
	float xmin;
	float ymin;
	float xmax;
	float ymax;
	float prob;

} BB_INFO;

class CMyIE
{
public:
	CMyIE();
        CMyIE(vector<std::string> PluginDirs, string Plugin, string Device,string Model, int batchSize, bool pcflag);
	virtual ~CMyIE();
	void LoadPlugIn(std::vector<std::string> PluginDirs, std::string Plugin, std::string Device);
	void EnablePerformanceCounters(void);
	void ReadPerformanceCounters(void);
	bool ReadModel(std::string Model);
	void SetTargetDevice(std::string Device);
	bool LoadNetwork(void);
	void SetBatchSize(size_t batchsize);
	size_t GetBatchSize(void);
	bool IOBlobsSetting(int *w_o, int *h_o);
	void PreAllocateInputBuffer(size_t cnt);
        float *getInputPtr(size_t cnt);
	void PreAllocateBoxBuffer(size_t cnt);
	int GetInputSize(void);
	void AllocteInputData(cv::Mat resized, float normalize_factor);	
	void AllocteInputData2(unsigned char *pRGBP, float normalize_factor);	
	void AllocteInputData3(unsigned char *B, unsigned char *G, unsigned char *R, unsigned int pitch, unsigned int width, unsigned int height,  float normalize_factor);
	
	bool Infer(void);
	void ReadyForOutputProcessing(void);
	std::vector<BB_INFO> DetectValidBoundingBox(std::string anyTopology, float threshold, int labelIndex = 0);

private:
	InferenceEngine::InferenceEnginePluginPtr m_pIEPlugIn;
	InferenceEngine::CNNNetReader m_Network;
	size_t m_BatchSize;
	InputsDataMap inputs;
	InferenceEngine::SizeVector inputDims;
	InferenceEngine::SizeVector outputDims;
	InferenceEngine::BlobMap inputBlobs;
	InferenceEngine::BlobMap outputBlobs;
	InferenceEngine::TBlob<float>::Ptr input;
	std::string inputName;
	std::string outputName;
	InferenceEngine::Blob::Ptr detectionOutBlob;
	InferenceEngine::TBlob < float >::Ptr detectionOutArray;
	float* m_InputPtr;
	float* m_BoxPtr;

	int maxProposalCount;
	int m_InputChannels;
	int m_ChannelSize;
	int m_InputSize;
	size_t m_OutputSize;

	int frameWidth;
	int frameHeight;

	float overlap(float x1, float w1, float x2, float w2);
	float boxIntersection(DetectedObject a, DetectedObject b);
	float boxUnion(DetectedObject a, DetectedObject b);
	float boxIoU(DetectedObject a, DetectedObject b);
	void doNMS(std::vector<DetectedObject>& objects, float thresh);
	std::vector < DetectedObject > yoloNetParseOutput(float *net_out, int class_num, int modelWidth, int modelHeight, float threshold);
	void printPerformanceCounters(const std::map<std::string, InferenceEngine::InferenceEngineProfileInfo>& perfomanceMap);
};

