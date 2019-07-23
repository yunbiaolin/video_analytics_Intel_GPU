/*****************************************************************************
*  Intel DSS Vidoe Analytic POC                                              *
*  Copyright (C) 2019 Henry.Wen  lei.xia@intel.com.                          *
*                                                                            *
*  This file is part of the POC.                                             *
*                                                                            *
*  This program is free software under Intel NDA;                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*  @file     image.hpp                                                       *
*  @brief    head file for image classes                                     *
*  @author   Lei.Xia                                                         *
*  @email    lei.xia@intel.com                                               *
*  @version  0.0.0.1                                                         *
*  @date     Mar 25, 2019                                                    *
*  @license  Intel NDA                                                       *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : Define the classes for image operation from decoded video*
*                   streams 
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version> | <Author>       | <Description>                   *
*----------------------------------------------------------------------------*
*  2019/03/25 | 0.0.0.1   | Lei.Xia        | Create file                     *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef __DSS_POC_IMAGE_HPP__
#define __DSS_POC_IMAGE_HPP__

#include <cstdint>
#include <string>
#include <cstring>

/** 
  * @brief enum of color space of image 
  * Defines the supported color formats. Currently, only NV12 and RGB formats
  * are supported.
  */
enum ColorSpace {
    UNKNOW_COLOR = 0,
    NV12, ///< NV12
    RGB // RGB
};

/** 
  * @brief enum of image formats
  * Defines the supported image compression formats. Currently, only JPEG 
  * is supported.
  */
enum ImageFormat {
    UNKNOW_FORMAT = 0, 
    JPEG ///< JPEG
};

/** 
  * @brief Base class of image
  * Defines the basic attributes of a image.
  */
struct ImageBase {
    ImageBase(): num_width(0), num_height(0), num_quality(100), enum_color(UNKNOW_COLOR), enum_format(UNKNOW_FORMAT), ptr_image(nullptr) {};
    uint32_t num_height; ///< Height of the image, 32 bits unsigned 
    uint32_t num_width; ///< Width if the image, 32 bits unsigned
    ColorSpace enum_color; ///< Color format, either NV12 or RGP
    ImageFormat enum_format; ///< Image compression format, must be JPEG today
    char *ptr_image = nullptr; ///< Pointer to the image buffer
    uint8_t num_quality;
    uint64_t num_timestamp; ///< Timestamp of the image
};

/** 
  * @brief Image class
  * Inherited from ImageBase. @class ImageBase
  * Defines interfaces of the image. 
  */
class Image {
    private: 
      ImageBase *ptr_base;
      size_t num_size;
    public:
      Image(); ///< The default constructor 
      /** 
        * @brief Constructor
        * @param height       Height of the image
        * @param width        Width of the image
        * @param quality      Compression Quality
        * @param color        Color space format of the image @enum ColorSpace
        * @param format       Image compression format @enum ImageFormat
        */
      Image(const uint32_t width, const uint32_t height, const uint8_t quality, const ColorSpace color, const ImageFormat format);
      /** 
        * @brief Deconstructor
        */
      ~Image(); 
      /** 
        * @brief Set image timestamp
        * @param timestamp        Timestamp of the image
        */
      void set_timestamp(const uint64_t timestamp); 
      /** 
        * @brief Set image width and height attribute
        * @param width        Width of the image
        * @param height       Height of the image
        */
      void set_resolution(const uint32_t width, const uint32_t height); 
      /** 
        * @brief Set image color space format attribute
        * @param color       Color spapce format of the image @enum ColorSpace
        */
      void set_color(const ColorSpace color); 
      /** 
        * @brief Set compression quality
        * @param quality      Compression quality
        */
      void set_quality(const uint8_t quality); 
      /** 
        * @brief Set image compression format attribute
        * @param format       Image compression format of the image @enum ImageFormat
        */
      void set_format(const ImageFormat format);
      /** 
        * @brief Set image buffer pointer to a image 
        * @param buffer       Pointer of a image buffer
        */
      void set_image(const void* image, size_t size);
      /** 
        * @brief free image buffer 
        */
      void delete_image_buffer();
      /** 
        * @brief Get image height attribute
        * @return     Height of the image
        *     @retval >0 height of the image
        *     @retval 0  image height is unknow
        */
      uint32_t height(); 
      /** 
        * @brief Get image Width attribute
        * @retrun     Width of the image
        *     @retval >0 width of the image
        *     @retval 0  image width is unknow
        */
      uint32_t width();
      /** 
        * @brief Get compression quality 
        * @retrun     The compression quality
        */
      uint8_t quality();
      /** 
        * @brief Get image color space format attribute
        * @return     Color spapce format of the image @enum ColorSpace
        */
      ColorSpace color();
      /** 
        * @brief Get image compression format attribute
        * @retrun     Image compression format of the image @enum ImageFormat
        */
      ImageFormat format();
      /** 
        * @brief      Get image timestamp
        * @retrun     Timestamp string
        */
      uint64_t timestamp();
      /** 
        * @brief Get the image 
        * @retrun     Pointer to the image 
        */
      char* image();
      /** 
        * @brief Get the memory size of image 
        * @retrun     Size of the image
        */
      size_t size();

};

