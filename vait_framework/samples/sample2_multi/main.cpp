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

#include "tbb/flow_graph.h"
#include "../../workers/inference/inference.h"
#include "../../workers/ocv_reader/OCV_reader.h"
#include "../../workers/ocv_render/OCV_render.h"
#include "../../memories/SSD_ROI_memory.h"
#include "../../memories/OCV_RGB_Frame_Memory.h"


using namespace VAITFramework;
using namespace tbb::flow;


int main(int argc, char *argv[]) {

    InferenceEngine::StatusCode sts;
    const int batchSize=4;

    //memory
    OCV_RGB_Frame_Memory FrameMemory(batchSize);
    SSD_ROI_Memory ROImemory(batchSize);

    //workers
    OCV_reader OCVreader0("../../test_content/video/test_public4_448x448.avi",&FrameMemory,0);
    OCVreader0.Init();
    OCV_reader OCVreader1("../../test_content/video/test_public4_448x448.avi",&FrameMemory,1);
    OCVreader1.Init();
    OCV_reader OCVreader2("../../test_content/video/test_public4_448x448.avi",&FrameMemory,2);
    OCVreader2.Init();
    OCV_reader OCVreader3("../../test_content/video/test_public4_448x448.avi",&FrameMemory,3);
    OCVreader3.Init();
    Inference IE(.5, &FrameMemory,&ROImemory,
    "../../test_content/IR/SSD_squeezenet_INTEL_ONLY/squeezenet_v1.1_VOC0712_SSD_224x224_orig_deploy.xml");
    IE.Init();
    OCV_render OCVrender0(&FrameMemory,&ROImemory,0);
    OCVrender0.Init();
    OCV_render OCVrender1(&FrameMemory,&ROImemory,1);
    OCVrender1.Init();
    OCV_render OCVrender2(&FrameMemory,&ROImemory,2);
    OCVrender2.Init();
    OCV_render OCVrender3(&FrameMemory,&ROImemory,3);
    OCVrender3.Init();

    //define TBB flowgraph
     graph g;
     broadcast_node< continue_msg > start(g);

     continue_node<continue_msg> OCVreader0_node(g,
         [&OCVreader0]( const continue_msg &) {
          OCVreader0.Execute();
          printf("OCVreader0 ");
      });

     make_edge(start,OCVreader0_node);

     continue_node<continue_msg> OCVreader1_node(g,
         [&OCVreader1]( const continue_msg &) {
          OCVreader1.Execute();
          printf("OCVreader1 ");
      });

     make_edge(start,OCVreader1_node);

     continue_node<continue_msg> OCVreader2_node(g,
         [&OCVreader2]( const continue_msg &) {
          OCVreader2.Execute();
          printf("OCVreader2 ");
      });

     make_edge(start,OCVreader2_node);

     continue_node<continue_msg> OCVreader3_node(g,
         [&OCVreader3]( const continue_msg &) {
          OCVreader3.Execute();
          printf("OCVreader3 ");
      });

     make_edge(start,OCVreader3_node);

     continue_node<continue_msg> IE_node(g,
         [&IE, &ROImemory]( const continue_msg &) {
          IE.Execute();
          printf("IE ");
      });
     make_edge(OCVreader0_node,IE_node);
     make_edge(OCVreader1_node,IE_node);
     make_edge(OCVreader2_node,IE_node);
     make_edge(OCVreader3_node,IE_node);


     continue_node<continue_msg> OCVrender0_node(g,
         [&OCVrender0, &ROImemory]( const continue_msg &) {
          OCVrender0.Execute();
          printf("OCVrender0 ");
      });
     make_edge(IE_node,OCVrender0_node);

     continue_node<continue_msg> OCVrender1_node(g,
         [&OCVrender1, &ROImemory]( const continue_msg &) {
          OCVrender1.Execute();
          printf("OCVrender1 ");
      });
     make_edge(IE_node,OCVrender1_node);

     continue_node<continue_msg> OCVrender2_node(g,
         [&OCVrender2, &ROImemory]( const continue_msg &) {
          OCVrender2.Execute();
          printf("OCVrender2 ");
      });
     make_edge(IE_node,OCVrender2_node);

     continue_node<continue_msg> OCVrender3_node(g,
         [&OCVrender3, &ROImemory]( const continue_msg &) {
          OCVrender3.Execute();
          printf("OCVrender3 ");
      });
     make_edge(IE_node,OCVrender3_node);


     //main loop
     for (int i = 0; i < 200; ++i) {
        start.try_put(continue_msg());
        g.wait_for_all();
        puts("----");
    }


    return 0;
}
