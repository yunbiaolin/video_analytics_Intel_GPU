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

/*
// brief The entry point for the Inference Engine object_detection sample application
*/

#include "main.h"
#include "my_ie.h"
#include <chrono>
#include <thread>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

// =================================================================
// Intel Media SDK
// =================================================================
#include <mfxvideo++.h>
#include <mfxstructures.h>
#include <unistd.h>
#include "common_utils.h"
#include "box.h"

#include "bufferqueue.h"

// =================================================================
// Inference Engine Interface
// =================================================================

#include "my_ie.h"

using namespace cv;

#define DUMP_VPP_OUTPUT  0   // Enabe VPP output dump for validation purpose
#define VERIFY_PURPOSE   0   // Verify the inference result
#define INFERENCE_ENABLE 1
#define NUM_OF_CHANNELS 48   // Maximum supported decoding and video processing 


// Helper macro definitions
#define MSDK_PRINT_RET_MSG(ERR)         {PrintErrString(ERR, __FILE__, __LINE__);}
#define MSDK_CHECK_RESULT(P, X, ERR)    {if ((X) > (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_POINTER(P, ERR)      {if (!(P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_ERROR(P, X, ERR)     {if ((X) == (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_IGNORE_MFX_STS(P, X)       {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_BREAK_ON_ERROR(P)          {if (MFX_ERR_NONE != (P)) break;}
#define MSDK_SAFE_DELETE_ARRAY(P)       {if (P) {delete[] P; P = NULL;}}
#define MSDK_ALIGN32(X)                 (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)             (((value + 15) >> 4) << 4)
#define MSDK_SAFE_RELEASE(X)            {if (X) { X->Release(); X = NULL; }}
#define MSDK_MAX(A, B)                  (((A) > (B)) ? (A) : (B))
#define MSDK_MIN(A, B)                  (((A) < (B)) ? (A) : (B))

#define MFX_MAKEFOURCC(A,B,C,D)    ((((int)A))+(((int)B)<<8)+(((int)C)<<16)+(((int)D)<<24))

#ifndef MFX_FOURCC_RGBP
#define MFX_FOURCC_RGBP  MFX_MAKEFOURCC('R','G','B','P')
#endif

chrono::high_resolution_clock::time_point tmStart, tmEnd;
chrono::high_resolution_clock::time_point time1,time2;

double pre_stagesum=0;
std::chrono::duration<double> diff;

vector<double> pre_stage_times;
vector<double> infer_times;
vector<double> post_stage_times;

int nBatch=0;


CMyIE *g_pIE[NUM_OF_CHANNELS];
Mat* g_resized[NUM_OF_CHANNELS];
size_t g_batchSize;
std::string g_labels[NUMLABELS];

int gNet_input_width[NUM_OF_CHANNELS];
int gNet_input_height[NUM_OF_CHANNELS];

void App_ShowUsage(void);

bool IE_Execute(int nchannleID, mfxU16 w_ie, mfxU16 h_ie, float normalizefactor, unsigned char *pIE_InputFrame, int *batchindex);
bool IE_Execute_LowLatency(size_t *m);
void IE_DrawBoundingBox(int nchannleID );
void IE_SaveOutput(int nchannleID, bool bSingleImageMode, VideoWriter *vw);
mfxStatus WriteRawFrameToMemory(mfxFrameSurface1* pSurface, int dest_w, int dest_h, unsigned char* dest, mfxU32 fourCC);
bool PrepareData(mfxFrameSurface1* pSurface, mfxU32 fourCC, mfxU16 w_ie, mfxU16 h_ie, float normalizefactor, size_t *m, int *batchindex);


struct DecThreadConfig
{
public:
    DecThreadConfig()
    {
    };
    int totalDecNum;
    FILE *f_i;
    MFXVideoDECODE *pmfxDEC;
    mfxU16 nDecSurfNum;
    mfxU16 nVPP_In_SurfNum;
    mfxBitstream *pmfxBS;
    mfxFrameSurface1** pmfxDecSurfaces;
    mfxFrameSurface1** pmfxVPP_In_Surfaces;
    mfxFrameSurface1** pmfxVPP_Out_Surfaces;
    MFXVideoSession *  pmfxSession;
    BufferQueue      * pVideoProcessINQ;
    BufferQueue      * pVideoProcessOutQ;
    mfxFrameAllocator *pmfxAllocator;
    MFXVideoVPP       *pmfxVPP;

    int decOutputFile;
    int nChannel;
    int nFPS;
    bool bStartCount;
    int nFrameProcessed;

    std::chrono::high_resolution_clock::time_point tmStart;
    std::chrono::high_resolution_clock::time_point tmEnd;

};

