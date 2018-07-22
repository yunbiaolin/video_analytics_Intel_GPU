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

#include <memory>
#include <vector>

#include "types.h"
#include "exception.h"

namespace VAITFramework {


typedef struct {
    Device device;

    DataType dataType;
    DataFormat dataFormat;

    std::vector<int> dims;
} MemoryDescr;

class Memory {


public:
    Memory(Device device, DataType dataType, DataFormat dataFormat, std::vector<int> dims, void* ptr = nullptr)
    {
        _descr.device = device;
        _descr.dataType = dataType;
        _descr.dataFormat = dataFormat;
        _descr.dims = dims;
        SetDataPointer(ptr);
    }
    Memory(MemoryDescr descr, void* ptr = nullptr)
    {
      _descr=descr;
      SetDataPointer(ptr);
    }

    Memory(){};

    virtual ~Memory() {};//=NULL;


    Device GetDevice() {
        return _descr.device;
    }

    DataType GetDataType() {
        return _descr.dataType;
    }

    void SetDataType(DataType type) {
        _descr.dataType = type;
    }

    DataFormat GetDataFormat() {
        return _descr.dataFormat;
    }

    void SetDataFormat(DataFormat format) {
        _descr.dataFormat = format;
    }

    std::vector<int> GetDimensions() {
        return _descr.dims;
    }

    void SetDimensions(std::vector<int>& dims) {
        _descr.dims = dims;
    }

    

    void* GetData() {
        return ptr;
    }

    void SetDataPointer(void* data) {
        ptr=data;
    }

    void SetDescr(MemoryDescr descr)
    {
        _descr=descr;
    }
    size_t GetDataTypeSize(DataType dataType) {
        switch (dataType) {
            case I8:
            case U8:
                return 1;
            case F16:
                return 2;
            case F32:
                return 4;
            default:
                THROW_VAIT_EXCEPTION << "Unknown data type.";
        }
    }

private:
    MemoryDescr _descr;

    void *ptr;
};

}
