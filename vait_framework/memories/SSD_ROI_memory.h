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

namespace VAITFramework {
    class SSD_ROI_Memory : public Memory {
    public:
        SSD_ROI_Memory ()
        {
            init_memory();
        }
        SSD_ROI_Memory (unsigned int batchSize)
        {
            init_memory();
            Detectiontype D;
            for(int i=0;i<batchSize;i++)
            {
             ROIs.push_back(D);
            }
        }
        unsigned int GetSize()
        {
            return ROIs.size();
        }

        void ClearDetections(unsigned int mb)
        {
            ROIs[mb].clear();
        }

        void StoreDetection(SSD_ROI_type ROItmp,unsigned int mb)
        {
            ROIs[mb].push_back(ROItmp);
        }

        unsigned int GetNumDetections(unsigned int mb)
        {
            return ROIs[mb].size();
        }

        SSD_ROI_type GetDetection(unsigned int mb, unsigned int n)
        {
            if (n>ROIs[mb].size())
            {
                SSD_ROI_type tmpROI;
                memset(&tmpROI,0,sizeof(SSD_ROI_type));
                return tmpROI;
            }
            return ROIs[mb][n];
        }
    private:
        void init_memory()
        {
            MemoryDescr descr;
            descr.device = CPU;
            descr.dataType = F32;
            descr.dataFormat = SSD_ROI;
            descr.dims = std::vector<int> {-1};
            SetDescr(descr);
            SetDataPointer(&ROIs);
        }
        ROImemtype ROIs;
    };
}