/** 
  * @brief Base class of a stream
  * Defines the basic attributes of a stream.
  */
struct StreamBase {
    StreamBase(): num_id(0) {};
    uint32_t num_id;    ///< Stream ID
    std::string str_desc; ///< A optional description of the stream 
};


/** 
  * @brief Image class
  * Inherited from StreamBase. @class StreamBase
  * Defines interfaces of the image. 
  */
class Stream: private StreamBase {
    private:
      StreamBase *ptr_base;
    public:
      Stream(); ///< the default constructor
      /** 
        * @brief Constructor
        * @param id           ID of the stream
        */
      Stream(const uint32_t id);
      /** 
        * @brief Constructor
        * @param id           ID of the stream
        * @param desc         Description of the stream
        * @param quality      Compression Quality
        */
      Stream(const uint32_t id, const std::string & desc);
      /** 
        * @brief Deconstructor
        */
      ~Stream();
      /** 
        * @brief Set stream ID attribute
        * @param id           ID of the stream 
        */
      void set_id(const uint32_t id);
      /** 
        * @brief Set stream descrition attribute
        * @param desc          Description of the stream 
        */
      void set_desc(const std::string & desc);
      /** 
        * @brief Get stream ID attribute
        * @return     ID of the stream 
        */
      uint32_t id();
      /** 
        * @brief Get stream description attribute
        * @return     Description of the stream 
        */
      std::string desc();
};

/** 
  * @brief Class of a image from a stream
  * Inherited from Image and Stream. @class Image @class Stream
  */
class StreamImage: public Image, public Stream {
    public:
        /** 
          * @brief Copy constructor 
          */
        StreamImage(StreamImage& copy);
        /** 
          * @brief Constructor 
          * @param stream_id           ID of the stream
          * @param stream_desc         Description of the stream
          * @param image_timestamp    Timestamp of the image
          * @param image_height       Height of the image
          * @param image_width        Width of the image
          * @param image_quality      Compression quality of the image
          * @param image_color        Color space format of the image @enum ColorSpace
          * @param image_format       Image compression format @enum ImageFormat
          */
        //StreamImage(const uint32_t stream_id, const std::string &stream_desc, const uint32_t image_width, const uint32_t image_height, const uint8_t image_quality); 
        /** 
          * @brief Constructor 
          * @param stream_id           ID of the stream
          * @param stream_desc         Description of the stream
          * @param image_timestamp    Timestamp of the image
          * @param image_height       Height of the image
          * @param image_width        Width of the image
          * @param image_quality      Compression quality of the image
          * @param image_color        Color space format of the image @enum ColorSpace
          * @param image_format       Image compression format @enum ImageFormat
          */
        StreamImage(const uint32_t stream_id, const std::string &stream_desc, const uint64_t image_timestamp, const uint32_t image_width, const uint32_t image_height, const uint8_t image_quality); 
        /** 
          * @brief Constructor
          * @param stream_id           ID of the stream
          * @param stream_desc         Description of the stream
          * @param image_timestamp    Timestamp of the image
          * @param image_height       Height of the image
          * @param image_width        Width of the image
          * @param image_color        Color space format of the image @enum ColorSpace
          * @param image_format       Image compression format @enum ImageFormat
          */
        StreamImage(const uint32_t stream_id, const std::string &stream_desc, const uint64_t image_timestamp, const uint32_t image_width, const uint32_t image_height, const uint8_t image_quality, const ColorSpace image_color, const ImageFormat image_format); 

        StreamImage& operator=(StreamImage& copy); 
};

typedef StreamImage* StreamImage_ptr_t;
#endif