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

#include "cm_rt.h"
#include <mfxvideo++.h>
#include <mfxstructures.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include <pthread.h> 

extern VADisplay m_va_dpy;

static pthread_mutex_t s_copymutex;

static std::map<VASurfaceID, CmSurface2D *> s_cmsurfacesmap;

static CmDevice *s_device = NULL;

int InitFastcopy(void **ppdevice, void **ppqueue)
{
    // create cm device
    int  result;
    UINT version = 0;
    if(NULL == m_va_dpy)
    {
        std::cout<<"FastCopy: initMDF failed due to m_va_dpy = NULL"<<std::endl;
        return -1;
    }

    pthread_mutex_init(&s_copymutex, NULL);
    pthread_mutex_lock(&s_copymutex);

    if(s_device == NULL)
    {
        result = ::CreateCmDevice(s_device, version, m_va_dpy);
        if (result != CM_SUCCESS ) {
            std::cout<<"CmDevice creation error"<<std::endl;
            return -1;
        }
        if( version < CM_1_0 ){
            std::cout<<" The runtime API version is later than runtime DLL version"<<std::endl;
            return -1;
        }
        std::cout<<"A new MDF device was created success"<<std::endl;
    }

    pthread_mutex_unlock(&s_copymutex);
    *ppdevice = (void *)s_device;

    CmDevice *pDevice = s_device;

    // create cm queue
    CmQueue *pQueue = NULL;
    if (pDevice)
    {
        result = pDevice->CreateQueue( pQueue );
        if (result != CM_SUCCESS ) {
            perror("CM CreateQueue error");
            return -1;
        }
    }
    *ppqueue = (void *)pQueue;

    return 0;
}


int FastCopy(void *device, mfxFrameAllocator *pmfxAllocator, mfxFrameSurface1* pmfxSurface, void *pdata, unsigned int width, unsigned int height)
{
    // check if initialization is needed
    CmDevice *cmdevice = (CmDevice *)device;
    if (cmdevice == NULL)
        return -1;
    
    // Get vaSurfaceID from mfxFrameSurface1
    mfxHDL handle;
    pmfxAllocator->GetHDL(pmfxAllocator->pthis, pmfxSurface->Data.MemId, &(handle));
    VASurfaceID surfId = *(VASurfaceID *)handle;

    // Create CmSurface from this VASurfaceID
    CmSurface2D *cmSurface = NULL;
    cmdevice->CreateSurface2D(surfId, cmSurface);

    // fast copy it out
    CmEvent *event = NULL;
    cmSurface->ReadSurfaceHybridStrides((unsigned char *)pdata, event, width, height); // maybe a memleak for event here

    cmdevice->DestroySurface(cmSurface);
    return 0;
    
}

int FastCopyAsync(void *device, void *queue, mfxFrameAllocator *pmfxAllocator, mfxFrameSurface1* pmfxSurface, void *pdata, unsigned int width, unsigned int height, void **sync_handle)
{
    // check if initialization is needed
    CmDevice *cmdevice = (CmDevice *)device;
    CmQueue *cmqueue = (CmQueue *)queue;
    if (cmdevice == NULL || cmqueue == NULL)
        return -1;
    
    // Get vaSurfaceID from mfxFrameSurface1
    mfxHDL handle;
    pmfxAllocator->GetHDL(pmfxAllocator->pthis, pmfxSurface->Data.MemId, &(handle));
    VASurfaceID surfId = *(VASurfaceID *)handle;

    CmSurface2D *cmSurface = NULL;

    auto ite = s_cmsurfacesmap.find(surfId);
    if (ite == s_cmsurfacesmap.end())
    {
        cmdevice->CreateSurface2D(surfId, cmSurface);
        s_cmsurfacesmap[surfId] = cmSurface;
    }
    else
    {
        cmSurface = s_cmsurfacesmap[surfId];
    }

    // fast copy it out
    CmEvent *event = CM_NO_EVENT;
    cmqueue->EnqueueCopyGPUToCPUFullStride(cmSurface, (unsigned char *)pdata, width, height, 0, event);
    
    *sync_handle = (void *)handle;

    return 0;
    
}

int WaitForFastCopy(void *queue, void *sync_handle)
{
    /*CmEvent *event = (CmEvent *)sync_handle;
    if (event)
    {
        CmQueue *cmqueue = (CmQueue *)queue;
        if (cmqueue == NULL)
            return -1;
        event->WaitForTaskFinished();
        cmqueue->DestroyEvent(event);
    }*/
    VASurfaceID surfId = *(VASurfaceID *)sync_handle;
    vaSyncSurface(m_va_dpy, surfId);
    
    return 0;
}


int ShowNV12_Dump(void *pdata, unsigned int width, unsigned int height)
{
    char filename[256];
    sprintf(filename, "dump_%dx%d.nv12", width, height);
    FILE *fp = fopen(filename, "ab");
    if (fp == NULL)
    {
        printf("open file failed\n");
        return -1;
    }

    // dump Y and UV
    fwrite(pdata, 1, width * height * 3/2, fp);

    fclose(fp);

    return 0;
}

/*int ShowNV12(void *pdata, unsigned int width, unsigned int height)
{
    cv::Mat yuvimg;
    cv::Mat rgbimg;
    rgbimg.create(height, width, CV_8UC3);
    yuvimg.create(height*3/2, width, CV_8UC1);
    yuvimg.data = (unsigned char *)pdata;
    cv::cvtColor(yuvimg, rgbimg, CV_YUV2BGR_NV12);

    cv::imshow( "Display window", rgbimg );  
    cv::waitKey(10);
    return 0;
}*/


