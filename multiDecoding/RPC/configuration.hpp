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
*  @file     configuration.hpp                                               *
*  @brief    Head file for image classes                                     *
*  @author   Lei.Xia                                                         *
*  @email    lei.xia@intel.com                                               *
*  @version  0.0.0.1                                                         *
*  @date     Mar 25, 2019                                                    *
*  @license  Intel NDA                                                       *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : Define the classes for image operation from decoded video*
*                   streams                                                  *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version> | <Author>       | <Description>                   *
*----------------------------------------------------------------------------*
*  2019/03/25 | 0.0.0.1   | Lei.Xia        | Create file                     *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef __DSS_POC_CONFIGURATION_HPP__
#define __DSS_POC_CONFIGURATION_HPP__

#include "image.hpp"
#include <string>
#include <vector>

/** 
  * @brief calss of a inference node
  */
struct Node {
  std::string str_type; ///< Type of the node
  std::string str_IP; ///< IP address
  uint32_t num_port; ///< a UDP port to recieve the data
};

/** 
  * @brief calss of the states of a node
  */
struct State {
    State():b_alive(true){}
    bool b_alive; ///< true if a node is reachable, otherwise false. default is true.
}; 

/** 
  * @brief calss to integrate node and its state 
  */
class NodeWithState: private Node, private State {
  public:
      /** 
      * @brief The constructor 
      */
    NodeWithState(NodeWithState&); ///< constructor
      /** 
      * @brief The constructor 
      * @param type  type of the node
      * @param IP    IP address in string
      * @param port  port number
      */
    NodeWithState(std::string &type, std::string &IP, uint32_t port); ///< constructor
    /** 
      * @brief Get node aliveness
      * @return bool
      *     @retval 0 dead
      *     @retval 1 alive
      */
    bool is_alive();
    /** 
      * @brief Get IP address of the node
      * @return IP address
      */
    std::string ip();
    /** 
      * @brief Get communication port of the node
      * @return a UDP port
      */
    uint32_t port();
    /** 
      * @brief Disable the node by setting b_alive to false
      */
    void disable();
    /** 
      * @brief Get the node type
      * @return Node type
      */
    std::string type();
};

typedef NodeWithState* NodeWithState_ptr_t;

/** 
  * @brief calss configuration of all inference nodes to transmit data to
  */
class Configuration{
  private:
    std::vector<NodeWithState_ptr_t> nodes; ///< vector to store information of all inferencing nodes 
    std::vector<StreamImage_ptr_t> streams; ///< vector to store information of camera streams 
  public:
    /** 
      * @brief The constructor 
      */
    Configuration();
    /** 
      * @brief The constructor 
      */
    Configuration(Configuration&);
    /** 
      * @brief The deconstructor 
      */
    ~Configuration();
    /** 
      * @brief read configuration from a json file
      * @param filename    json configuration filename.
      * @return bool
      *     @retval 0 fail
      *     @retval 1 success
      */
    bool init_from_json(const std::string &filename);
    /** 
      * @brief Get the size of streams
      * @return Number of streams
      */
    int size_of_streams();
    /** 
      * @brief Get the size of nodes
      * @return Number of streams
      */
    int size_of_nodes();
    /** 
      * @brief Get camera stream configurations
      * @param streams    vector of camera stream configurations
      */
    void get_streams(std::vector<StreamImage_ptr_t>&streams);
    /** 
      * @brief Get node configurations
      * @param nodes    vector of node configurations
      */
    void get_nodes(std::vector<NodeWithState_ptr_t> &nodes);
};

#endif