// ================= Decoding Thread =======
void *DecodeThreadFunc(void *arg)
{
    struct DecThreadConfig *pDecConfig = NULL;
    int nFrame = 0;
    mfxBitstream mfxBS;
    mfxStatus sts = MFX_ERR_NONE;
    mfxSyncPoint syncpDec;
    mfxSyncPoint syncpVPP;

    int nIndexDec     = 0;
    int nIndexVPP_In  = 0;
    int nIndexVPP_In2 = 0;
    int nIndexVPP_Out = 0;
    bool bNeedMore    = false;

    pDecConfig= (DecThreadConfig *) arg;
    if( NULL == pDecConfig)
    {
        std::cout << std::endl << "Failed Decode Thread Configuration" << std::endl;
        return NULL;
    }
    std::cout << std::endl <<">channel("<<pDecConfig->nChannel<<") Initialized " << std::endl;

    pDecConfig->tmStart = std::chrono::high_resolution_clock::now();
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts)
    {
        //std::cout << std::endl <<">channel("<<pDecConfig->nChannel<<") Dec: nFrame:"<<nFrame<<" totalDecNum:"<<pDecConfig->totalDecNum << std::endl;
        if (nFrame >= pDecConfig->totalDecNum) { break;};
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            usleep(1000); // Wait if device is busy, then repeat the same call to DecodeFrameAsync
        }

        if (MFX_ERR_MORE_DATA == sts)
        {   
            sts = ReadBitStreamData(pDecConfig->pmfxBS, pDecConfig->f_i); // Read more data into input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }
        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
        {
            nIndexDec =GetFreeSurfaceIndex(pDecConfig->pmfxDecSurfaces, pDecConfig->nDecSurfNum);
            if(nIndexDec == MFX_ERR_NOT_FOUND){
                 std::cout << std::endl<< ">channel("<<pDecConfig->nChannel<<") >> Not able to find an avaialbe decoding recon surface" << std::endl;
            }
        }

        if(bNeedMore == false)
        {
                if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
                {
                
                    nIndexVPP_In = GetFreeSurfaceIndex(pDecConfig->pmfxVPP_In_Surfaces, pDecConfig->nVPP_In_SurfNum); // Find free frame surface
                    if(nIndexVPP_In == MFX_ERR_NOT_FOUND){
                         std::cout << ">> Not able to find an avaialbe decoding surface" << std::endl;
                    }

                    if(nIndexVPP_In == MFX_ERR_NOT_FOUND){
                         std::cout << ">channel("<<pDecConfig->nChannel<<") >> Not able to find an avaialbe VPP input surface" << std::endl;
                    }
                }
        }

        // Decode a frame asychronously (returns immediately)
        //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
            sts = pDecConfig->pmfxDEC->DecodeFrameAsync(pDecConfig->pmfxBS, pDecConfig->pmfxDecSurfaces[nIndexDec], &(pDecConfig->pmfxVPP_In_Surfaces[nIndexVPP_In]), &syncpDec);

        // Ignore warnings if output is available,eat the D
        // if no output and no action required just repecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpDec)
        {
            bNeedMore = false;
            sts = MFX_ERR_NONE;
        }
        else if(MFX_ERR_MORE_DATA == sts)
        {
            bNeedMore = true;
            //std::cout << std::endl<< ">channel("<<pDecConfig->nChannel<<") sts: MFX_ERR_MORE_DATA" << std::endl;
        }
        else if(MFX_ERR_MORE_SURFACE == sts)
        {
        	bNeedMore = true;
        	//std::cout << std::endl<< ">channel("<<pDecConfig->nChannel<<") sts: MFX_ERR_MORE_SURFACE" << std::endl;
        }
        //else
        //    std::cout << std::endl<< ">channel("<<pDecConfig->nChannel<<") sts: " << sts << std::endl;

         if (MFX_ERR_NONE == sts )
         {

            if(pDecConfig->bStartCount == true){
                pDecConfig->nFrameProcessed ++;
            }

            nFrame++;
            if ((nFrame % pDecConfig->nFPS) == 0) 
            {
                //std::cout <<std::endl << "Channel("<<pDecConfig->nChannel<<") VPP: Try to get VPP out surface index" << std::endl;
                int  nIndexVPP_Out =pDecConfig->pVideoProcessOutQ->dequeueBuffer(1);

                //std::cout <<std::endl << "Channel("<<pDecConfig->nChannel<<") VPP In: " << nIndexVPP_In << ">> VPP out: " << nIndexVPP_Out << std::endl;

                for (;;)
                {
                     //std::cout << std::endl << "Channel("<<pDecConfig->nChannel<<") Start VPP!" << std::endl;

                     // Process a frame asychronously (returns immediately)
                     time1 = std::chrono::high_resolution_clock::now();
 
                     sts = pDecConfig->pmfxVPP->RunFrameVPPAsync(pDecConfig->pmfxVPP_In_Surfaces[nIndexVPP_In], pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out), NULL, &syncpVPP);

                     //std::cout << std::endl << "channel("<<pDecConfig->nChannel<<") Done VPP!" << std::endl;

                     if (MFX_ERR_NONE < sts && !syncpVPP) // repeat the call if warning and no output
                     {
                         if (MFX_WRN_DEVICE_BUSY == sts)
                         {
                             std::cout << std::endl << "> warning: MFX_WRN_DEVICE_BUSY" << std::endl;
                             usleep(1000); // wait if device is busy
                         }
                     }
                     else if (MFX_ERR_NONE < sts && syncpVPP)
                     {
                            sts = MFX_ERR_NONE; // ignore warnings if output is available
                            break;
                     }
                     else
                         break; // not a warning
                 }

                 // VPP needs more data, let decoder decode another frame as input
                 if (MFX_ERR_MORE_DATA == sts)
                 {
                     continue;
                 }
                 else if (MFX_ERR_MORE_SURFACE == sts)
                 {
                    // Not relevant for the illustrated workload! Therefore not handled.
                    // Relevant for cases when VPP produces more frames at output than consumes at input. E.g. framerate conversion 30 fps -> 60 fps
                    break;
                 }
                 //else
                 // std::cout << ">> Vpp sts" << sts << std::endl;
                 // MSDK_BREAK_ON_ERROR(sts);
                
                 //std::cout << std::endl << "Channel("<<pDecConfig->nChannel<<") Vpp: sts" << sts << std::endl;
                 if (MFX_ERR_NONE == sts)
                 { 
                    //std::cout << std::endl << "Channel("<<pDecConfig->nChannel<<") Vpp sync oepration xxxx1."  << std::endl;
                 
                    sts = pDecConfig->pmfxSession->SyncOperation(syncpVPP, 60000); // Synchronize. Wait until decoded frame is ready

                    if(DUMP_VPP_OUTPUT)
                    {

                        if(pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out) == NULL){
                            std::cout << ">> buffer is empty"	<< std::endl;
                        }
                        pDecConfig->pmfxAllocator->Lock(pDecConfig->pmfxAllocator->pthis, 
                                                      pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data.MemId, 
                                                      &(pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data));

                        mfxFrameInfo* pInfo = &(pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Info);
                        mfxFrameData* pData = &(pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data);
                        mfxU32 i, j, h, w;
                        mfxU32 iYsize, ipos;
                        std::cout << ">> 4" << pData<<std::endl;

                         
                        mfxU8* ptr;

                        if (pInfo->CropH > 0 && pInfo->CropW > 0)
                        {
                            w = pInfo->CropW;
                            h = pInfo->CropH;
                        }
                        else
                        {
                            w = pInfo->Width;
                            h = pInfo->Height;
                        }

                        ptr = MSDK_MIN( MSDK_MIN(pData->R, pData->G), pData->B);
                        ptr = ptr + pInfo->CropX + pInfo->CropY * pData->Pitch;
                        int dest_w = w;
                        int dest_h =h;
                        unsigned char *dest = (unsigned char *)malloc(dest_w*dest_h*4);
                        for(i = 0; i < dest_h; i++)
                        memcpy(dest + i*dest_w*4, ptr + i * pData->Pitch, dest_w*4);

                        FILE *fp = fopen("./dump.rgb32","a+b");
                        if(fp != NULL){
                          fwrite(dest, 1, dest_w*dest_h*4, fp);
                          fclose(fp);
                        }
                        free(dest);
                        dest=NULL;

                        // memcpy(dest + i * (pInfo->CropW), pData->Y + (pInfo->CropY * pData->Pitch + pInfo->CropX) + i * pData->Pitch, pInfo->CropW);
                        pDecConfig->pmfxAllocator->Unlock(pDecConfig->pmfxAllocator->pthis, 
                                                          pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data.MemId, 
                                                          &(pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data));


                        std::cout << ">> 5"  <<std::endl;

                        }

                     }


                     //std::cout << std::endl << "channel("<<pDecConfig->nChannel<<") Vpp: sts is complete" << sts << std::endl;

                     pDecConfig->pVideoProcessOutQ->enqueueBuffer(pDecConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out),1);//Send IE for further processing
                     }
                }
            
       }

      pDecConfig->tmEnd = std::chrono::high_resolution_clock::now();

      std::cout << std::endl<< "channel("<<pDecConfig->nChannel<<") pDecoding is done\r\n"<< std::endl;

    return (void *)0;

}


class InferThreadConfig
 {
 public:
     InferThreadConfig()
     {
     };
     int totalDecNum;
     MFXVideoVPP  *pmfxVPP;
     mfxU16 nDecSurfNum;
     mfxU16 nVPP_In_SurfNum;
     mfxFrameSurface1** pmfxVPP_In_Surfaces;
     mfxFrameSurface1** pmfxVPP_Out_Surfaces;
     MFXVideoSession *pmfxSession;

     BufferQueue      *pVideoProcessINQ;
     BufferQueue      *pVideoProcessOutQ;
     mfxFrameAllocator *pmfxAllocator;
     unsigned char  *pResizedFrame;

     int gNet_input_width;
     int gNet_input_height;

     float normalize_factor;
     VideoWriter *pvideo;
     int nChannel;
 };

