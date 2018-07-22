#include <stdlib.h>

#include "image_to_input.h"

#if defined(_MSC_VER)
    #include <intrin.h>
#else
    #include <x86intrin.h>
#endif

#include <string.h>

using namespace InferencePlugin;

#pragma GCC diagnostic ignored "-Wunused-variable"

ImageToInput::ImageToInput(ImageFormat srcFormat, size_t srcWidth, size_t srcHeight, size_t srcStrideInBytes,
                           size_t dstWidth, size_t dstHeight) :
    _srcFormat(srcFormat), _srcWidth(srcWidth), _srcHeight(srcHeight), _srcStride(srcStrideInBytes),
    _dstWidth(dstWidth), _dstHeight(dstHeight)
{
    InitParams();
}

void ImageToInput::Convert(const std::vector<uint8_t*>& srcs, float* dst) {
    switch (_srcFormat) {
        case BGRA:
        case BGR:
        case RGB:
        case RGBA:
            FromRGBA(srcs, dst);
            break;

        case YUV420: {
            FromYUV(srcs, dst);
        }
            break;

        case YUV420_GRAY: {
            FromYUV_GRAY(srcs, dst);
        }
            break;

        default: {
            throw "Not supported image format!";
        }
            break;
    }
}

#ifdef BXT

void ImageToInput::FromRGBA(const std::vector<uint8_t*>& srcs, float* dst) {
    const size_t dstChannelSize = _dstWidth * _dstHeight;

    float* rBuffer = &dst[dstChannelSize * 0];
    float* gBuffer = &dst[dstChannelSize * 1];
    float* bBuffer = &dst[dstChannelSize * 2];

    size_t idx = 0;

    bool hasMean = (_meanData.size() == 3);
    bool hasNorm = (_normData.size() == 3);

    if ((_padH != 0 && _padW != 0) || _srcChannels != 4) {

        for (size_t mb = 0; mb < srcs.size(); mb++) {
            const uint8_t* srcBuffer = srcs[mb];

            idx = dstChannelSize * _dstChannels * mb;

            for (size_t h = _padH; h < (_srcHeight - _padH); h++) {
                size_t srcIdx = (h * _srcStride + _padW * _srcChannels);

                for (size_t w = _padW; w < (_srcWidth - _padW); w++, idx++) {
                    rBuffer[idx] = srcBuffer[srcIdx + _rOff];
                    gBuffer[idx] = srcBuffer[srcIdx + _gOff];
                    bBuffer[idx] = srcBuffer[srcIdx + _bOff];

                    srcIdx += _srcChannels;
                }
            }
        }
    }
    else {
    ;    
    }
}

#else

