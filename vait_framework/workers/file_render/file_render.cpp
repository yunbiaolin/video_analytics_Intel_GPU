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

#include "file_render.h"
using namespace VAITFramework;


int file_render::Execute()
{
    int ndetect=ROIs->GetNumDetections(batchnum);

    Mat *pF=frames->GetFramePtr(batchnum);
    for (int i=0;i<ndetect;i++) {
        SSD_ROI_type ROI=ROIs->GetDetection(batchnum,i);

        int ncols=pF->cols;
        int nrows=pF->rows;

        float xmin       = ROI.xmin * (float)ncols;
        float ymin       = ROI.ymin * (float)nrows;
        float xmax       = ROI.xmax * (float)ncols;
        float ymax       = ROI.ymax * (float)nrows;


        rectangle(*pF,
                          Point((int)xmin, (int)ymin),
                          Point((int)xmax, (int)ymax), Scalar(0, 255, 0),
                          0, LINE_8,0);
    }

    if (video.isOpened())
    {
    video.write(*pF);
    
    }
}
