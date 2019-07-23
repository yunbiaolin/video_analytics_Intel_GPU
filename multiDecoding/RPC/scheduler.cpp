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
*  @file     scheduler.cpp                                                       *
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

#include "scheduler.hpp"
#include "configuration.hpp"
#include <iostream>

RoundRobinScheduler::RoundRobinScheduler(std::string type):current_index(-1),str_type(type) {};

void RoundRobinScheduler::attach(std::vector<NodeWithState_ptr_t>* ptr_nodes){
    this->ptr_nodes = ptr_nodes;
};

NodeWithState_ptr_t RoundRobinScheduler::next_available(){
    int previous_index = current_index++;
    //std::cout<<"previous: "<<previous_index<<std::endl;

    // move to the next node
    current_index = current_index % ptr_nodes->size();
    NodeWithState *ptr_node = ptr_nodes->at(current_index);
    //std::cout<<"current: "<<current_index<<std::endl;

    // if it is a dead node or different type, move to next until there is alive node
    while(!ptr_node->is_alive() || ptr_node->type()!= str_type) {
        current_index++;
        current_index = current_index % ptr_nodes->size();
        if (current_index == previous_index) {
            // no other alive node
            ptr_node = ptr_nodes->at(current_index);
            break;
        }
        ptr_node = ptr_nodes->at(current_index);
    }
    //std::cout<<"current: "<<current_index<<std::endl;
    return ptr_node;
}

uint16_t RoundRobinScheduler::available_size(){
    uint16_t count=0;

    for (std::vector<NodeWithState_ptr_t>::iterator it = ptr_nodes->begin(); it!=ptr_nodes->end(); it++){
        NodeWithState *ptr_node = *it;
        if (ptr_node->is_alive() && ptr_node->type()== str_type) count++;
    }
    return count;

}