void ImageToInput::FromRGBA(const std::vector<uint8_t*>& srcs, float* dst) {
    const size_t dstChannelSize = _dstWidth * _dstHeight;

    float* rBuffer = &dst[dstChannelSize * 0];
    float* gBuffer = &dst[dstChannelSize * 1];
    float* bBuffer = &dst[dstChannelSize * 2];

    size_t idx = 0;

    bool hasMean = (_meanData.size() == 3);
    bool hasNorm = (_normData.size() == 3);

    __m256 meanReg[3];
    __m256 normReg[3];

    if (hasMean) {
        for (size_t i = 0; i < _dstChannels; i++) {
            float mean = _meanData[i];
            meanReg[i] = _mm256_broadcast_ss(&mean);
        }
    }

    if (hasNorm) {
        for (size_t i = 0; i < _dstChannels; i++) {
            float norm = _normData[i];
            normReg[i] = _mm256_broadcast_ss(&norm);
        }
    }

    if ((_padH != 0 && _padW != 0) || _srcChannels != 4) {

        for (size_t mb = 0; mb < srcs.size(); mb++) {
            const uint8_t* srcBuffer = srcs[mb];

            idx = dstChannelSize * _dstChannels * mb;

            for (size_t h = _padH; h < (_srcHeight - _padH); h++) {
                size_t srcIdx = (h * _srcStride + _padW * _srcChannels);

                for (size_t w = _padW; w < (_srcWidth - _padW); w++, idx++) {
                    rBuffer[idx] = srcBuffer[srcIdx + _rOff];
                    gBuffer[idx] = srcBuffer[srcIdx + _gOff];
                    bBuffer[idx] = srcBuffer[srcIdx + _bOff];

                    srcIdx += _srcChannels;
                }
            }
        }

        if (hasMean) {
            idx = 0;

            __m256 valReg;

            for (size_t mb = 0; mb < srcs.size(); mb++) {
                for (size_t ch = 0; ch < _dstChannels; ch++) {
                    float meanVal = _meanData[ch];

                    for (size_t i = 0; i < dstChannelSize; i += 8, idx += 8) {
                        valReg = _mm256_loadu_ps(&dst[idx]);
                        valReg = _mm256_sub_ps(valReg, meanReg[ch]);
                        _mm256_storeu_ps(&dst[idx], valReg);
                    }

                    size_t tail = (dstChannelSize % 8);

                    for (size_t i = 0; i < tail; i++, idx++) {
                        dst[idx] -= meanVal;
                    }
                }
            }
        }

        if (hasNorm) {
            idx = 0;

            __m256 valReg;

            for (size_t mb = 0; mb < srcs.size(); mb++) {
                for (size_t ch = 0; ch < _dstChannels; ch++) {
                    float normVal = _normData[ch];

                    for (size_t i = 0; i < dstChannelSize; i += 8, idx += 8) {
                        valReg = _mm256_loadu_ps(&dst[idx]);
                        valReg = _mm256_mul_ps(valReg, meanReg[ch]);
                        _mm256_storeu_ps(&dst[idx], valReg);
                    }

                    size_t tail = (dstChannelSize % 8);

                    for (size_t i = 0; i < tail; i++, idx++) {
                        dst[idx] *= normVal;
                    }
                }
            }
        }

    }
    else {
        
        __m128i mask = {0};

        if (_srcFormat == RGBA) {
            mask = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
        }
        else if (_srcFormat == BGRA) {
            mask = _mm_setr_epi8(2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12, 3, 7, 11, 15);
        }

        __m128i row[4];
        __m128i tmp[4];

        __m256 res[3][2];

        size_t srcImageSize = _srcStride * _srcHeight;
        size_t dstImageSize = dstChannelSize * _dstChannels;

        float* dstBuffers[3] = { rBuffer, gBuffer, bBuffer };

        for (size_t mb = 0; mb < srcs.size(); mb++) {
            const uint8_t* srcBuffer = srcs[mb];

            idx = dstImageSize * mb;

            for (int i = 0; i < srcImageSize; i += 64, idx += 16) {
                for(int r = 0; r < 4; r++) {
                    row[r] = _mm_loadu_si128((__m128i*)&srcBuffer[i + 16 * r]);
                    row[r] = _mm_shuffle_epi8(row[r], mask);
                }

                tmp[0] = _mm_unpacklo_epi32(row[0], row[1]);
                tmp[1] = _mm_unpacklo_epi32(row[2], row[3]);
                tmp[2] = _mm_unpackhi_epi32(row[0], row[1]);
                tmp[3] = _mm_unpackhi_epi32(row[2], row[3]);

                row[0] = _mm_unpacklo_epi64(tmp[0], tmp[1]);
                row[1] = _mm_unpackhi_epi64(tmp[0], tmp[1]);
                row[2] = _mm_unpacklo_epi64(tmp[2], tmp[3]);

                for (int r = 0; r < 3; r++) {
                    res[r][0] = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(row[r]));
                    res[r][1] = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(row[r], 8)));
                }

                if (hasMean) {
                    for (int r = 0; r < 3; r++) {
                        res[r][0] = _mm256_sub_ps(res[r][0], meanReg[r]);
                        res[r][1] = _mm256_sub_ps(res[r][1], meanReg[r]);
                    }
                }

                if (hasNorm) {
                    for (int r = 0; r < 3; r++) {
                        res[r][0] = _mm256_mul_ps(res[r][0], normReg[r]);
                        res[r][1] = _mm256_mul_ps(res[r][1], normReg[r]);
                    }
                }

                for (int r = 0; r < 3; r++) {
                    _mm256_storeu_ps(&dstBuffers[r][idx], res[r][0]);
                    _mm256_storeu_ps(&dstBuffers[r][idx + 8], res[r][1]);
                }
            }

            size_t tail = srcImageSize % 64;

            for (size_t i = (srcImageSize - tail); i < srcImageSize; i++, idx++) {
                rBuffer[idx] = srcBuffer[i + _rOff];
                gBuffer[idx] = srcBuffer[i + _gOff];
                bBuffer[idx] = srcBuffer[i + _bOff];

                if (hasMean) {
                    rBuffer[idx] -= _meanData[0];
                    gBuffer[idx] -= _meanData[1];
                    bBuffer[idx] -= _meanData[2];
                }

                if (hasNorm) {
                    rBuffer[idx] *= _normData[0];
                    gBuffer[idx] *= _normData[1];
                    bBuffer[idx] *= _normData[2];
                }
            }
        }
    }
}

