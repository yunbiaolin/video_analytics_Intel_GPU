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

#include "tbb/tbb.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include "tbb/scalable_allocator.h"

#include "../../workers/inference/inference.h"
#include "../../workers/ocv_reader/OCV_reader.h"
#include "../../workers/ocv_render/OCV_render.h"
#include "../../workers/file_render/file_render.h"
#include "../../memories/SSD_ROI_memory.h"
#include "../../memories/OCV_RGB_Frame_Memory.h"


using namespace VAITFramework;
using namespace tbb;


int main(int argc, char *argv[]) {

    InferenceEngine::StatusCode sts;
    const int batchSize=4;

    //memory
    OCV_RGB_Frame_Memory FrameMemory(batchSize);
    SSD_ROI_Memory ROImemory(batchSize);

    //workers
    OCV_reader *OCVreaders[batchSize];
    for (int b=0;b<batchSize;b++)
    {
      OCVreaders[b]=new OCV_reader("../../../test_content/video/test_public4_448x448.avi",&FrameMemory,b);
      OCVreaders[b]->Init();
    }

    Inference IE(.5, &FrameMemory,&ROImemory,
    "../../../test_content/IR/SSD_squeezenet_INTEL_ONLY/squeezenet_v1.1_VOC0712_SSD_224x224_orig_deploy.xml");
    IE.Init();

#if 0
    OCV_render *OCVrenders[batchSize];
    for (int b=0;b<batchSize;b++)
    {
       OCVrenders[b]=new OCV_render(&FrameMemory,&ROImemory,b);
       OCVrenders[b]->Init();
    }
#else
    file_render *renders[batchSize];
    for (int b=0;b<batchSize;b++)
    {
       renders[b]=new file_render(&FrameMemory,&ROImemory,b);
       OCVreaders[b]->Execute(); 
       renders[b]->Init();
    }
#endif

    
    int nthreads=4;
    if (0==nthreads)
        nthreads = task_scheduler_init::default_num_threads();
    task_scheduler_init init(nthreads);

    for (int framenum=0;framenum<200;framenum++)
    {
      parallel_for( blocked_range<size_t>(0,batchSize,batchSize/nthreads), 
      [=](const blocked_range<size_t>& r) {
                      //printf("[%d-%d]\n",r.begin(),r.end()); fflush(stdout);
                      for(size_t b=r.begin(); b!=r.end(); ++b) 
                          OCVreaders[b]->Execute(); 
                  }
      );
 

      IE.Execute();


      parallel_for( blocked_range<size_t>(0,batchSize,batchSize/nthreads), 
      [=](const blocked_range<size_t>& r) {
                      for(size_t b=r.begin(); b!=r.end(); ++b) 
                          renders[b]->Execute(); 
                  }
      );
      
      printf("frame %d\n",framenum);
    }

    return 0;
}
