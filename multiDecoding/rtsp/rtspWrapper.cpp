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

#include "rtspWrapper.h"
#include "dualpipe.h"
#include <malloc.h>

int rtsp_run(const char* url, RtspWrapper *wrapper);

void *RtspThreadFunc(void *arg)
{
    RtspThreadParam *param = (RtspThreadParam *)arg;
    //rtsp_run(param->url, param->wrapper);
    param->wrapper->rtspRun(param->url);
}

RtspWrapper::RtspWrapper():
    m_bufferOffset(0),
    m_buffer(nullptr),
    m_dpipe(nullptr),
    m_param(nullptr),
    m_end(false),
    m_watch(0),
    m_url(nullptr)
{
}

RtspWrapper::~RtspWrapper()
{
    delete m_dpipe;
    free(m_buffer);
    delete m_param;
    m_watch = 1;
    pthread_join(m_threadid, NULL);
}

int RtspWrapper::Reset()
{
    delete m_dpipe;
    free(m_buffer);
    delete m_param;
    m_watch = 1;
    pthread_join(m_threadid, NULL);
    return Initialize(m_url);
}

int RtspWrapper::Initialize(const char *url)
{
    printf("In Init\n");
    m_buffer = (uint8_t *)malloc(1024*1024);
    m_dpipe = new VaDualPipe;

    m_end = false;
    m_dpipe->Initialize(12, sizeof(_rtspData));
    m_param = new RtspThreadParam;
    m_param->url = url;
    m_param->wrapper = this;
    
    pthread_create(&m_threadid, NULL, RtspThreadFunc, (void *)(m_param));
    m_url = url;
    printf("out init\n");
    return 0;
}


uint32_t RtspWrapper::Read(void *ptr, uint32_t size)
{
    uint32_t copiedSize = 0;
    if (size == 0)
    {
        return copiedSize;
    }

    if (m_end)
    {
        m_watch = 1;
        return 0;
    }

    // copy the remaining data in buffer
    if (m_bufferOffset > size)
    {
        memcpy(ptr, m_buffer, size);
        copiedSize += size;

        uint32_t remainingSize = m_bufferOffset - size;
        memmove(m_buffer, m_buffer + size, remainingSize);
        m_bufferOffset = remainingSize;
        return copiedSize;
    }

    if (m_bufferOffset > 0)
    {
        memcpy(ptr, m_buffer, m_bufferOffset);
        copiedSize += m_bufferOffset;
        m_bufferOffset = 0;
    }

    while (copiedSize < size)
    {
        _rtspData *pac = (_rtspData*)m_dpipe->Load(NULL);
        if (pac->end)
        {
            m_end = true;
            break;
        }
        uint8_t *data = pac->data;
        uint32_t len = pac->len;
        if (len > (size - copiedSize))
        {
            uint32_t copyLen = size - copiedSize;
            uint32_t remainingLen = len - copyLen;
            memcpy(ptr + copiedSize, data, copyLen);
            copiedSize += copyLen;
            memcpy(m_buffer + m_bufferOffset, data+copyLen, remainingLen);
            m_bufferOffset += remainingLen;
        }
        else
        {
            memcpy(ptr + copiedSize, data, len);
            copiedSize += len;
        }
        m_dpipe->Put(pac);
    }

    return copiedSize;
}

uint32_t RtspWrapper::Send(void *ptr, uint32_t size, bool withHeader)
{
    uint32_t sentSize = 0;
    if (withHeader)
    {
        _rtspData *pac = (_rtspData *)m_dpipe->Get();
        pac->data[0] = 0;
        pac->data[1] = 0;
        pac->data[2] = 0;
        pac->data[3] = 1;
        uint32_t copyLen = (size < (1024*1024 -4))?size:(1024*1024-4);
        memcpy(pac->data + 4, ptr, copyLen);
        pac->len = copyLen + 4;
        pac->end = 0;
        sentSize += copyLen;
        m_dpipe->Store(pac);
    }
    while (sentSize < size)
    {
        uint32_t remainingSize = size - sentSize;
        _rtspData *pac = (_rtspData *)m_dpipe->Get();
        uint32_t copyLen = (remainingSize < 1024*1024)?remainingSize:1024*1024;
        memcpy(pac->data, ptr, copyLen);
        pac->len = copyLen;
        pac->end = 0;
        sentSize += copyLen;
        m_dpipe->Store(pac);
    }
    return sentSize;
}

uint32_t RtspWrapper::End()
{
    _rtspData *pac = (_rtspData *)m_dpipe->Get();
    pac->len = 0;
    pac->end = 1;
    m_dpipe->Store(pac);

    return 0;
}


