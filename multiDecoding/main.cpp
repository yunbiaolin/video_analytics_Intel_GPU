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

/*
// brief The entry point for the Inference Engine object_detection sample application
*/
#include <iostream>
#include <thread>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <vector>
#include <queue>
#include <signal.h>
#include <sys/time.h>

#include <pthread.h>  
#include <unistd.h>  
#include <semaphore.h>
#include <chrono>

#include <string>
#include <sstream>
// =================================================================
// Intel Media SDK
// =================================================================
#include <mfxvideo++.h>
#include <mfxstructures.h>
#include <unistd.h>
#include "common_utils.h"
#include "image_queue.hpp"

using namespace std;

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

#define FASTCOPY_DECODED_FRAME 0
#define SHOW_FASTCOPY_FRAME 0

#define VP_OUTPUT_WIDTH 1000
#define VP_OUTPUT_HEIGHT 500

using namespace std;

static pthread_mutex_t s_countermutex;
static pthread_mutex_t s_decodermutex_odd;
static pthread_mutex_t s_decodermutex_even;
static pthread_mutex_t s_decodermutex_third;
static pthread_mutex_t s_decodermutex_forth;
static pthread_mutex_t s_image_queue_mutex;
ImageQueue image_queue(CLIENT);
void dummy_scheduler_func()
{
    return;
}
static int totalFrames = 0;

int FastCopyAsync(void *device, void *queue, mfxFrameAllocator *pmfxAllocator, mfxFrameSurface1* pmfxSurface, void *pdata, unsigned int width, unsigned int height, void **sync_handle);
int WaitForFastCopy(void *queue, void *sync_handle);
int ShowNV12_Dump(void *pdata, unsigned int width, unsigned int height);
int InitFastcopy(void **ppdevice, void **ppqueue);

struct DecThreadConfig
{
public:
    DecThreadConfig()
    {
    };
    FILE *f_i;
    MFXVideoDECODE *pmfxDEC;
    MFXVideoENCODE *pmfxENC;
    mfxU16 nDecSurfNum;
    mfxU16 nVPP_In_SurfNum;
    mfxU16 nVPP_Out_SurfNum;
    mfxBitstream *pmfxBS;
    mfxFrameSurface1** pmfxDecSurfaces;
    mfxFrameSurface1** pmfxVPP_In_Surfaces;
    mfxFrameSurface1** pmfxVPP_Out_Surfaces;
    MFXVideoSession *  pmfxSession;
    MFXVideoSession *  pmfxSession_Dec_SW;
    mfxFrameAllocator *pmfxAllocator;
    MFXVideoVPP       *pmfxVPP;

    int decOutputFile;
    int nChannel;
    int nFPS;
    bool bStartCount;
    int nFrameProcessed;
};

std::vector<DecThreadConfig *>   vpDecThradConfig;

int grunning = true;

static int batch_num = 0;
static bool need_vp = true;
static int vp_ratio = 5;
static bool jpeg_encoder = false;
static bool write_jpeg = false;
static bool sw_decoder = false;
static bool send_jpeg = false;

// Performance information
int total_fps;

mfxStatus WriteRawFrameToMemory(mfxFrameSurface1* pSurface, int dest_w, int dest_h, unsigned char* dest, mfxU32 fourCC);

void App_ShowUsage(void)
{
    printf("Usage: multichannel_decode -i input.264 [-r vp_ratio] [-c channel_number] [-b batch_num]\n");
    printf("           -r vp_ratio: the ratio of decoded frames to vp frames, e.g., -r 2 means doing vp every other frame\n");
    printf("                        5 by default\n");
    printf("                        0 means no vp\n");
    printf("           -c channel_number: number of decoding channels, default value is 1\n");
    printf("           -b batch_num: number of frames in batch decoding\n");
    printf("                         0 by default, meaning every decoding thread runs freely\n");
    printf("           -j: enable jpeg encoder\n");
    printf("           -w: dump jpeg encoder output\n");
    printf("           -s: use software deocder\n");
}

