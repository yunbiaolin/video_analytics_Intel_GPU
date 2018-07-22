#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace InferencePlugin {
    class ImageToInput;

    using ImageToInputPtr = std::shared_ptr<ImageToInput>;

    typedef enum {
        RGB,
        RGBA,
        BGR,
        BGRA,
        YUV420,
        YUV420_GRAY
    }
    ImageFormat;

    class ImageToInput {
    public:
        ImageToInput(ImageFormat srcFormat, size_t srcWidth, size_t srcHeight, size_t srcStrideInBytes, size_t dstWidth, size_t dstHeight);

    public:
        void SetMeanData(const std::vector<float>& data) {
            _meanData = data;
        }

        void SetNormData(const std::vector<float>& data) {
            _normData = data;
        }
        
        void Convert(const std::vector<uint8_t*>& srcs, float* dst);

    private:
        void InitParams();

        void FromRGBA(const std::vector<uint8_t*>& srcs, float* dst);
        void FromYUV(const std::vector<uint8_t*>& srcs, float* dst);
        void FromYUV_GRAY(const std::vector<uint8_t*>& srcs, float* dst);

        void YUV420ToRGB(uint8_t *yp, uint8_t *up, uint8_t *vp,
                         uint32_t sy, uint32_t suv,
                         int width, int height,
                         float* rgb, uint32_t srgb);

    private:
        ImageFormat _srcFormat;

        size_t _srcWidth;
        size_t _srcHeight;
        size_t _srcStride;

        size_t _dstWidth;
        size_t _dstHeight;

        size_t _padW;
        size_t _padH;

        size_t _rOff;
        size_t _gOff;
        size_t _bOff;

        size_t _srcChannels;
        size_t _dstChannels;

        std::vector<float> _meanData;
        std::vector<float> _normData;
    };
}
