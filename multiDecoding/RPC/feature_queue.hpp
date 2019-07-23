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
*  @file     feature.hpp                                                      *
*  @brief    head file for feature classes                                    *
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

#ifndef __DSS_POC_FEATURE_QUEUE_HPP__
#define __DSS_POC_FEATURE_QUEUE_HPP__

#include <vector>
#include <default.hpp>
#include <feature.hpp>
#include <Ice/Ice.h>
#include <scheduler.hpp>
#include <utility.hpp>




typedef std::vector<Feature> Features_t; 
typedef void (*MatchingFun_t)(Features_t&);

/** 
  * @brief FeatureQueue class
  * Defines the queue of features 
  */
class FeatureQueue{
    private:
        QueueRole enum_role; ///< Role of the queue to decide actions in add_object()
        uint16_t num_batch_size;  ///< Batch size threshold to trigger data transmission
        Features_t features; ///< vector of feature
        MatchingFun_t ptr_matching_fun; ///< pointer to a detection function;
        static Ice::CommunicatorHolder* ptr_ich; ///< pointer to ICE communication holder
        static SchedulerBase* ptr_scheduler; ///< pointer to a scheduler
        static Configuration* ptr_config; ///< pointer to configuration
        static int counter; ///< counter of instance
        /** 
          * @brief Send a batch of vectors to another node on network
          * @remarks
          *     Triggered inside the push() call when the size of the queue 
          *     reaches num_batch_size if enum_role=CLIENT
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool send_batch(); 
        /** 
          * @brief Run the registered matching function 
          * @remarks  
          *     Triggered inside the push() call when the size of the queue 
          *     reaches num_batch_size if enum_role=SERVER
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool do_matching();
    public:
        /** 
          * @brief Remove n items from the frontd
          * @param n  The number of the items to remove
          * @return bool
          *     @retval false fail
          *     @retval true success
          */
        bool remove_front(size_t n);
        FeatureQueue() = delete; ///< disable the default constructor (C++11 or newer)
        /** 
          * @brief Constructuor
          */
        FeatureQueue(QueueRole role);
        /** 
          * @brief Constructuor
          */
        FeatureQueue(FeatureQueue& feature);
        /** 
          * @brief Deconstructuor
          */
        ~FeatureQueue();
        /** 
          * @brief append an feature to the end of the queue
          * @param feature       Reference of an StreamObject object @class StreamObject
          * @return bool
          *     -<em>false</em> fail
          *     -<em>True</em> success
          */
        bool push(Feature& feature);
        
        /** 
          * @brief retrive an feature from the top of the queue
          * @param Object       Reference of an StreamObject object @class StreamObject
          * @return bool
          *     @retval false   No data send or inference event is triggered
          *     @retval true   A data send or inference event is triggered
          */
        Feature_ptr_t pop();
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
        void reg_matching_fun(MatchingFun_t ptr_fun);
        
};

#endif