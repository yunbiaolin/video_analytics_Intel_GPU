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
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "../../include/worker.h"
#include "../../include/types.h"
#include "../../memories/SSD_ROI_memory.h"
#include "../../memories/OCV_RGB_Frame_Memory.h"

using namespace std;
using namespace cv;

namespace VAITFramework
{

class OCV_render: public Worker
{
public:
    OCV_render(OCV_RGB_Frame_Memory *f, SSD_ROI_Memory *ROImemory, unsigned int b)
    {
        frames=f;
        ROIs=ROImemory;
        batchnum=b;
    }
    virtual void Init() {}
    virtual int Execute();

private:
    OCV_RGB_Frame_Memory *frames;
    SSD_ROI_Memory *ROIs;
    unsigned int batchnum;
};

}
