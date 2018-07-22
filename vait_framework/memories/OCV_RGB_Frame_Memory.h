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

#pragma once
#include "../include/memory.h"
#include <vector>
#include <opencv2/opencv.hpp>

using namespace cv;

namespace VAITFramework {
    class OCV_RGB_Frame_Memory : public Memory {
    public:
        OCV_RGB_Frame_Memory ()
        {
            batchSize=1;
            init_memory();
        }

        OCV_RGB_Frame_Memory (unsigned int bs)
        {
            batchSize=bs;
            init_memory();
        }

        ~OCV_RGB_Frame_Memory()
        {
            delete [] frames;
        }

        void SetFrame(unsigned int mb,Mat &f)
        {
            frames[mb]=f;
        }

        Mat* GetFramePtr(unsigned int mb)
        {
            return &frames[mb];
        }
    private:
        void init_memory()
        {
            frames=new Mat[batchSize];
        }
        unsigned int batchSize;
        Mat *frames;
    };
}