// handle to end the process
void sigint_hanlder(int s)
{
    grunning = false;
}

// ================= Decoding Thread =======
void *DecodeThreadFunc(void *arg)
{
    struct DecThreadConfig *pDecConfig = NULL;
    int nFrame = 0;
    mfxBitstream mfxBS;
    mfxStatus sts = MFX_ERR_NONE;
    mfxSyncPoint syncpDec;
    mfxSyncPoint syncpVPP;
    mfxSyncPoint syncpENC;

    struct timeval pipeStartTs = {0};

    int nIndexDec     = 0;
    int nIndexVPP_In  = 0;
    int nIndexVPP_In2 = 0;
    int nIndexVPP_Out = 0;
    bool bNeedMore    = false;
    
    timeval lasttv = {0};
    timeval curtv = {0};
#if FASTCOPY_DECODED_FRAME
    // init fast copy context
    void *fcDevice = NULL;
    void *fcQueue = NULL;
    int ret = InitFastcopy(&fcDevice, &fcQueue);
    if (ret != 0)
    {
        std::cout << std::endl <<"Failed to initialize fastcopy context" << std::endl;
        return NULL;  
    }
#endif 

    pDecConfig= (DecThreadConfig *) arg;
    if( NULL == pDecConfig)
    {
        std::cout << std::endl << "Failed Decode Thread Configuration" << std::endl;
        return NULL;
    }
    std::cout << std::endl <<">channel("<<pDecConfig->nChannel<<") Initialized " << std::endl;
    
    pthread_mutex_t *decodermutex = NULL;
    if (pDecConfig->nChannel%4 == 0)
        decodermutex = &s_decodermutex_even;
    else if (pDecConfig->nChannel%4 == 1)
        decodermutex = &s_decodermutex_odd;
    else if (pDecConfig->nChannel%4 == 2)
        decodermutex = &s_decodermutex_third;
    else
        decodermutex = &s_decodermutex_forth;
    
    pthread_mutex_lock(decodermutex);

    while ((MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts))
    {
        if (grunning == false) { break;};
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            usleep(1000); // Wait if device is busy, then repeat the same call to DecodeFrameAsync
        }
    
        if (MFX_ERR_MORE_DATA == sts)
        {   
            sts = ReadBitStreamData(pDecConfig->pmfxBS, pDecConfig->f_i); // Read more data into input bit stream
            if(sts != 0){
              fseek(pDecConfig->f_i,0, SEEK_SET);
              sts = ReadBitStreamData(pDecConfig->pmfxBS, pDecConfig->f_i);
            }
            MSDK_BREAK_ON_ERROR(sts);
        }
        
        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
        {
recheck21:

            nIndexDec =GetFreeSurfaceIndex(pDecConfig->pmfxDecSurfaces, pDecConfig->nDecSurfNum);
            if(nIndexDec == MFX_ERR_NOT_FOUND){
               //std::cout << std::endl<< ">channel("<<pDecConfig->nChannel<<") >> Not able to find an avaialbe decoding recon surface" << std::endl;
               usleep(10000);
               goto recheck21;
            }
        }

        if(bNeedMore == false)
        {
            if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
            {   
recheck:
                nIndexVPP_In = GetFreeSurfaceIndex(pDecConfig->pmfxVPP_In_Surfaces, pDecConfig->nVPP_In_SurfNum); // Find free frame surface
         
                if(nIndexVPP_In == MFX_ERR_NOT_FOUND){
                   std::cout << ">channel("<<pDecConfig->nChannel<<") >> Not able to find an avaialbe VPP input surface" << std::endl;
                     
                   goto recheck;
                }
            }
        }

        // Decode a frame asychronously (returns immediately)
        //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
        sts = pDecConfig->pmfxDEC->DecodeFrameAsync(pDecConfig->pmfxBS, pDecConfig->pmfxDecSurfaces[nIndexDec], &(pDecConfig->pmfxVPP_In_Surfaces[nIndexVPP_In]), &syncpDec);

        // Ignore warnings if output is available,eat the Decode surface
        // if no output and no action required just repecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpDec)
        {
            bNeedMore = false;
            sts = MFX_ERR_NONE;
        }
        else if(MFX_ERR_MORE_DATA == sts)
        {
            bNeedMore = true;
        }
        else if(MFX_ERR_MORE_SURFACE == sts)
        {
            bNeedMore = true;
        }
        //else

        if (MFX_ERR_NONE == sts )
        {
            pDecConfig->nFrameProcessed ++;

            if (sw_decoder)
            {
                sts = pDecConfig->pmfxSession_Dec_SW->SyncOperation(syncpDec, 60000);
                MSDK_BREAK_ON_ERROR(sts);
            }

            if (need_vp && (nFrame % vp_ratio) == 0)
            {
recheck2:
               nIndexVPP_Out = GetFreeSurfaceIndex(pDecConfig->pmfxVPP_Out_Surfaces, pDecConfig->nVPP_Out_SurfNum); // Find free frame surface

               if(nIndexVPP_Out == MFX_ERR_NOT_FOUND){
                   std::cout << ">channel("<<pDecConfig->nChannel<<") >> Not able to find an avaialbe VPP output surface" << std::endl;
                 //  return NULL;
                   goto recheck2;
               }
              // std::cerr << "Get free vp index "<<nIndexVPP_Out << std::endl;
                              
               for (;;)
               {
                   // Process a frame asychronously (returns immediately) 
                   sts = pDecConfig->pmfxVPP->RunFrameVPPAsync(pDecConfig->pmfxVPP_In_Surfaces[nIndexVPP_In], pDecConfig->pmfxVPP_Out_Surfaces[nIndexVPP_Out], NULL, &syncpVPP);

#if FASTCOPY_DECODED_FRAME
                   mfxFrameSurface1 *surf = pDecConfig->pmfxVPP_In_Surfaces[nIndexVPP_In];
                   unsigned int width = surf->Info.Width;
                   unsigned int height = surf->Info.Height;
                   //fast copy supports any resolution without alignment restrictions
                   //unsigned int width = 601;
                   //unsigned int height = 299;
                   // Copy the decoded output frame to system memory
                   void *pData = malloc(width*height*3/2);
                   //FastCopy(pDecConfig->pmfxAllocator, surf, pData, width, height);
                   void *sync_handle = NULL;
                   FastCopyAsync(fcDevice, fcQueue, pDecConfig->pmfxAllocator, surf, pData, width, height, &sync_handle);
                   WaitForFastCopy(fcQueue, sync_handle);
#if SHOW_FASTCOPY_FRAME
                   ShowNV12_Dump(pData, width, height);
#endif
                   free(pData);
#endif

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
                   else{
                       break; // not a warning
                   }
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
                
  
                 if (MFX_ERR_NONE == sts)
                 { 
                    if(jpeg_encoder )
                    {
                        /* output bitstream allcoation */
                        mfxBitstream mfxBS;
                        memset(&mfxBS, 0, sizeof(mfxBS));
                        mfxBS.MaxLength = VP_OUTPUT_WIDTH*VP_OUTPUT_HEIGHT*4;
                        mfxBS.Data = new mfxU8[mfxBS.MaxLength];
                        if( NULL == mfxBS.Data)
                        {
                            std::cout << std::endl << "Failed allocate output bitstream" << std::endl;
                            return NULL;
                        }

                        sts = pDecConfig->pmfxENC->EncodeFrameAsync(NULL, pDecConfig->pmfxVPP_Out_Surfaces[nIndexVPP_Out], &mfxBS, &syncpENC);

                        if (MFX_ERR_NONE < sts && !syncpENC) // repeat the call if warning and no output
                        {
                            if (MFX_WRN_DEVICE_BUSY == sts)
                            {
                                std::cout << std::endl << "> warning: MFX_WRN_DEVICE_BUSY" << std::endl;
                                usleep(1000); // wait if device is busy
                            }
                        }
                        else if (MFX_ERR_NONE < sts && syncpENC)
                        {
                            sts = MFX_ERR_NONE; // ignore warnings if output is available
                        }

                        if (MFX_ERR_NONE == sts)
                        {
                            sts = pDecConfig->pmfxSession->SyncOperation(syncpENC, 60000); /* Encoder's Sync */
                            //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
                            if (write_jpeg)
                            {
                                stringstream jpeg_output_file;
                                jpeg_output_file << "output_" << pDecConfig->nChannel << "_" << nFrame << ".jpeg";

                                if(FILE *m_dump_bitstream_1 = std::fopen(jpeg_output_file.str().c_str(), "wb"))
                                {
                                    std::fwrite(mfxBS.Data, sizeof(char),mfxBS.DataLength, m_dump_bitstream_1);
                                    std::fclose(m_dump_bitstream_1);
                                }
                            }
                            if (send_jpeg)
                            {
                                StreamImage image1(1, "test_image1", VP_OUTPUT_WIDTH, VP_OUTPUT_HEIGHT, 100/*quality*/);
                                image1.set_image(mfxBS.Data, mfxBS.DataLength);
                                pthread_mutex_lock(&s_image_queue_mutex);
                                image_queue.push(image1);
                                pthread_mutex_unlock(&s_image_queue_mutex);
                            }

                            delete mfxBS.Data;
                        }
                    }
                    else
                    {
                        sts = pDecConfig->pmfxSession->SyncOperation(syncpVPP, 60000); // Synchronize. Wait until decoded frame is ready
                    }
                 }
                
            }// nframe % vp_ratio
            pthread_mutex_lock(&s_countermutex);
            totalFrames ++;
            pthread_mutex_unlock(&s_countermutex);
            nFrame++;
            if (batch_num > 0 && nFrame%batch_num == 0)
            {
                pthread_mutex_unlock(decodermutex);
                gettimeofday(&curtv, NULL);
                long long duration = 33000*batch_num + lasttv.tv_sec * 1000000 + lasttv.tv_usec
                                    - curtv.tv_sec * 1000000 - curtv.tv_usec;
                if (duration < 0 )
                {
                    lasttv.tv_sec = curtv.tv_sec;
                    lasttv.tv_usec = curtv.tv_usec;
                }
                else
                {
                    timeval temp;
                    temp.tv_sec = 0;
                    temp.tv_usec = 33000*batch_num;
                    timeradd(&lasttv, &temp, &lasttv);
                    if (duration > 1000)  // < 1ms, just pass
                    {
                        usleep(duration);
                    }
                }
                
                //usleep(10000*batch_num);
                pthread_mutex_lock(decodermutex);
            }
            
        }  
    }

    std::cout << std::endl<< "channel("<<pDecConfig->nChannel<<") pDecoding is done\r\n"<< std::endl;

    return (void *)0;

}

