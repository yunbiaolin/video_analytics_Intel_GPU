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
#include "../../memories/OCV_RGB_Frame_Memory.h"

using namespace std;
using namespace cv;

namespace VAITFramework
{

class OCV_reader: public Worker
{
public:
    OCV_reader(std::string infile1,OCV_RGB_Frame_Memory *f,int b)
    {
            infile=infile1;
            frames=f;
            mb=b;        
    }

    virtual void Init() {
      //open video capture
      cap= new VideoCapture(infile.c_str());

      input_width   = cap->get(CV_CAP_PROP_FRAME_WIDTH);
      input_height  = cap->get(CV_CAP_PROP_FRAME_HEIGHT);
    }
    virtual int Execute();

private:
    OCV_RGB_Frame_Memory *frames;

    VideoCapture *cap;
    std::string infile;
    int mb;
    size_t input_width;
    size_t input_height;
};

}
