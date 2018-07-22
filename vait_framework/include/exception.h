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

#include <string>
#include <memory>
#include <sstream>

#define THROW_VAIT_EXCEPTION\
    throw VAITFramework::Exception(__FILE__, __LINE__)\

#ifdef NDEBUG
    #define VAIT_ASSERT(EXPRESSION)\
    if (!(EXPRESSION)) throw VAITFramework::Exception(__FILE__, __LINE__)\
    << "AssertionFailed: " << #EXPRESSION
#else
#include <cassert>
#define VAIT_ASSERT(EXPRESSION)\
    assert((EXPRESSION))
#endif  // NDEBUG

namespace VAITFramework {
class Exception : public std::exception {
public:
    const char *what() const noexcept override {
        if (errorDesc.empty() && exception_stream) {
            errorDesc = exception_stream->str();
#ifndef NDEBUG
            errorDesc +=  "\n" + _file + ":" + std::to_string(_line);
#endif
        }
        return errorDesc.c_str();
    }

    Exception(const std::string &filename, const int line) : _file(filename), _line(line) {
    }

    template<class T>
    Exception &operator<<(const T &arg) {
        if (!exception_stream) {
            exception_stream.reset(new std::stringstream());
        }
        (*exception_stream) << arg;
        return *this;
    }

private:
    mutable std::string errorDesc;
    std::string _file;
    int _line;
    std::shared_ptr<std::stringstream> exception_stream;
};
}