void *InferThreadFunc(void *arg)
{
    struct InferThreadConfig *pInferConfig = NULL;
    int nFrame = 0;
    mfxStatus sts = MFX_ERR_NONE;
    mfxSyncPoint syncpVPP;
 
    int nIndexDec	   = 0;
    int nIndexVPP_In  = 0;
    int nIndexVPP_Out = 0;
    bool bret    = false;

    pInferConfig= (InferThreadConfig *) arg;
    if( NULL == pInferConfig)
    {
        std::cout << std::endl << "Failed Inference Thread Configuration" << std::endl;
        return NULL;
    }
#if 1
    // Always waiting for feed from decoding process.
    while (1)
    {
        int current_batchindex = 0;
        if (nFrame >= pInferConfig->totalDecNum) break;

        nIndexVPP_Out = pInferConfig->pVideoProcessOutQ->acquireBuffer(2);

        #if 1// INFERENCE_PHASE_1_ENABLE

        pInferConfig->pmfxAllocator->Lock(pInferConfig->pmfxAllocator->pthis, pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data.MemId, &(pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data));
        WriteRawFrameToMemory(pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out), pInferConfig->gNet_input_width, pInferConfig->gNet_input_height,  pInferConfig->pResizedFrame, MFX_FOURCC_RGBP);
        pInferConfig->pmfxAllocator->Unlock(pInferConfig->pmfxAllocator->pthis, pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data.MemId, &(pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data));
        #endif

        pInferConfig->pVideoProcessOutQ->returnBuffer(nIndexVPP_Out, 2);
        nFrame++;
    #if 1 //INFERENCE_PHASE_2_ENABLE			
        bret = IE_Execute(pInferConfig->nChannel, pInferConfig->gNet_input_width, pInferConfig->gNet_input_height, pInferConfig->normalize_factor,  pInferConfig->pResizedFrame, &current_batchindex);
        if(bret == false)
        {
            std::cout << "\t. [error] Failed to execute inferencing." << std::endl;
            break;
        }

    #if VERIFY_PURPOSE
        if(current_batchindex == g_batchSize)
           IE_SaveOutput(pInferConfig->nChannel,false, pInferConfig->pvideo);
    #endif
    #endif
     
    }
    std::cout << std::endl<<  "Channel ("<<pInferConfig->nChannel<<") Inference thread is done\r\n"<< std::endl;
#else
    size_t m = 0;
	while (1)
	{
		int batchindex = 0;
	
		if (nFrame >= pInferConfig->totalDecNum) break;

		nIndexVPP_Out = pInferConfig->pVideoProcessOutQ->acquireBuffer(2);
		pInferConfig->pmfxAllocator->Lock(pInferConfig->pmfxAllocator->pthis, pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data.MemId, &(pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data));

		PrepareData(pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out), MFX_FOURCC_RGBP, pInferConfig->gNet_input_width, pInferConfig->gNet_input_height, pInferConfig->normalize_factor, &m, &batchindex);

		pInferConfig->pmfxAllocator->Unlock(pInferConfig->pmfxAllocator->pthis, pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data.MemId, &(pInferConfig->pVideoProcessOutQ->getBuffer(nIndexVPP_Out)->Data));

		pInferConfig->pVideoProcessOutQ->returnBuffer(nIndexVPP_Out, 2);
		nFrame++;
	    if(batchindex == g_batchSize)	
	    {
			bret = IE_Execute_LowLatency(&m);
			if(bret == false)
			{
				std::cout << "\t. [error] Failed to execute inferencing." << std::endl;
				break;
			}

	    }

	}
#endif	
   std::cout << std::endl<<  "Channel ("<<pInferConfig->nChannel<<") Inference thread is done\r\n"<< std::endl;
 
   return (void *)0;


}

 
int main(int argc, char *argv[])
{
    bool bret = false;

    // =================================================================

    // Parse command line parameters
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);

    if (FLAGS_h) {
        App_ShowUsage();
        return 0;
    }

    if (FLAGS_l.empty()) {
        std::cout << " [error] Labels file path not set" << std::endl;
        App_ShowUsage();
        return 1;
    }

    bool noPluginAndBadDevice = FLAGS_p.empty() && FLAGS_d.compare("CPU")
                                && FLAGS_d.compare("GPU");
    if (FLAGS_i.empty() || FLAGS_m.empty() || noPluginAndBadDevice) {
        if (noPluginAndBadDevice)
            std::cout << " [error] Device is not supported" << std::endl;
        if (FLAGS_m.empty())
            std::cout << " [error] File with model - not set" << std::endl;
        if (FLAGS_i.empty())
            std::cout << " [error] Image(s) for inference - not set" << std::endl;
        App_ShowUsage();
        return 1;
    }

    if ((FLAGS_t.compare("SSD") && (FLAGS_t.compare("YOLO") && (FLAGS_t.compare("YOLO-tiny")))) != 0) {
        std::cout << " [error] Inference type must be SSD, YOLO, or YOLO-tiny" << std::endl;
        App_ShowUsage();
        return 1;
    }

    // prepare video input
    std::cout << std::endl;

    std::string input_filename = FLAGS_i;
    bool bSingleImageMode = false;
    std::vector<std::string> imgExt = { ".bmp", ".jpg" };

    for (size_t i = 0; i < imgExt.size(); i++) {
        if (input_filename.rfind(imgExt[i]) != std::string::npos) {
            bSingleImageMode = true;
            std::cout << "> Use [Intel CV SDK] only." << std::endl;
            break;
        }
    }

// =================================================================
// Intel Media SDK
//
// if input file has ".264" extension, let assume we want to enable
// media sdk to decode video elementary stream here.
    bool bUseMediaSDK = false;

    if(bSingleImageMode == false)
    {
        std::vector<std::string> eleExt = {".264", ".h264"};
        for (size_t i = 0; i < eleExt.size(); i++)
        {
            if (input_filename.rfind(eleExt[i]) != std::string::npos)
            {
                bUseMediaSDK = true;
                std::cout << "> Use [Intel Media SDK] and [CV SDK]." << std::endl;
                break;
            }
        }
    }

// Check input file state
    std::ifstream infile(FLAGS_i);

    if(!infile.is_open())
    {
        std::cout << "\t. [error] Can't open input file." << std::endl;
        return 1;
    }



// =================================================================

// Read class names
    std::cout << "> Read labels for image classification." << std::endl;

    std::ifstream labelfile(FLAGS_l);

    if (!labelfile.is_open()) {
        std::cout << "\t. [error] Can't open labels file." << std::endl;
        return 1;
    }

    for (int i = 0; i < NUMLABELS; i++) {
        getline(labelfile, g_labels[i]);
    }

#ifdef WIN32
    std::string archPath = "../../../bin" OS_LIB_FOLDER "intel64/Release/";
#else
    std::string archPath = "../../../lib/" OS_LIB_FOLDER "intel64";
#endif

