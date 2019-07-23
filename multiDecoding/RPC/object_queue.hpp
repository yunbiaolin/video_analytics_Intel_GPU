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
*  @file     object_queue.hpp                                                *
*  @brief    head file for object queue classes                              *
*  @author   Lei.Xia                                                         *
*  @email    lei.xia@intel.com                                               *
*  @version  0.0.0.1                                                         *
*  @date     Mar 25, 2019                                                    *
*  @license  Intel NDA                                                       *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : Define the classes for Object operation from decoded video*
*                   streams 
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version> | <Author>       | <Description>                   *
*----------------------------------------------------------------------------*
*  2019/03/25 | 0.0.0.1   | Lei.Xia        | Create file                     *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef __DSS_POC_OBJECT_QUEUE_HPP__
#define __DSS_POC_OBJECT_QUEUE_HPP__

#include "configuration.hpp"
#include <vector>
#include <utility.hpp>
#include <object.hpp>
#include <Ice/Ice.h>
#include <scheduler.hpp>

class ObjectQueue;
/** 
  * @brief ObjectData type
  */
typedef typename ::std::vector <Object_ptr_t> ObjectData_t;
/** 
  * @brief ObjectQueue_ptr_t type
  */
typedef ObjectQueue* ObjectQueue_ptr_t;
/** 
  * @brief Classification function callback type
  */
typedef void (*ClassificationFun_t)(ObjectData_t&);


/** 
  * @brief ObjectQueue class
  */
class ObjectQueue {
    private:
        QueueRole enum_role; ///< Role of the queue
        uint16_t num_batch_size;  ///< Batch size threshold to trigger data transmission or local processing
        ObjectData_t data_queue; /// The queue to store the Objects @class ObjectData
        ClassificationFun_t ptr_classification_fun; ///< point to a detection function;
        static Ice::CommunicatorHolder* ptr_ich; /// pointer to ICE communication holder
        static SchedulerBase* ptr_scheduler; /// pointer to a scheduler
        static Configuration* ptr_config; /// pointer to configuration
        static int counter; /// counter of instance
        /** 
          * @brief Send a batch of Objects to another node on network
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
        bool do_classification();
    public:
        /** 
          * @brief Remove n items from the frontd
          * @param n  The number of the items to remove
          * @return bool
          *     @retval false fail
          *     @retval true success
          */
        bool remove_front(size_t n);
        ObjectQueue() = delete; ///< disable the default constructor (C++11 or newer)
        /** 
          * @brief Constructuor
          */
        ObjectQueue(QueueRole role);
        /** 
          * @brief Constructuor
          */
        ObjectQueue(ObjectQueue&);
        /** 
          * @brief Deconstructuor
          */
        ~ObjectQueue();
        /** 
          * @brief append an Object to the end of the queue
          * @param Object       Reference of an StreamObject object @class StreamObject
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool push(Object &Object);
        
        /** 
          * @brief retrive an Object from the top of the queue
          * @param Object       Reference of an StreamObject object @class StreamObject
          * @return bool
          *     @retval false   No data send or inference event is triggered
          *     @retval true   A data send or inference event is triggered
          */
        Object_ptr_t pop();
        /** 
          * @brief Get size of the Object queue
          * @return Size of the Object queue
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
        void reg_classification_fun(ClassificationFun_t ptr_fun);
};

#endif