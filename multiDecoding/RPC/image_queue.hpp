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
*  @file     image_queue.hpp                                                       *
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

#ifndef __DSS_POC_IMAGE_QUEUE_HPP__
#define __DSS_POC_IMAGE_QUEUE_HPP__
#include "image.hpp"
#include <configuration.hpp>
#include <scheduler.hpp>
#include <vector>
#include <utility.hpp>
#include <Ice/Ice.h>
#include <scheduler.hpp>

class ImageQueue;
/** 
  * @brief ImageData type
  */
typedef typename ::std::vector <StreamImage_ptr_t> ImageData_t;
/** 
  * @brief ImageQueue_ptr_t type
  */
typedef ImageQueue* ImageQueue_ptr_t;
/** 
  * @brief Detection function callback type
  */
typedef void (*DetectionFun_t)(ImageData_t&, void*);


/** 
  * @brief Image class
  * Inherited from ImageBase. @class ImageBase
  * Defines interfaces of the image. 
  */
class ImageQueue {
    private:
        QueueRole enum_role; ///< Role of the queue to decide actions in add_image()
        uint16_t num_batch_size;  ///< Batch size threshold to trigger data transmission
        ImageData_t data_queue; /// The queue to store the images @class ImageData
        DetectionFun_t ptr_detection_fun; ///< pointer to a detection function;
        static Ice::CommunicatorHolder* ptr_ich; /// pointer to ICE communication holder
        static SchedulerBase* ptr_scheduler; /// pointer to a scheduler
        static Configuration* ptr_config; /// pointer to configuration
        static int counter; /// counter of instance
        void* object;
        /** 
          * @brief Send a batch of images to another node on network
          * @remarks
          *     Triggered inside the push() call when the size of the queue 
          *     reaches num_batch_size if enum_role=CLIENT
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool send_batch(); 
        /** 
          * @brief Run the registered inference function 
          * @remarks  
          *     Triggered inside the push() call when the size of the queue 
          *     reaches num_batch_size if enum_role=SERVER
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool do_detection();
    public:
        /** 
          * @brief Remove n items from the frontd
          * @param n  The number of the items to remove
          * @return bool
          *     @retval false fail
          *     @retval true success
          */
        bool remove_front(size_t n);
        ImageQueue() = delete; ///< disable the default constructor (C++11 or newer)
        /** 
          * @brief Constructuor
          */
        ImageQueue(QueueRole role);
        /** 
          * @brief Deconstructuor
          */
        ~ImageQueue();
        /** 
          * @brief append an image to the end of the queue
          * @param image       Reference of an StreamImage object @class StreamImage
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool push(StreamImage &image);
        
        /** 
          * @brief retrive an image from the top of the queue
          * @param image       Reference of an StreamImage object @class StreamImage
          * @return bool
          *     @retval false   No data send or inference event is triggered
          *     @retval true   A data send or inference event is triggered
          */
        StreamImage_ptr_t pop();
        /** 
          * @brief Get size of the image queue
          * @return Size of the image queue
          */
        size_t size();
        /** 
          * @brief Set batch size of the queue
          * @param batch_size       The batch size
          */
        void set_batch_size(uint16_t batch_size);
        /** 
          * @brief Register the detection callback function
          * @param ptr_fun      A callback function pointer for detection
          */
        void reg_detection_fun(DetectionFun_t ptr_fun, void* object);
        /** 
          * @brief Set up a schedule for all ImageQueue instance
          * @param s  Pointer of a scheduler 
          */
        void set_scheduler(SchedulerBase* ptr_s);
        /** 
          * @brief Get the static configuration of image queue
          * @return Pointer to the configuration object
          */
        Configuration* get_config();

};



#endif