//-------------------------------------------
// Inference Engine Initialization
    std::cout << std::endl << "> Init CV SDK IE session." << std::endl;
    VideoWriter *video[NUM_OF_CHANNELS];
    for(int nLoop=0; nLoop< FLAGS_c; nLoop++)
    {

        g_pIE[nLoop] = new CMyIE;

    // Load plugin
        std::cout << "\t. Load inference engine plugin (" << FLAGS_p << ")." << std::endl;

        std::vector<std::string> pluginDirs = {FLAGS_pp, archPath, DEFAULT_PATH_P, ""};
        std::string plugin = FLAGS_p;   // mklDNN/ clDNN plugin
        std::string device = FLAGS_d;   // CPU or GPU

         g_pIE[nLoop]->LoadPlugIn(pluginDirs, plugin, device);

    // Enable performance counters
        if (FLAGS_pc) {
            std::cout << "\t. Enable performance counters." << std::endl;
             g_pIE[nLoop]->EnablePerformanceCounters();
        }

    // Read model (network, weights)
        std::cout << "\t. Read model (network, weights)." << std::endl;
        bret =  g_pIE[nLoop]->ReadModel(FLAGS_m);

        if (bret == false)
        {
            std::cout << "\t. [error] Failed to read model." << std::endl;
            return 1;
        }

    // Set the target device
        if (!FLAGS_d.empty())
        {
            std::cout << "\t. Set the target device: " << FLAGS_d << std::endl;
             g_pIE[nLoop]->SetTargetDevice(FLAGS_d);
        }

        if (bSingleImageMode)
            g_batchSize = 1;
        else
            g_batchSize = FLAGS_batch;

    // Set batch size
        std::cout << "\t. Set batch size: " << g_batchSize << std::endl;
         g_pIE[nLoop]->SetBatchSize(g_batchSize);

        //std::cout << "Batch size = " << g_pIE->GetBatchSize(); << std::endl;

    //  Inference engine input/output setup
        std::cout << "\t. Setting-up input, output blobs..." << std::endl;

  

        bret =  g_pIE[nLoop]->IOBlobsSetting(&gNet_input_width[nLoop], 
        &gNet_input_height[nLoop]);
        std::cout << "\t gNet_input_width." 
        <<gNet_input_width[nLoop]<<"gNet_input_height"<<gNet_input_height[nLoop]<< std::endl;

        if (bret == false)
        {
            std::cout << "\t. [error] Failed to set IO blobs." << std::endl;
            return 1;
        }

    // Prepare output file to archive result video
        char szBuffer[256]={0};
        sprintf(szBuffer,"out_%d.h264",nLoop);
        video[nLoop]= new VideoWriter(szBuffer, CV_FOURCC('H', '2', '6', '4'), 10, 
        Size(gNet_input_width[nLoop], gNet_input_height[nLoop]), true);
    // Load Network to plugin
        std::cout << "\t. Load network to plugin." << std::endl;

        bret =  g_pIE[nLoop]->LoadNetwork();

        if (bret == false)
        {
            std::cout << "\t. [error] Failed to load network." << std::endl;
            return 1;
        }
 
    }
    std::cout << "> Done Initialization CV SDK IE session." << std::endl;

    Mat frame;
    float normalize_factor = 1.0;

    if (FLAGS_t.compare("SSD") != 0)
        normalize_factor = 256;
// Done, Inference Engine Initialization

