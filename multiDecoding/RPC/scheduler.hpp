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
*  @file     scheduler.hpp                                                       *
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

#ifndef __DSS_POC_SCHEDULER_HPP__
#define __DSS_POC_SCHEDULER_HPP__

#include "configuration.hpp"

/** 
  * @brief Base calss of schedulers
  */
class SchedulerBase {
  public:
    /** 
      * @brief Identify and return next available node based on the scheduler-based strategy
      * @return Node    @class Node
      */
    virtual NodeWithState_ptr_t next_available()=0 ;
    /** 
      * @brief Get number of available nodes
      * @return number of available nodes
      */
    virtual uint16_t available_size() = 0;
    virtual void attach(std::vector<NodeWithState_ptr_t> *ptr_nodes)=0;
};

/** 
  * @brief A simple example scheduler based on round-robin strategy
  */
class RoundRobinScheduler: public SchedulerBase {
  private:
    std::string str_type; ///< type of the scheduler 
    int current_index; ///< the index to the current availiable resource
    std::vector<NodeWithState_ptr_t> *ptr_nodes; ///< point to the node resource vector
  public:
    RoundRobinScheduler(std::string type); ///< The default constructor
    void attach(std::vector<NodeWithState_ptr_t> *ptr_nodes) override;
    /** 
      * @brief Get the next available node based on the round-robin strategy
      * @return a point to the node    @class NodeWidthState
      *     @retval nullptr  no node available
      *     @retval other the next available node
      */
    NodeWithState_ptr_t next_available() override;
    /** 
      * @brief Get number of available nodes
      * @return number of available nodes
      */
    uint16_t available_size() override;

};

#endif