#define NUM_OF_CHANNELS 60 // maximum channel number it supports

int main(int argc, char *argv[])
{
    // opt
    int c;
    int channelNum = 1;
    std::string input_filename;
    while ((c = getopt (argc, argv, "c:i:b:r:jwhs")) != -1)
    {
    switch (c)
      {
      case 'c':
        channelNum = atoi(optarg);
        break;
      case 'b':
        batch_num = atoi(optarg);
        break;
      case 'i':
        input_filename = optarg;
        break;
      case 'r':
        vp_ratio = atoi(optarg);
        need_vp = (vp_ratio != 0);
        break;
      case 'j':
        jpeg_encoder = true;
        break;
      case 'w':
        write_jpeg = true;
        break;        
      case 's':
        sw_decoder = true;
        break;
      case 'h':
        App_ShowUsage();
        exit(0);
      default:
        abort ();
      }
    }
    
    if (input_filename.empty())
    {
        printf("Missing input file name!!!!!!!\n");
        App_ShowUsage();
        exit(0);
    }

    std::vector<pthread_t>    vDecThreads;
    // =================================================================


// =================================================================
// Intel Media SDK
//
// if input file has ".264" extension, let assume we want to enable
// media sdk to decode video elementary stream here.

    std::cout << "> Use [Intel Media SDK]." << std::endl;

    bool no_more_data = false;
    
    pthread_mutex_init(&s_countermutex, NULL);
    pthread_mutex_init(&s_decodermutex_odd, NULL);
    pthread_mutex_init(&s_decodermutex_even, NULL);
    pthread_mutex_init(&s_decodermutex_third, NULL);
    pthread_mutex_init(&s_decodermutex_forth, NULL);
    pthread_mutex_init(&s_image_queue_mutex, NULL);
    image_queue.set_batch_size(2);
    image_queue.reg_scheduler_fun(dummy_scheduler_func);

    // =================================================================
    // Intel Media SDK
    //
    // Use Intel Media SDK for decoding and VPP (resizing)

    mfxStatus sts;
    mfxIMPL impl; 				  // SDK implementation type: hardware accelerator?, software? or else
    mfxVersion ver;				  // media sdk version

    // mfxSession and Allocator can be shared globally.
    MFXVideoSession   mfxSession[NUM_OF_CHANNELS];
    MFXVideoSession   mfxSession_decSw;
    mfxFrameAllocator mfxAllocator[NUM_OF_CHANNELS];	 

    FILE *f_i[NUM_OF_CHANNELS];
    mfxBitstream mfxBS[NUM_OF_CHANNELS];
    mfxFrameSurface1** pmfxDecSurfaces[NUM_OF_CHANNELS];
    mfxFrameSurface1** pmfxVPP_In_Surfaces[NUM_OF_CHANNELS];
    mfxFrameSurface1** pmfxVPP_Out_Surfaces[NUM_OF_CHANNELS];

    std::vector<MFXVideoDECODE *>    vpMFXDec;
    std::vector<MFXVideoVPP *>       vpMFXVpp;
    std::vector<MFXVideoENCODE *>    vpMFXEncode;
    
    int last_counter = 0;

    std::cout << "channels of stream:"<<channelNum << std::endl;

    for(int nLoop=0; nLoop< channelNum; nLoop++)
    {
        // =================================================================
         // Intel Media SDK
        std::cout << "\t. Open input file: " << input_filename << std::endl;
       
        f_i[nLoop] = fopen(input_filename.c_str(), "rb");
        if(f_i[nLoop] ==0 ){
           std::cout << "\t.Failed to  open input file: " << input_filename << std::endl;
           goto exit_here;
        }

        std::cout << std::endl << "> Declare Intel Media SDK video session and Init." << std::endl;
        sts = MFX_ERR_NONE;
        impl = MFX_IMPL_AUTO_ANY;
        ver = { {0, 1} };
        sts = Initialize(impl, ver, &mfxSession[nLoop], &mfxAllocator[nLoop]);
        if( sts != MFX_ERR_NONE){
            std::cout << "\t. Failed to initialize decode session" << std::endl;
            goto exit_here;
        }

        if (sw_decoder)
        {
            impl = MFX_IMPL_SOFTWARE;
            ver = { {0, 1} };
            sts = Initialize(impl, ver, &mfxSession_decSw, &mfxAllocator[nLoop]);
            if( sts != MFX_ERR_NONE){
                std::cout << "\t. Failed to initialize SW decode session" << std::endl;
                goto exit_here;
            }
        }

        MFXVideoDECODE *pmfxDEC = NULL;
        if (sw_decoder)
            pmfxDEC = new MFXVideoDECODE(mfxSession_decSw);
        else
            pmfxDEC = new MFXVideoDECODE(mfxSession[nLoop]);
        vpMFXDec.push_back(pmfxDEC);
        MFXVideoVPP    *pmfxVPP = new MFXVideoVPP(mfxSession[nLoop]);
        vpMFXVpp.push_back(pmfxVPP);
        MFXVideoENCODE *pmfxENC = new MFXVideoENCODE(mfxSession[nLoop]);
        vpMFXEncode.push_back(pmfxENC);
        if( pmfxDEC == NULL || pmfxVPP == NULL || pmfxENC == NULL){
            std::cout << "\t. Failed to initialize decode session" << std::endl;
            goto exit_here;
        }

        // [DECODER]
        // Initialize decode video parameters
        // - In this example we are decoding an AVC (H.264) stream
        // - For simplistic memory management, system memory surfaces are used to store the decoded frames
        std::cout << "\t. Start preparing video parameters for decoding." << std::endl;
        mfxExtDecVideoProcessing DecoderPostProcessing;
        mfxVideoParam DecParams;
        mfxVideoParam EncParams;
        bool VD_LP_resize = false; // default value is for VDBOX+SFC case
   
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
        mfxU16 nDecSurfNum= 0;

        mfxVideoParam VPPParams;
        mfxExtVPPScaling scalingConfig;
        memset(&VPPParams, 0, sizeof(VPPParams));
        memset(&scalingConfig, 0, sizeof(mfxExtVPPScaling));

        mfxExtBuffer *pExtBuf[1];
        mfxExtBuffer *pExtBufDec[1];
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
        VPPParams.vpp.Out.CropW         = VP_OUTPUT_WIDTH; // SSD: 300, YOLO-tiny: 448
        VPPParams.vpp.Out.CropH         = VP_OUTPUT_HEIGHT;    // SSD: 300, YOLO-tiny: 448
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
        
        /* If enabled JPEG encoding enabled:
         * VPP works for resize only
         * CSC NV12->RGBP do not required (as no inference)
         * And inference disabled
         * */
        if (jpeg_encoder)
        {
            VPPParams.vpp.Out.FourCC        = MFX_FOURCC_NV12;
            VPPParams.vpp.Out.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
        }

        /* LP size */
        if (VD_LP_resize)
        {
            if ( (MFX_CODEC_AVC == DecParams.mfx.CodecId) && /* Only for AVC */
                 (MFX_PICSTRUCT_PROGRESSIVE == DecParams.mfx.FrameInfo.PicStruct)) /* ...And only for progressive!*/
            {   /* it is possible to use decoder's post-processing */
                DecoderPostProcessing.Header.BufferId    = MFX_EXTBUFF_DEC_VIDEO_PROCESSING;
                DecoderPostProcessing.Header.BufferSz    = sizeof(mfxExtDecVideoProcessing);
                DecoderPostProcessing.In.CropX = 0;
                DecoderPostProcessing.In.CropY = 0;
                DecoderPostProcessing.In.CropW = DecParams.mfx.FrameInfo.CropW;
                DecoderPostProcessing.In.CropH = DecParams.mfx.FrameInfo.CropH;

                DecoderPostProcessing.Out.FourCC = DecParams.mfx.FrameInfo.FourCC;
                DecoderPostProcessing.Out.ChromaFormat = DecParams.mfx.FrameInfo.ChromaFormat;
                DecoderPostProcessing.Out.Width = MSDK_ALIGN16(VPPParams.vpp.Out.Width);
                DecoderPostProcessing.Out.Height = MSDK_ALIGN16(VPPParams.vpp.Out.Height);
                DecoderPostProcessing.Out.CropX = 0;
                DecoderPostProcessing.Out.CropY = 0;
                DecoderPostProcessing.Out.CropW = VPPParams.vpp.Out.CropW;
                DecoderPostProcessing.Out.CropH = VPPParams.vpp.Out.CropH;
                DecParams.ExtParam = (mfxExtBuffer **)pExtBufDec;
                DecParams.ExtParam[0] = (mfxExtBuffer*)&(DecoderPostProcessing);
                DecParams.NumExtParam = 1;
                std::cout << "\t.Decoder's post-processing is used for resizing\n"<< std::endl;

                /* need to correct VPP params: re-size done after decoding
                 * So, VPP for CSC NV12->RGBP only */
                VPPParams.vpp.In.Width = DecoderPostProcessing.Out.Width;
                VPPParams.vpp.In.Height = DecoderPostProcessing.Out.Height;
                VPPParams.vpp.In.CropW = VPPParams.vpp.Out.CropW;
                VPPParams.vpp.In.CropH = VPPParams.vpp.Out.CropH;
                /* scaling is off (it was configured via extended buffer)*/
                VPPParams.NumExtParam = 0;
                VPPParams.ExtParam = NULL;
            }
        }
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
        
        nDecSurfNum = DecResponse.NumFrameActual;

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
        if(MFX_ERR_NONE > sts)
        {
            MSDK_PRINT_RET_MSG(sts);
            return 1;
        }

        mfxU16 nVPP_In_SurfNum = VPP_In_Response.NumFrameActual * 2;

        std::cout << "\t\t. VPP In Surface Actual Number: " << nVPP_In_SurfNum << std::endl;

        // this surface will contain decoded frame
        pmfxVPP_In_Surfaces[nLoop] = new mfxFrameSurface1 * [nVPP_In_SurfNum];

        if(!pmfxVPP_In_Surfaces[nLoop])
        {
            MSDK_PRINT_RET_MSG(MFX_ERR_MEMORY_ALLOC);            
            return 1;
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
            return 1;
        }

        mfxU16 nVPP_Out_SurfNum = VPP_Out_Response.NumFrameActual;

        std::cout << "\t\t. VPP Out Surface Actual Number: " << nVPP_Out_SurfNum << std::endl;

        // this surface will contain resized frame (video processed)
        pmfxVPP_Out_Surfaces[nLoop] = new mfxFrameSurface1 * [nVPP_Out_SurfNum];

        if(!pmfxVPP_Out_Surfaces[nLoop])
        {
            MSDK_PRINT_RET_MSG(MFX_ERR_MEMORY_ALLOC);            
            return 1;
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
        for (mfxU16 i = 0; i < nDecSurfNum; i++)
        {
             std::cout<<"dec:handle (%d): "<<pmfxDecSurfaces[nLoop][i]<<std::endl;
        }


        for (mfxU16 i = 0; i < nVPP_In_SurfNum; i++)
        {
             std::cout<<"vpp:handle: "<<pmfxVPP_In_Surfaces[nLoop][i]<<std::endl;
        }
        
        // [ENCODER]
        if (jpeg_encoder)
        {
            memset(&EncParams, 0, sizeof(EncParams));
            EncParams.mfx.CodecId = MFX_CODEC_JPEG;
            EncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;

            // frame info parameters
            EncParams.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
            EncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
            EncParams.mfx.FrameInfo.PicStruct    = MFX_PICSTRUCT_PROGRESSIVE;

            EncParams.mfx.FrameInfo.Width  = VPPParams.vpp.Out.Width;
            EncParams.mfx.FrameInfo.Height = VPPParams.vpp.Out.Height;

            EncParams.mfx.FrameInfo.CropX = VPPParams.vpp.Out.CropX;
            EncParams.mfx.FrameInfo.CropY = VPPParams.vpp.Out.CropY;
            EncParams.mfx.FrameInfo.CropW = VPPParams.vpp.Out.CropW;
            EncParams.mfx.FrameInfo.CropH = VPPParams.vpp.Out.CropH;

            EncParams.mfx.Interleaved = 1;
            EncParams.mfx.Quality = 95;
            EncParams.mfx.RestartInterval = 0;
            EncParams.mfx.CodecProfile = MFX_PROFILE_JPEG_BASELINE;
            EncParams.mfx.NumThread = 1;
            memset(&EncParams.mfx.reserved5,0,sizeof(EncParams.mfx.reserved5));

            EncParams.AsyncDepth = 1;

            mfxFrameAllocRequest EncRequest = { 0 };
            sts = pmfxENC->QueryIOSurf(&EncParams, &EncRequest);
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);

            // Initialize the Media SDK encoder
            std::cout << "\t. Init Intel Media SDK Encoder" << std::endl;

            sts = pmfxENC->Init(&EncParams);
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);

            if(MFX_ERR_NONE > sts)
            {
                MSDK_PRINT_RET_MSG(sts);
                goto exit_here;
            }
        }

        //int decOutputFile = open("./dump.nv12", O_APPEND | O_CREAT, S_IRWXO);

        DecThreadConfig *pDecThreadConfig = new DecThreadConfig();
        memset(pDecThreadConfig, 0, sizeof(DecThreadConfig));
        pDecThreadConfig->f_i                    = f_i[nLoop];
        pDecThreadConfig->pmfxDEC                = pmfxDEC;
        pDecThreadConfig->pmfxVPP                = pmfxVPP;
        pDecThreadConfig->pmfxENC                = pmfxENC;
        pDecThreadConfig->nDecSurfNum            = nDecSurfNum;
        pDecThreadConfig->nVPP_In_SurfNum        = nVPP_In_SurfNum;
        pDecThreadConfig->nVPP_Out_SurfNum       = nVPP_Out_SurfNum;
        pDecThreadConfig->pmfxDecSurfaces        = pmfxDecSurfaces[nLoop];
        pDecThreadConfig->pmfxVPP_In_Surfaces    = pmfxVPP_In_Surfaces[nLoop];
        pDecThreadConfig->pmfxVPP_Out_Surfaces   = pmfxVPP_Out_Surfaces[nLoop];
        pDecThreadConfig->pmfxBS                 = &mfxBS[nLoop];
        pDecThreadConfig->pmfxSession            = &mfxSession[nLoop];
        if (sw_decoder)
            pDecThreadConfig->pmfxSession_Dec_SW = &mfxSession_decSw;
        else
            pDecThreadConfig->pmfxSession_Dec_SW = NULL;
        pDecThreadConfig->pmfxAllocator          = &mfxAllocator[nLoop];
        pDecThreadConfig->nChannel               = nLoop;
        pDecThreadConfig->nFPS                   = 1;//30/pDecThreadConfig->nFPS;
        pDecThreadConfig->bStartCount            = false;
        pDecThreadConfig->nFrameProcessed        = 0;
        vpDecThradConfig.push_back(pDecThreadConfig);

        pthread_t threadid;
        pthread_create(&threadid, NULL, DecodeThreadFunc, (void *)(pDecThreadConfig));

        vDecThreads.push_back(threadid) ; 

    }

    while (1)
    {
        usleep(1000000);
        printf("Total FPS is %d\n", totalFrames - last_counter);
        printf("Average FPS is %.3f\n", (double)(totalFrames - last_counter)/channelNum);
        last_counter = totalFrames;
    }
    
    grunning=false;
    
    for (auto& th : vDecThreads) {
        pthread_join(th,NULL);
    }
 
    std::cout<<" All thread is termined " <<std::endl;

exit_here:

    for(int nLoop=0; nLoop< channelNum; nLoop++)
    {
        fclose(f_i[nLoop]);
        //fclose(f_o);

        MSDK_SAFE_DELETE_ARRAY(pmfxDecSurfaces[nLoop]);
        MSDK_SAFE_DELETE_ARRAY(pmfxVPP_In_Surfaces[nLoop]);
        MSDK_SAFE_DELETE_ARRAY(pmfxVPP_Out_Surfaces[nLoop]);
        MSDK_SAFE_DELETE_ARRAY(mfxBS[nLoop].Data);
    }

    std::cout << "> Complete execution !";

    fflush(stdout);
    return 0;
}


mfxStatus WriteRawFrameToMemory(mfxFrameSurface1* pSurface, int dest_w, int dest_h, unsigned char* dest, mfxU32 fourCC)
{
    mfxFrameInfo* pInfo = &pSurface->Info;
    mfxFrameData* pData = &pSurface->Data;
    mfxU32 i, j, h, w;
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 iYsize, ipos;


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

    return sts;
}