#endif

void ImageToInput::FromYUV(const std::vector<uint8_t*>& srcs, float* dst) {
    const size_t sourceChannels = 1;
    const size_t destChannels = 3;

    const size_t channelSize = _dstWidth * _dstHeight;

    float* rBuffer = &dst[channelSize * 0];
    float* gBuffer = &dst[channelSize * 1];
    float* bBuffer = &dst[channelSize * 2];

    size_t idx = 0;

    if (_meanData.size() == 3) {
        for (size_t mb = 0; mb < srcs.size(); mb++) {
            uint8_t* yBuffer = srcs[mb];
            uint8_t* uBuffer = yBuffer + (_srcHeight * _srcStride);
            uint8_t* vBuffer = uBuffer + (_srcHeight * _srcStride) / 4;

            idx = channelSize * destChannels * mb;

            for (size_t ch = 0; ch < sourceChannels; ch++) {
                for (size_t h = _padH; h < (_srcHeight - _padH); h++) {
                    size_t yIdx = (h * _srcStride + _padW) * sourceChannels;
                    size_t uvIdx = (h / 2 * _srcWidth + _padW) / 2;

                    for (size_t w = _padW; w < (_srcWidth - _padW); w += 2, uvIdx++) {
                        uint8_t U = uBuffer[uvIdx];
                        uint8_t V = vBuffer[uvIdx];

                        uint8_t Y = yBuffer[yIdx];

                        float YM = 1.164 * (Y - 16);
                        float VR = 1.596 * (V - 128) - _meanData[0];
                        float VUG = 0.813 * (V - 128) - 0.391 * (U - 128) - _meanData[1];
                        float UB = 2.018 * (U - 128) - _meanData[2];

                        rBuffer[idx] = YM + VR;
                        gBuffer[idx] = YM - VUG;
                        bBuffer[idx] = YM + UB;

                        idx++;
                        yIdx++;

                        Y = yBuffer[yIdx];

                        YM = 1.164 * (Y - 16);

                        rBuffer[idx] = YM + VR;
                        gBuffer[idx] = YM - VUG;
                        bBuffer[idx] = YM + UB;
                        
                        idx++;
                        yIdx++;
                    }
                }
            }
        }
    }
    else {
        for (size_t mb = 0; mb < srcs.size(); mb++) {
            uint8_t* yBuffer = srcs[mb];
            uint8_t* uBuffer = yBuffer + (_srcHeight * _srcStride);
            uint8_t* vBuffer = uBuffer + (_srcHeight * _srcStride) / 4;

            YUV420ToRGB(yBuffer, uBuffer, vBuffer, (uint32_t)_srcStride, (uint32_t)(_srcStride / 2),
                        (uint32_t)_srcWidth, (uint32_t)_srcHeight, dst + channelSize * destChannels * mb, (uint32_t)_srcWidth);
        }
    }
}