//-------------------------------------------

    bool no_more_data = false;
    int totalFrames = 0;

    // =================================================================
    // Intel Media SDK
    //
    // Use Intel Media SDK for decoding and VPP (resizing)

    //std::vector<std::thread > vWorkThreads;
    mfxStatus sts;
    mfxIMPL impl; 				  // SDK implementation type: hardware accelerator?, software? or else
    mfxVersion ver;				  // media sdk version

    // mfxSession and Allocator can be shared globally.
    MFXVideoSession   mfxSession[NUM_OF_CHANNELS];
    mfxFrameAllocator mfxAllocator[NUM_OF_CHANNELS];	 

    BufferQueue *pVideoProcessINQ[NUM_OF_CHANNELS];
    BufferQueue *pVideoProcessOutQ[NUM_OF_CHANNELS];

    FILE *f_i[NUM_OF_CHANNELS];
    mfxBitstream mfxBS[NUM_OF_CHANNELS];
    mfxFrameSurface1** pmfxDecSurfaces[NUM_OF_CHANNELS];
    mfxFrameSurface1** pmfxVPP_In_Surfaces[NUM_OF_CHANNELS];
    mfxFrameSurface1** pmfxVPP_Out_Surfaces[NUM_OF_CHANNELS];

    unsigned char *pResizedFrame;
    std::vector<std::thread >        vWorkThreads;
    std::vector<DecThreadConfig *>   vpDecThradConfig;
    std::vector<InferThreadConfig *> vpInferThreadConfig;
    std::vector<MFXVideoDECODE *>    vpMFXDec;
    std::vector<MFXVideoVPP *>       vpMFXVpp;

    std::cout << "channels of stream:"<<FLAGS_c << std::endl;
    std::cout << " FPS of each inference workload:"<<FLAGS_fps << std::endl; 

    for(int nLoop=0; nLoop< FLAGS_c; nLoop++)
    {
        
        g_resized[nLoop] = new Mat[g_batchSize];
        
        // =================================================================
         // Intel Media SDK
        std::cout << "\t. Open input file: " << input_filename << std::endl;
        std::cout << "\t. Create output file: ./resized.rgb32" << std::endl;

        f_i[nLoop] = fopen(input_filename.c_str(), "rb");
        //f_o = fopen("./resized.rgb32", "wb");

        // Initialize Intel Media SDK session
        // - MFX_IMPL_AUTO_ANY selects HW accelaration if available (on any adapter)
        // - Version 1.0 is selected for greatest backwards compatibility.
        //   If more recent API features are needed, change the version accordingly

        std::cout << std::endl << "> Declare Intel Media SDK video session and Init." << std::endl;
        sts = MFX_ERR_NONE;
        impl = MFX_IMPL_AUTO_ANY;
        ver = { {0, 1} };
        sts = Initialize(impl, ver, &mfxSession[nLoop], &mfxAllocator[nLoop]);

        MFXVideoDECODE *pmfxDEC = new MFXVideoDECODE(mfxSession[nLoop]);
        vpMFXDec.push_back(pmfxDEC);
        MFXVideoVPP    *pmfxVPP = new MFXVideoVPP(mfxSession[nLoop]);
        vpMFXVpp.push_back(pmfxVPP);

        // [DECODER]
        // Initialize decode video parameters
        // - In this example we are decoding an AVC (H.264) stream
        // - For simplistic memory management, system memory surfaces are used to store the decoded frames
        std::cout << "\t. Start preparing video parameters for decoding." << std::endl;
        mfxVideoParam DecParams;
        memset(&DecParams, 0, sizeof(DecParams));

        DecParams.mfx.CodecId = MFX_CODEC_AVC;                  // h264
        DecParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;   // will use GPU memory in this example

        std::cout << "\t\t.. Suppport H.264(AVC) and use video memory." << std::endl;

        // Prepare media sdk bit stream buffer
        // - Arbitrary buffer size for this example        
        memset(&mfxBS[nLoop], 0, sizeof(mfxBS[nLoop]));
        mfxBS[nLoop].MaxLength = 1024 * 1024;
        mfxBS[nLoop].Data = new mfxU8[mfxBS[nLoop].MaxLength];
        MSDK_CHECK_POINTER(mfxBS[nLoop].Data, MFX_ERR_MEMORY_ALLOC);

        // Read a chunk of data from stream file into bit stream buffer
        // - Parse bit stream, searching for header and fill video parameters structure
        // - Abort if bit stream header is not found in the first bit stream buffer chunk
        sts = ReadBitStreamData(&mfxBS[nLoop], f_i[nLoop]);
        
        sts = pmfxDEC->DecodeHeader(&mfxBS[nLoop], &DecParams);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        
        std::cout << "\t. Done preparing video parameters." << std::endl;

        // [VPP]
        // Initialize VPP parameters
        // - For simplistic memory management, system memory surfaces are used to store the raw frames
        //   (Note that when using HW acceleration D3D surfaces are prefered, for better performance)
        std::cout << "\t. Start preparing VPP In/ Out parameters." << std::endl;

        mfxU32 fourCC_VPP_Out =  MFX_FOURCC_RGBP;
        mfxU32 chromaformat_VPP_Out = MFX_CHROMAFORMAT_YUV444;
        mfxU8 bpp_VPP_Out     = 24;
        mfxU8 channel_VPP_Out = 3;

        mfxVideoParam VPPParams;
        mfxExtVPPScaling scalingConfig;
        memset(&VPPParams, 0, sizeof(VPPParams));
        memset(&scalingConfig, 0, sizeof(mfxExtVPPScaling));

        mfxExtBuffer *pExtBuf[1];
        VPPParams.ExtParam = (mfxExtBuffer **)pExtBuf; 
        scalingConfig.Header.BufferId    = MFX_EXTBUFF_VPP_SCALING;
        scalingConfig.Header.BufferSz    = sizeof(mfxExtVPPScaling);
        scalingConfig.ScalingMode        = MFX_SCALING_MODE_LOWPOWER;
        VPPParams.ExtParam[0] = (mfxExtBuffer*)&(scalingConfig);
        VPPParams.NumExtParam = 1;
        printf("Num of ext params=%d\r\n",VPPParams.NumExtParam );


        // Video processing input data format / decoded frame information
        VPPParams.vpp.In.FourCC         = MFX_FOURCC_NV12;
        VPPParams.vpp.In.ChromaFormat   = MFX_CHROMAFORMAT_YUV420;
        VPPParams.vpp.In.CropX          = 0;
        VPPParams.vpp.In.CropY          = 0;
        VPPParams.vpp.In.CropW          = DecParams.mfx.FrameInfo.CropW;
        VPPParams.vpp.In.CropH          = DecParams.mfx.FrameInfo.CropH;
        VPPParams.vpp.In.PicStruct      = MFX_PICSTRUCT_PROGRESSIVE;
        VPPParams.vpp.In.FrameRateExtN  = 30;
        VPPParams.vpp.In.FrameRateExtD  = 1;
        // width must be a multiple of 16
        // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
        VPPParams.vpp.In.Width  = MSDK_ALIGN16(VPPParams.vpp.In.CropW);
        VPPParams.vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.In.PicStruct)?
                                  MSDK_ALIGN16(VPPParams.vpp.In.CropH) : MSDK_ALIGN32(VPPParams.vpp.In.CropH);

        // Video processing output data format / resized frame information for inference engine
        VPPParams.vpp.Out.FourCC        = fourCC_VPP_Out;
        VPPParams.vpp.Out.ChromaFormat  = chromaformat_VPP_Out;
        VPPParams.vpp.Out.CropX         = 0;
        VPPParams.vpp.Out.CropY         = 0;
        VPPParams.vpp.Out.CropW         = gNet_input_width[nLoop]; // SSD: 300, YOLO-tiny: 448
        VPPParams.vpp.Out.CropH         = gNet_input_height[nLoop];    // SSD: 300, YOLO-tiny: 448
        VPPParams.vpp.Out.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
        VPPParams.vpp.Out.FrameRateExtN = 30;
        VPPParams.vpp.Out.FrameRateExtD = 1;
        // width must be a multiple of 16
        // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
        VPPParams.vpp.Out.Width  = MSDK_ALIGN16(VPPParams.vpp.Out.CropW);
        VPPParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.Out.PicStruct)?
                                   MSDK_ALIGN16(VPPParams.vpp.Out.CropH) : MSDK_ALIGN32(VPPParams.vpp.Out.CropH);

        VPPParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

        std::cout << "\t. VPP ( " << VPPParams.vpp.In.CropW << " x " << VPPParams.vpp.In.CropH << " )"
                  << " -> ( " << VPPParams.vpp.Out.CropW << " x " << VPPParams.vpp.Out.CropH << " )" << ", ( " << VPPParams.vpp.Out.Width << " x " << VPPParams.vpp.Out.Height << " )" << std::endl;

        std::cout << "\t. Done preparing VPP In/ Out parameters." << std::endl;

        // [DECODER]
        // Query number of required surfaces
        std::cout << "\t. Query number of required surfaces for decoder and memory allocation." << std::endl;

        mfxFrameAllocRequest DecRequest = { 0 };
        sts = pmfxDEC->QueryIOSurf(&DecParams, &DecRequest);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        
        DecRequest.Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_FROM_DECODE | MFX_MEMTYPE_FROM_VPPIN;
        DecRequest.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

        // Try to add two more surfaces
        DecRequest.NumFrameMin +=2;
        DecRequest.NumFrameSuggested +=2;

        // [VPP]
        // Query number of required surfaces
        std::cout << "\t. Query number of required surfaces for VPP and memory allocation." << std::endl;

        mfxFrameAllocRequest VPPRequest[2];// [0] - in, [1] - out
        memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
        sts = pmfxVPP->QueryIOSurf(&VPPParams, VPPRequest);
        
        // [DECODER]
        // memory allocation
        mfxFrameAllocResponse DecResponse = { 0 };
        sts = mfxAllocator[nLoop].Alloc(mfxAllocator[nLoop].pthis, &DecRequest, &DecResponse);

        if(MFX_ERR_NONE > sts)
        {
            MSDK_PRINT_RET_MSG(sts);
            goto exit_here;
        }
        
        mfxU16 nDecSurfNum = DecResponse.NumFrameActual;

        std::cout << "\t\t. Decode Surface Actual Number: " << nDecSurfNum << std::endl;

        // this surface will be used when decodes encoded stream
        pmfxDecSurfaces[nLoop] = new mfxFrameSurface1 * [nDecSurfNum];

        if(!pmfxDecSurfaces[nLoop] )
        {
            MSDK_PRINT_RET_MSG(MFX_ERR_MEMORY_ALLOC);            
            goto exit_here;
        }
        
        for (int i = 0; i < nDecSurfNum; i++)
        {
            pmfxDecSurfaces[nLoop][i] = new mfxFrameSurface1;
            memset(pmfxDecSurfaces[nLoop][i], 0, sizeof(mfxFrameSurface1));
            memcpy(&(pmfxDecSurfaces[nLoop][i]->Info), &(DecParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

            // external allocator used - provide just MemIds
            pmfxDecSurfaces[nLoop][i]->Data.MemId = DecResponse.mids[i];
        }

        // [VPP_In]
        // memory allocation
        mfxFrameAllocResponse VPP_In_Response = { 0 };
        memcpy(&VPPRequest[0].Info, &(VPPParams.vpp.In), sizeof(mfxFrameInfo)); // allocate VPP input frame information

        sts = mfxAllocator[nLoop].Alloc(mfxAllocator[nLoop].pthis, &(VPPRequest[0]), &VPP_In_Response);

        printf("sts=%d\r\n", sts);
        if(MFX_ERR_NONE > sts)
        {
            MSDK_PRINT_RET_MSG(sts);
            goto exit_here;
        }

        mfxU16 nVPP_In_SurfNum = VPP_In_Response.NumFrameActual;

        std::cout << "\t\t. VPP In Surface Actual Number: " << nVPP_In_SurfNum << std::endl;

        // this surface will contain decoded frame
        pmfxVPP_In_Surfaces[nLoop] = new mfxFrameSurface1 * [nVPP_In_SurfNum];

        if(!pmfxVPP_In_Surfaces[nLoop])
        {
            MSDK_PRINT_RET_MSG(MFX_ERR_MEMORY_ALLOC);            
            goto exit_here;
        }
        
        for (int i = 0; i < nVPP_In_SurfNum; i++)
        {
            pmfxVPP_In_Surfaces[nLoop][i] = new mfxFrameSurface1;
            memset(pmfxVPP_In_Surfaces[nLoop][i], 0, sizeof(mfxFrameSurface1));
            memcpy(&(pmfxVPP_In_Surfaces[nLoop][i]->Info), &(VPPParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

            // external allocator used - provide just MemIds
            pmfxVPP_In_Surfaces[nLoop][i]->Data.MemId = VPP_In_Response.mids[i];
        }

        // [VPP_Out]
        // memory allocation
        mfxFrameAllocResponse VPP_Out_Response = { 0 };
        memcpy(&VPPRequest[1].Info, &(VPPParams.vpp.Out), sizeof(mfxFrameInfo));    // allocate VPP output frame information

        sts = mfxAllocator[nLoop].Alloc(mfxAllocator[nLoop].pthis, &(VPPRequest[1]), &VPP_Out_Response);

        if(MFX_ERR_NONE > sts)
        {
            MSDK_PRINT_RET_MSG(sts);
            goto exit_here;
        }

        mfxU16 nVPP_Out_SurfNum = VPP_Out_Response.NumFrameActual;

        std::cout << "\t\t. VPP Out Surface Actual Number: " << nVPP_Out_SurfNum << std::endl;

        // this surface will contain resized frame (video processed)
        pmfxVPP_Out_Surfaces[nLoop] = new mfxFrameSurface1 * [nVPP_Out_SurfNum];

        if(!pmfxVPP_Out_Surfaces[nLoop])
        {
            MSDK_PRINT_RET_MSG(MFX_ERR_MEMORY_ALLOC);            
            goto exit_here;
        }

        for (int i = 0; i < nVPP_Out_SurfNum; i++)
        {
            pmfxVPP_Out_Surfaces[nLoop][i] = new mfxFrameSurface1;
            memset(pmfxVPP_Out_Surfaces[nLoop][i], 0, sizeof(mfxFrameSurface1));
            memcpy(&(pmfxVPP_Out_Surfaces[nLoop][i]->Info), &(VPPParams.vpp.Out), sizeof(mfxFrameInfo));

            // external allocator used - provide just MemIds
            pmfxVPP_Out_Surfaces[nLoop][i]->Data.MemId = VPP_Out_Response.mids[i];
        }

        // Initialize the Media SDK decoder
        std::cout << "\t. Init Intel Media SDK Decoder" << std::endl;

        sts = pmfxDEC->Init(&DecParams);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        
        if(MFX_ERR_NONE > sts)
        {
            MSDK_PRINT_RET_MSG(sts);
            goto exit_here;
        }

        std::cout << "\t. Done decoder Init" << std::endl;

        // Initialize Media SDK VPP
        std::cout << "\t. Init Intel Media SDK VPP" << std::endl;

        sts = pmfxVPP->Init(&VPPParams);
        printf("VPP ext buffer=%d\r\n",VPPParams.NumExtParam );
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        std::cout << "\t. Done VPP Init" << std::endl;

        std::cout << "> Done Intel Media SDK session, decoder, VPP initialization." << std::endl;
        std::cout << std::endl;

       // Start decoding the frames from the stream

       pResizedFrame = (unsigned char *)malloc(gNet_input_width[nLoop] * 
       gNet_input_height[nLoop] * channel_VPP_Out); // resized frame from VPP


        // =====Create three threads (One for Encoding/VideoProcessing/Inference/)===
        pVideoProcessOutQ[nLoop] = new BufferQueue(100);
        // Register all VPP in buffers to the pVideoProcessQ.
        for (mfxU16 i = 0; i < nVPP_Out_SurfNum; i++)
        {
            pVideoProcessOutQ[nLoop]->registerBuffer(pmfxVPP_Out_Surfaces[nLoop][i]);
        }


        pVideoProcessINQ[nLoop] = new BufferQueue(100);
        // Register all VPP in buffers to the pVideoProcessQ.
        for (mfxU16 i = 0; i < nDecSurfNum; i++)
        {
            pVideoProcessINQ[nLoop]->registerBuffer(pmfxDecSurfaces[nLoop][i]);
        }

        for (mfxU16 i = 0; i < nDecSurfNum; i++)
        {
             std::cout<<"dec:handle (%d): "<<pmfxDecSurfaces[nLoop][i]<<std::endl;
        }


        for (mfxU16 i = 0; i < nVPP_In_SurfNum; i++)
        {
             std::cout<<"vpp:handle: "<<pmfxVPP_In_Surfaces[nLoop][i]<<std::endl;
        }

        int ret=0;


        int decOutputFile = open("./dump.nv12", O_APPEND | O_CREAT);

        DecThreadConfig *pDecThreadConfig = new DecThreadConfig();
        memset(pDecThreadConfig, 0, sizeof(DecThreadConfig));
        pDecThreadConfig->totalDecNum            = FLAGS_fr;
        pDecThreadConfig->f_i                    = f_i[nLoop];
        pDecThreadConfig->pmfxDEC                = pmfxDEC;
        pDecThreadConfig->pmfxVPP                = pmfxVPP;
        pDecThreadConfig->nDecSurfNum            = nDecSurfNum;
        pDecThreadConfig->nVPP_In_SurfNum        = nVPP_In_SurfNum;
        pDecThreadConfig->pmfxDecSurfaces        = pmfxDecSurfaces[nLoop];
        pDecThreadConfig->pmfxVPP_In_Surfaces    = pmfxVPP_In_Surfaces[nLoop];
        pDecThreadConfig->pmfxVPP_Out_Surfaces   = pmfxVPP_Out_Surfaces[nLoop];
        pDecThreadConfig->pmfxBS                 = &mfxBS[nLoop];
        pDecThreadConfig->pmfxSession            = &mfxSession[nLoop];
        pDecThreadConfig->pVideoProcessINQ       = pVideoProcessINQ[nLoop];
        pDecThreadConfig->pVideoProcessOutQ      = pVideoProcessOutQ[nLoop];
        pDecThreadConfig->pmfxAllocator          = &mfxAllocator[nLoop];
        pDecThreadConfig->decOutputFile          = decOutputFile;
        pDecThreadConfig->nChannel               = nLoop;
        pDecThreadConfig->nFPS                   = 1;//30/pDecThreadConfig->nFPS;
        pDecThreadConfig->bStartCount			 = false;
        pDecThreadConfig->nFrameProcessed		 = 0;

        vpDecThradConfig.push_back(pDecThreadConfig);
        vWorkThreads.push_back( std::thread(DecodeThreadFunc,
                                (void *)(pDecThreadConfig)
                              ) );    


        InferThreadConfig *pInferThreadConfig = new InferThreadConfig();
        memset(pInferThreadConfig, 0, sizeof(InferThreadConfig));
        pInferThreadConfig->totalDecNum            = (FLAGS_fr);///30)*pDecThreadConfig->nFPS;
        pInferThreadConfig->pmfxVPP                = pmfxVPP;
        pInferThreadConfig->nDecSurfNum            = nDecSurfNum ;
        pInferThreadConfig->nVPP_In_SurfNum        = nVPP_In_SurfNum;
        pInferThreadConfig->pmfxVPP_In_Surfaces    = pmfxVPP_In_Surfaces[nLoop];
        pInferThreadConfig->pmfxVPP_Out_Surfaces   = pmfxVPP_Out_Surfaces[nLoop];
        pInferThreadConfig->pmfxSession = &mfxSession[nLoop];
        pInferThreadConfig->pVideoProcessINQ       = pVideoProcessINQ[nLoop];
        pInferThreadConfig->pVideoProcessOutQ      = pVideoProcessOutQ[nLoop];
        pInferThreadConfig->normalize_factor       = normalize_factor;
        pInferThreadConfig->pmfxAllocator          = &mfxAllocator[nLoop];
        pInferThreadConfig->pResizedFrame          = pResizedFrame;
        pInferThreadConfig->gNet_input_width       = gNet_input_width[nLoop];
        pInferThreadConfig->gNet_input_height      = gNet_input_height[nLoop];
        pInferThreadConfig->pvideo                 = video[nLoop];
        pInferThreadConfig->nChannel               = nLoop;
        vpInferThreadConfig.push_back(pInferThreadConfig);
        vWorkThreads.push_back(  std::thread(InferThreadFunc,
                                 (void *)(pInferThreadConfig)
                                ) );    
       

    }
    // Report performance counts
    {
        std::chrono::high_resolution_clock::time_point staticsStart, staticsEnd;

        staticsStart = std::chrono::high_resolution_clock::now();
        for (auto& decReport : vpDecThradConfig) {
            decReport->bStartCount = true;
        }


        for (auto& th : vWorkThreads) {
            th.join();
        }

        staticsEnd = std::chrono::high_resolution_clock::now();
        int totalFrameProcessed = 0;
        for (auto& decReport : vpDecThradConfig) {
            totalFrameProcessed += decReport->nFrameProcessed;
        }

        chrono::duration<double> diffTime  = staticsEnd   - staticsStart;
        double fps = (totalFrameProcessed*1000/(diffTime.count()*1000.0));
        std::cout<< "TotalFames Processed:" << totalFrameProcessed <<" total latency:" << diffTime.count()*1000.0<<"ms"<<std::endl;
        std::cout<< "FPS:" << fps <<"(f/s)"<<std::endl;
    }

exit_here:

    for(int nLoop=0; nLoop< FLAGS_c; nLoop++)
    {
        fclose(f_i[nLoop]);
        //fclose(f_o);

        MSDK_SAFE_DELETE_ARRAY(pmfxDecSurfaces[nLoop]);
        MSDK_SAFE_DELETE_ARRAY(pmfxVPP_In_Surfaces[nLoop]);
        MSDK_SAFE_DELETE_ARRAY(pmfxVPP_Out_Surfaces[nLoop]);
        MSDK_SAFE_DELETE_ARRAY(mfxBS[nLoop].Data);

        MSDK_SAFE_DELETE_ARRAY(g_resized[nLoop]);

    }

    for (auto& inferConfig : vpInferThreadConfig) {
         free(inferConfig->pResizedFrame);
    }
 
    std::cout << "> Complete execution !";

    fflush(stdout);
    return 0;
}

/**
* \brief This function show a help message
*/
void App_ShowUsage()
{
    std::cout << std::endl;
    std::cout << "[usage]" << std::endl;
    std::cout << "\ttutorial_4_vmem [option]" << std::endl;
    std::cout << "\toptions:" << std::endl;
    std::cout << std::endl;
    std::cout << "\t\t-h           " << help_message << std::endl;
    std::cout << "\t\t-i <path>    " << image_message << std::endl;
    std::cout << "\t\t-fr <path>   " << frames_message << std::endl;
    std::cout << "\t\t-m <path>    " << model_message << std::endl;
    std::cout << "\t\t-l <path>    " << labels_message << std::endl;
    std::cout << "\t\t-d <device>  " << target_device_message << std::endl;
    std::cout << "\t\t-t <type>    " << infer_type_message << std::endl;
    std::cout << "\t\t-pc          " << performance_counter_message << std::endl;
    std::cout << "\t\t-thresh <val>" << threshold_message << std::endl;
    std::cout << "\t\t-b <val>     " << batch_message << std::endl;
    std::cout << "\t\t-v           " << verbose_message << std::endl << std::endl;
}


mfxStatus WriteRawFrameToMemory(mfxFrameSurface1* pSurface, int dest_w, int dest_h, unsigned char* dest, mfxU32 fourCC)
{
    mfxFrameInfo* pInfo = &pSurface->Info;
    mfxFrameData* pData = &pSurface->Data;
    mfxU32 i, j, h, w;
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 iYsize, ipos;

    chrono::high_resolution_clock::time_point time11,time21;
    std::chrono::duration<double> diff1;
    time11 = std::chrono::high_resolution_clock::now();

    if(fourCC == MFX_FOURCC_RGBP)
    {
        mfxU8* ptr;

        if (pInfo->CropH > 0 && pInfo->CropW > 0)
        {
            w = pInfo->CropW;
            h = pInfo->CropH;
        }
        else
        {
            w = pInfo->Width;
            h = pInfo->Height;
        }

        mfxU8 *pTemp = dest;
        ptr   = pData->B + (pInfo->CropX ) + (pInfo->CropY ) * pData->Pitch;

        for (i = 0; i < dest_h; i++)
        {
           memcpy(pTemp + i*dest_w, ptr + i*pData->Pitch, dest_w);
        }


        ptr	= pData->G + (pInfo->CropX ) + (pInfo->CropY ) * pData->Pitch;
        pTemp = dest + dest_w*dest_h;
        for(i = 0; i < dest_h; i++)
        {
           memcpy(pTemp  + i*dest_w, ptr + i*pData->Pitch, dest_w);
        }

        ptr	= pData->R + (pInfo->CropX ) + (pInfo->CropY ) * pData->Pitch;
        pTemp = dest + 2*dest_w*dest_h;
        for(i = 0; i < dest_h; i++)
        {
            memcpy(pTemp  + i*dest_w, ptr + i*pData->Pitch, dest_w);
        }

        //for(i = 0; i < h; i++)
        //memcpy(dest + i*w*4, ptr + i * pData->Pitch, w*4);
    }
    else if(fourCC == MFX_FOURCC_RGB4)
    {
        mfxU8* ptr;

        if (pInfo->CropH > 0 && pInfo->CropW > 0)
        {
            w = pInfo->CropW;
            h = pInfo->CropH;
        }
        else
        {
            w = pInfo->Width;
            h = pInfo->Height;
        }

        ptr = MSDK_MIN( MSDK_MIN(pData->R, pData->G), pData->B);
        ptr = ptr + pInfo->CropX + pInfo->CropY * pData->Pitch;

        for(i = 0; i < dest_h; i++)
            memcpy(dest + i*dest_w*4, ptr + i * pData->Pitch, dest_w*4);

        //for(i = 0; i < h; i++)
        //memcpy(dest + i*w*4, ptr + i * pData->Pitch, w*4);
    }
    else if(fourCC == MFX_FOURCC_NV12)
    {
        iYsize = pInfo->CropH * pInfo->CropW;

        for (i = 0; i < pInfo->CropH; i++)
            memcpy(dest + i * (pInfo->CropW), pData->Y + (pInfo->CropY * pData->Pitch + pInfo->CropX) + i * pData->Pitch, pInfo->CropW);

        ipos = iYsize;

        h = pInfo->CropH / 2;
        w = pInfo->CropW;

        for (i = 0; i < h; i++)
            for (j = 0; j < w; j += 2)
                memcpy(dest+ipos++, pData->UV + (pInfo->CropY * pData->Pitch / 2 + pInfo->CropX) + i * pData->Pitch + j, 1);

        for (i = 0; i < h; i++)
            for (j = 1; j < w; j += 2)
                memcpy(dest+ipos++, pData->UV + (pInfo->CropY * pData->Pitch / 2 + pInfo->CropX)+ i * pData->Pitch + j, 1);


    }
    time21 = std::chrono::high_resolution_clock::now();
    diff1 = time21-time11;		  
     
    std::cout<<"\t CPU Copy from GPU memory to system memory:"<< diff1.count()*1000.0 << "ms/frame\t"<<std::endl;

    return sts;
}

cv::Mat createMat(unsigned char *rawData, unsigned int dimX, unsigned int dimY)
{
    // No need to allocate outputMat here
    cv::Mat outputMat;

    // Build headers on your raw data
    cv::Mat channelR(dimY, dimX, CV_8UC1, rawData);
    cv::Mat channelG(dimY, dimX, CV_8UC1, rawData + dimX * dimY);
    cv::Mat channelB(dimY, dimX, CV_8UC1, rawData + 2 * dimX * dimY);

    // Invert channels, 
    // don't copy data, just the matrix headers
    std::vector<cv::Mat> channels{ channelB, channelG, channelR };

    // Create the output matrix
    merge(channels, outputMat);

    return outputMat;
}

bool PrepareData(int nChannelID, mfxFrameSurface1* pSurface, mfxU32 fourCC, mfxU16 w_ie, mfxU16 h_ie, float normalizefactor, size_t *m, int *batchindex)
{
    static size_t frameNo =0; 
    mfxFrameInfo* pInfo = &pSurface->Info;
    mfxFrameData* pData = &pSurface->Data;
    mfxU32 i, j, h, w;
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 iYsize, ipos;

    chrono::high_resolution_clock::time_point time11,time21;
    std::chrono::duration<double> diff1;
    time11 = std::chrono::high_resolution_clock::now();
    // batching
    if(*m < g_batchSize)
    {
        g_pIE[nChannelID]->PreAllocateInputBuffer(*m);

        // storing frames
        g_pIE[nChannelID]->AllocteInputData3(pData->B, pData->G, pData->R, pData->Pitch, w_ie, h_ie,  normalizefactor);

        time2 = std::chrono::high_resolution_clock::now();
        diff = time2-time1;
        pre_stagesum += diff.count()*1000.0;
                
        *m=((*m) +1);
    }

    *batchindex = *m;    // <-- return current batch position

    return true;
}


bool IE_Execute_LowLatency(int nChannelID, size_t *m)
{

    static size_t frameNo =0;
    bool bret;

  
    printf("Batch %d\n",nBatch);
    
    pre_stage_times.push_back(pre_stagesum/g_batchSize);

    pre_stagesum = 0;
            
    //std::cout << "\t. Batching done, now start inferring." << std::endl;

    *m = 0;  // <-- init m value.

    // INFERENCING STAGE:        
    time1 = std::chrono::high_resolution_clock::now();

    bret = g_pIE[nChannelID]->Infer();
    
    time2 = std::chrono::high_resolution_clock::now();
    diff = time2-time1;        
    infer_times.push_back(diff.count()*1000.0/g_batchSize);

    printf("\tinfer:\t\t%5.2f ms/frame\n", diff.count()*1000.0/g_batchSize);

    if (bret == false)
        return false;

    // Read perfomance counters
    if (FLAGS_pc)
        g_pIE[nChannelID]->ReadPerformanceCounters();

    g_pIE[nChannelID]->ReadyForOutputProcessing();
    

    return true;
}

bool IE_Execute(int nChannelID, mfxU16 w_ie, mfxU16 h_ie, float normalizefactor, unsigned char *pIE_InputFrame, int *batchindex)
{
    std::cout << "IE_Execute (" << w_ie << "x" << h_ie << ")" << std::endl;

    static size_t m;
    static size_t frameNo =0;
    bool bret;

    // batching
    if(m < g_batchSize)
    {
        g_pIE[nChannelID]->PreAllocateInputBuffer(m);

#if VERIFY_PURPOSE
        chrono::high_resolution_clock::time_point time11,time21;
        std::chrono::duration<double> diff1;
        time11 = std::chrono::high_resolution_clock::now();

        cv::Mat frame = createMat(pIE_InputFrame, w_ie, h_ie);
        if (!frame.data)
        {
            std::cout << "\t\t. No more data" << std::endl;
            return false;
        }

        time21 = std::chrono::high_resolution_clock::now();
        diff1 = time21-time11;		  
         
        std::cout<<"\t SW CSC:"<< diff1.count()*1000.0 << "ms/frame\t"<<std::endl;

        cv::imshow("rgb frame", frame );
        cv::waitKey(0);

        g_resized[nChannelID][m] = frame;
#endif

        // storing frames
        g_pIE[nChannelID]->AllocteInputData2(pIE_InputFrame, normalizefactor);

        time2 = std::chrono::high_resolution_clock::now();
        diff = time2-time1;
        pre_stagesum += diff.count()*1000.0;
                
        m++;
    }

    *batchindex = m;    // <-- return current batch position

    if(m == g_batchSize)
    {
        printf("Batch %d\n",nBatch);
        
        pre_stage_times.push_back(pre_stagesum/g_batchSize);

        pre_stagesum = 0;
                
        //std::cout << "\t. Batching done, now start inferring." << std::endl;

        m = 0;  // <-- init m value.

        // INFERENCING STAGE:        
        time1 = std::chrono::high_resolution_clock::now();

        bret = g_pIE[nChannelID]->Infer();
        
        time2 = std::chrono::high_resolution_clock::now();
        diff = time2-time1;        
        infer_times.push_back(diff.count()*1000.0/g_batchSize);

        printf("\tinfer:\t\t%5.2f ms/frame\n", diff.count()*1000.0/g_batchSize);

        if (bret == false)
            return false;

        // Read perfomance counters
        if (FLAGS_pc)
            g_pIE[nChannelID]->ReadPerformanceCounters();

        g_pIE[nChannelID]->ReadyForOutputProcessing();

#ifdef VERIFY_PURPOSE 
        // Draw bounding box from result
        IE_DrawBoundingBox(nChannelID);
#endif
    }

    return true;
}

void IE_DrawBoundingBox(int nChannelID)
{
    time1 = std::chrono::high_resolution_clock::now();

    for (size_t mb = 0; mb < g_batchSize; mb++)
    {
        g_pIE[nChannelID]->PreAllocateBoxBuffer(mb);

        if (FLAGS_t.compare("SSD") == 0)    // SSD
        {
            std::vector<BB_INFO> rcBB = g_pIE[nChannelID]->DetectValidBoundingBox(FLAGS_t, FLAGS_thresh);

            for (int i = 0; i < rcBB.size(); i++)
                cv::rectangle(g_resized[nChannelID][mb], Point((int)rcBB[i].xmin, (int)rcBB[i].ymin),   Point((int)rcBB[i].xmax, (int)rcBB[i].ymax), Scalar(0, 55, 255), +1, 4);
        }
        else // YOLO-tiny
        {
            int idetectedobjcount = 0;

            for (int c = 0; c < NUMLABELS; c++)
            {
                std::vector<BB_INFO> rcBB = g_pIE[nChannelID]->DetectValidBoundingBox(FLAGS_t, FLAGS_thresh, c);

                for (int i = 0; i < rcBB.size(); i++)
                {
                    if (rcBB[i].prob <= 0)
                        continue;

                    if(FLAGS_v)
                        std::cout << "\t  - Class: " << c << " (" << g_labels[c].c_str() << "), " << "boxes: " << rcBB.size() << std::endl;

                    cv::rectangle(g_resized[nChannelID][mb], Point(rcBB[i].xmin, rcBB[i].ymin), Point(rcBB[i].xmax, rcBB[i].ymax), Scalar(0, 55, 255), +1, 4);

                    idetectedobjcount++;
                }
            }

            if(FLAGS_v)
                std::cout << "\t. " << idetectedobjcount << " object(s) detected." << std::endl;
        }
    }

    return;
}

void IE_SaveOutput(int nchannleID, bool bSingleImageMode, VideoWriter *vw)
{	
    for (int mb = 0; mb < g_batchSize; mb++)
    {
        if (bSingleImageMode)
            imwrite("out.jpg", g_resized[nchannleID][mb]);
        else{
             vw->write(g_resized[nchannleID][mb]);

        }
    }

    time2 = std::chrono::high_resolution_clock::now();
    diff = time2-time1;
    post_stage_times.push_back(diff.count()*1000.0/g_batchSize);
    
    printf("\tpost-stage:\t%5.2f ms/frame\n",diff.count()*1000.0/g_batchSize);
    fflush(stdout);

    nBatch++;
}
