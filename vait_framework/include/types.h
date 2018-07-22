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
#include <vector>

namespace VAITFramework {

using uint8= unsigned char;


typedef struct {
    int labelnum;
    float confidence;
    float xmin;
    float ymin;
    float xmax;
    float ymax;
}  SSD_ROI_type;


using Detectiontype = std::vector<SSD_ROI_type>;
using ROImemtype = std::vector< Detectiontype > ;

typedef enum {
    CPU,
    GPU,
    FPGA
} Device;

typedef enum {
    ANY_TYPE,
    I8,
    U8,
    F16,
    F32,
} DataType;


typedef enum {
    ANY_FORMAT,
    GRAY,
    RGB,
    BGR,
    BGRA,
    RGBA,
    YUV420,
    YUV420_GRAY,
    RGB_PLANAR,
    SSD_ROI
} DataFormat;



typedef enum {
    NOT_INITIALIZED,
    READY
} State;

typedef enum {
    OK = 0,
    GENERAL_ERROR = -1,
    NOT_IMPLEMENTED = -2
} StatusCode;


}