void ImageToInput::FromYUV_GRAY(const std::vector<uint8_t*>& srcs, float* dst) {
    const size_t sourceChannels = 1;
    const size_t destChannels = 3;

    const size_t channelSize = _dstWidth * _dstHeight;
    const size_t channelSizeInBytes = _dstWidth * _dstHeight * sizeof(float);

    float* rBuffer = &dst[channelSize * 0];
    float* gBuffer = &dst[channelSize * 1];
    float* bBuffer = &dst[channelSize * 2];

    size_t idx = 0;

    if (_meanData.size() == 3) {
        for (size_t mb = 0; mb < srcs.size(); mb++) {
            const uint8_t* srcBuffer = srcs[mb];

            idx = channelSize * sourceChannels * mb;

            for (size_t ch = 0; ch < destChannels; ch++) {
                for (size_t h = _padH; h < (_srcHeight - _padH); h++) {
                    size_t srcIdx = (h * _srcStride + _padW) * sourceChannels;

                    for (size_t w = _padW; w < (_srcWidth - _padW); w++, idx++, srcIdx++) {
                        rBuffer[idx] = srcBuffer[srcIdx] - _meanData[ch];
                    }
                }
            }
        }
    }
    else {
        for (size_t mb = 0; mb < srcs.size(); mb++) {
            const uint8_t* srcBuffer = srcs[mb];

            idx = channelSize * destChannels * mb;

            for (size_t ch = 0; ch < sourceChannels; ch++) {
                for (size_t h = _padH; h < (_srcHeight - _padH); h++) {
                    size_t srcIdx = (h * _srcStride + _padW) * sourceChannels;

                    for (size_t w = _padW; w < (_srcWidth - _padW); w++, idx++, srcIdx++) {
                        rBuffer[idx] = srcBuffer[srcIdx] - 16;
                    }
                }
            }

            idx -= channelSize;

            memcpy(&gBuffer[idx], &rBuffer[idx], channelSizeInBytes);
            memcpy(&bBuffer[idx], &gBuffer[idx], channelSizeInBytes);
        }
    }
}

