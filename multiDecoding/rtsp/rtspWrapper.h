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

#ifndef _RTSP_WRAPPER_H_
#define _RTSP_WRAPPER_H_

#include <pthread.h>
#include <list>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


class VaDualPipe;
class RtspWrapper;
struct RtspThreadParam
{
    RtspWrapper *wrapper;
    const char *url;
};

class RtspWrapper
{
public:
    RtspWrapper();
    ~RtspWrapper();
    int Reset();
    int Initialize(const char *url);
    uint32_t Read(void *ptr, uint32_t size);
    uint32_t Send(void *ptr, uint32_t size, bool withHeader = false);
    uint32_t End();
    int rtspRun(const char* url);

protected:
    struct _rtspData
    {
        uint32_t len;
        uint32_t end;
        uint8_t data[1024*1024];
    };
    const uint32_t m_bufferLen = 1024*1024;
    uint32_t m_bufferOffset;
    uint8_t *m_buffer;

    VaDualPipe *m_dpipe;
    pthread_t m_threadid;
    bool m_end;

    char m_watch;
    RtspThreadParam *m_param;

    const char *m_url;
};


#endif
