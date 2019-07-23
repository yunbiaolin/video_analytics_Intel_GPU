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
*  @file     configuration.cpp                                               *
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

#include "configuration.hpp"
#include "image.hpp"
#include <string>
#include <vector>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>
#include <cinttypes>
#include <assert.h>

#define STREAMKEY "Streams"
#define STREAMIDKEY "ID"
#define STREAMWIDTH "Width"
#define STREAMHEIGHT "Height"
#define STREAMQUALITY "Quality"
#define STREAMDESC "Desc"

#define NODEKEY "Processing Nodes"
#define NODETYPE "Type"
#define NODEIP "IPAddr"
#define NODEPORT "Port"

NodeWithState::NodeWithState(NodeWithState& n){
    str_type  = n.str_type;
    str_IP = n.str_IP;
    num_port = n.num_port;
}


//Node::Node(const std::string type, const std::string IP, const uint32_t port):  {}

NodeWithState::NodeWithState(std::string &type, std::string &IP, uint32_t port){
    str_type = type;
    str_IP = IP;
    num_port = port;
}

Configuration::Configuration(){}

Configuration::Configuration(Configuration & c){
    std::vector<NodeWithState_ptr_t> nodes;
    c.get_nodes(nodes);
    for (auto& node:nodes) {
        auto *new_node = new NodeWithState(*node);
        this->nodes.push_back(new_node);
    }

    std::vector<StreamImage_ptr_t> streams;
    c.get_streams(streams);
    for (auto stream:streams) {
        auto *new_stream = new StreamImage(*stream);
        this->streams.push_back(new_stream);
    }
}

Configuration::~Configuration(){
    // iterate to free camera streams information memory
    for (std::vector<StreamImage_ptr_t>::iterator it = streams.begin(); it != streams.end(); it++ ) {
        StreamImage_ptr_t ptr_stream = *it;
        delete ptr_stream;
    }
    // iterate to free nodes information memory
    for (std::vector<NodeWithState_ptr_t>::iterator it = nodes.begin(); it != nodes.end(); it++ ) {
        NodeWithState_ptr_t ptr_node = *it;
        delete ptr_node;
    }
}

bool Configuration::init_from_json(const std::string &filename) {
    std::ifstream in(filename);
    //assert(in.is_open());

    Json::Reader reader;
    Json::Value root; // will contains the root value after parsing.

    if (!reader.parse(in, root, false)){
        std::cerr << "configuration parser failed \n";
        return false;
    }

    //parse the stream configurations
    const Json::Value streams_root = root[STREAMKEY];
    for (int index = 0; index <streams_root.size(); ++index) {
        uint32_t id = streams_root[index].get(STREAMIDKEY, 0).asUInt();
        uint32_t width = streams_root[index].get(STREAMWIDTH,0).asUInt();
        uint32_t height = streams_root[index].get(STREAMHEIGHT,0).asUInt();
        uint8_t quality = streams_root[index].get(STREAMQUALITY,100).asUInt();
        std::string desc = streams_root[index].get(STREAMDESC,"").asString();
        StreamImage *ptr_stream = new StreamImage(id, desc, 0L, width, height, quality);
        streams.push_back(ptr_stream);  // store the parsed stream configuration 
        //std::cout<<id<<" "<<width<<" "<<height<<" "<<int(quality)<<" "<<desc<<std::endl;
    }

    //parse the node configurations
    const Json::Value nodes_root = root[NODEKEY];
    for (int index = 0; index <nodes_root.size(); ++index) {
        std::string type = nodes_root[index].get(NODETYPE, "").asString();
        std::string ip = nodes_root[index].get(NODEIP, "").asString();
        uint32_t port = nodes_root[index].get(NODEPORT, "").asUInt();
        NodeWithState *ptr_node = new NodeWithState(type, ip, port);
        nodes.push_back(ptr_node);  // store the parsed node configuration 
        //std::cout<<type<<" "<<ip<<" "<<int(port)<<std::endl;
    }

    return true;
}

int Configuration::size_of_streams(){
    return this->streams.size();
}

int Configuration::size_of_nodes(){
    return this->nodes.size();
}

void Configuration::get_streams(std::vector<StreamImage_ptr_t> &streams){
    streams.assign(this->streams.begin(), this->streams.end());
}

void Configuration::get_nodes(std::vector<NodeWithState_ptr_t> &nodes){
    nodes.assign(this->nodes.begin(), this->nodes.end());
}

bool NodeWithState::is_alive() {
    return b_alive;
}

std::string NodeWithState::ip() {
    return str_IP;
}

uint32_t NodeWithState::port() {
    return num_port;
}

void NodeWithState::disable(){
    b_alive=false;
}

std::string NodeWithState::type(){
    return str_type;
}