void ImageToInput::YUV420ToRGB(uint8_t *yp, uint8_t *up, uint8_t *vp,
                               uint32_t sy, uint32_t suv,
                               int width, int height,
                               float* rgb, uint32_t srgb )
{

#ifndef BXT
    __m128i y0r0, y0r1, u0, v0;
    __m128i y00r0, y01r0, y00r1, y01r1;
    __m128i u00, u01, v00, v01;
    __m128i rv00, rv01, gu00, gu01, gv00, gv01, bu00, bu01;
    __m128i r00, r01, g00, g01, b00, b01;

    __m128i *srcy128r0, *srcy128r1;

    float *dstR128r0;
    float *dstR128r1;

    float *dstG128r0;
    float *dstG128r1;

    float *dstB128r0;
    float *dstB128r1;

    __m64 *srcu64;
    __m64 *srcv64;

    int x;

    __m128i ysub = _mm_set1_epi32(0x00100010);
    __m128i uvsub = _mm_set1_epi32(0x00800080);

    __m128i facy = _mm_set1_epi32(0x004a004a);
    __m128i facrv = _mm_set1_epi32(0x00660066);
    __m128i facgu = _mm_set1_epi32(0x00190019);
    __m128i facgv = _mm_set1_epi32(0x00340034);
    __m128i facbu = _mm_set1_epi32(0x00810081);

    __m128i zero = _mm_set1_epi32(0x00000000);

    for (int y = 0; y < height; y += 2) {
        srcy128r0 = (__m128i*)(yp + sy * y);
        srcy128r1 = (__m128i*)(yp + sy * y + sy);

        srcu64 = (__m64*)(up + suv * (y / 2));
        srcv64 = (__m64*)(vp + suv * (y / 2));

        dstR128r0 = (rgb + srgb * y);
        dstR128r1 = (rgb + srgb * y + srgb);

        dstG128r0 = (rgb + height * srgb + srgb * y);
        dstG128r1 = (rgb + height * srgb + srgb * y + srgb);

        dstB128r0 = (rgb + height * srgb * 2 + srgb * y);
        dstB128r1 = (rgb + height * srgb * 2 + srgb * y + srgb);

        for (x = 0; x < width; x += 16) {
            u0 = _mm_loadl_epi64((__m128i*)srcu64);
            srcu64++;

            v0 = _mm_loadl_epi64((__m128i*)srcv64);
            srcv64++;

            y0r0 = _mm_loadu_si128(srcy128r0++);
            y0r1 = _mm_loadu_si128(srcy128r1++);

            y00r0 = _mm_mullo_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(y0r0, zero), ysub), facy);
            y01r0 = _mm_mullo_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(y0r0, zero), ysub), facy);
            y00r1 = _mm_mullo_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(y0r1, zero), ysub), facy);
            y01r1 = _mm_mullo_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(y0r1, zero), ysub), facy);

            u0 = _mm_unpacklo_epi8(u0,  zero);
            u00 = _mm_sub_epi16(_mm_unpacklo_epi16(u0, u0), uvsub);
            u01 = _mm_sub_epi16(_mm_unpackhi_epi16(u0, u0), uvsub);

            v0 = _mm_unpacklo_epi8(v0,  zero);
            v00 = _mm_sub_epi16(_mm_unpacklo_epi16(v0, v0), uvsub);
            v01 = _mm_sub_epi16(_mm_unpackhi_epi16(v0, v0), uvsub);

            rv00 = _mm_mullo_epi16(facrv, v00);
            rv01 = _mm_mullo_epi16(facrv, v01);
            gu00 = _mm_mullo_epi16(facgu, u00);
            gu01 = _mm_mullo_epi16(facgu, u01);
            gv00 = _mm_mullo_epi16(facgv, v00);
            gv01 = _mm_mullo_epi16(facgv, v01);
            bu00 = _mm_mullo_epi16(facbu, u00);
            bu01 = _mm_mullo_epi16(facbu, u01);

            // row 0
            r00 = _mm_srai_epi16(_mm_add_epi16(y00r0, rv00), 6);
            r01 = _mm_srai_epi16(_mm_add_epi16(y01r0, rv01), 6);
            g00 = _mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y00r0, gu00), gv00), 6);
            g01 = _mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y01r0, gu01), gv01), 6);
            b00 = _mm_srai_epi16(_mm_add_epi16(y00r0, bu00), 6);
            b01 = _mm_srai_epi16(_mm_add_epi16(y01r0, bu01), 6);

            r00 = _mm_packus_epi16(r00, r01);
            g00 = _mm_packus_epi16(g00, g01);
            b00 = _mm_packus_epi16(b00, b01);

            _mm256_store_ps(dstR128r0, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(r00)));
            _mm256_store_ps(dstR128r0 + 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(r00, 8))));

            _mm256_store_ps(dstG128r0, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(g00)));
            _mm256_store_ps(dstG128r0 + 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(g00, 8))));

            _mm256_store_ps(dstB128r0, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(b00)));
            _mm256_store_ps(dstB128r0 + 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(b00, 8))));

            dstR128r0 += 16;
            dstG128r0 += 16;
            dstB128r0 += 16;

            // row 1
            r00 = _mm_srai_epi16(_mm_add_epi16(y00r1, rv00), 6);
            r01 = _mm_srai_epi16(_mm_add_epi16(y01r1, rv01), 6);
            g00 = _mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y00r1, gu00), gv00), 6);
            g01 = _mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y01r1, gu01), gv01), 6);
            b00 = _mm_srai_epi16(_mm_add_epi16(y00r1, bu00), 6);
            b01 = _mm_srai_epi16(_mm_add_epi16(y01r1, bu01), 6);

            r00 = _mm_packus_epi16(r00, r01);
            g00 = _mm_packus_epi16(g00, g01);
            b00 = _mm_packus_epi16(b00, b01);

            _mm256_store_ps(dstR128r1, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(r00)));
            _mm256_store_ps(dstR128r1 + 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(r00, 8))));

            _mm256_store_ps(dstG128r1, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(g00)));
            _mm256_store_ps(dstG128r1 + 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(g00, 8))));
            
            _mm256_store_ps(dstB128r1, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(b00)));
            _mm256_store_ps(dstB128r1 + 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_srli_si128(b00, 8))));
            
            dstR128r1 += 16;
            dstG128r1 += 16;
            dstB128r1 += 16;
        }
    }
#endif

}

void ImageToInput::InitParams() {
    _dstChannels = 3;

    _padW = (_srcWidth - _dstWidth) / 2;
    _padH = (_srcHeight - _dstHeight) / 2;

    switch (_srcFormat) {
        case RGB:
        case RGBA:
            _rOff = 0;
            _gOff = 1;
            _bOff = 2;

            _srcChannels = (_srcFormat == RGBA) ? 4 : 3;
            break;

        case BGR:
        case BGRA:
            _rOff = 2;
            _gOff = 1;
            _bOff = 0;

            _srcChannels = (_srcFormat == BGRA) ? 4 : 3;
            break;

        default:
            throw "Unsupported input format.";
    }
}

