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
*  @file     feature_queue.cpp                                                       *
*  @brief    head file for Object classes                                     *
*  @author   Lei.Xia                                                         *
*  @email    lei.xia@intel.com                                               *
*  @version  0.0.0.1                                                         *
*  @date     Mar 25, 2019                                                    *
*  @license  Intel NDA                                                       *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : Define the class of FeatureQueue                         *
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version> | <Author>       | <Description>                   *
*----------------------------------------------------------------------------*
*  2019/03/25 | 0.0.0.1   | Lei.Xia        | Create file                     *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#include <feature_queue.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <default.hpp>
#include <remote_interface.hpp>
#include <utility.hpp>

 int FeatureQueue::counter=0;
 Ice::CommunicatorHolder* FeatureQueue::ptr_ich; 
 SchedulerBase* FeatureQueue::ptr_scheduler; 
 Configuration* FeatureQueue::ptr_config;

 FeatureQueue::FeatureQueue(QueueRole role):enum_role(role), num_batch_size(1) {
    counter++;
    // get node inforamtion from configuration file
    if (ptr_config == nullptr) {
        ptr_config = new Configuration();
        ptr_config->init_from_json(SYS_CONFIGFILE);
    }

    switch(role){
        case CLIENT:
            if (ptr_ich == nullptr) ptr_ich = new Ice::CommunicatorHolder(ICECLIENTCONFIGFILE);

            break;
        case SERVER:
            break;
    }

}

 FeatureQueue::FeatureQueue(FeatureQueue& f){
    enum_role = f.enum_role;
    num_batch_size = f.num_batch_size;
    features=f.features;
    ptr_matching_fun = f.ptr_matching_fun;
}

 FeatureQueue::~FeatureQueue(){
    counter --;
    if (counter==0) {
        if (ptr_ich != nullptr) {
            delete ptr_ich;
            ptr_ich=nullptr;
        }
        if (ptr_config != nullptr) {
            delete ptr_config;
            ptr_config=nullptr;
        }
        if (ptr_scheduler != nullptr) {
            delete ptr_scheduler;
            ptr_scheduler=nullptr;
        }
    }
}

 bool FeatureQueue::push(Feature &feature){
    features.push_back(feature);

    bool event_triggered = (features.size()>=num_batch_size);
    if (event_triggered) {
        switch (enum_role) {
            case CLIENT:
                send_batch();
                break;
            case SERVER:
                do_matching();
                break;
        }
    }
    return event_triggered;
}

 Feature_ptr_t FeatureQueue::pop(){
    if (!size()) return nullptr; // queue is empty

    // pop front
    Feature_ptr_t ptr_feature = new Feature;
    *ptr_feature = features.at(0);

    // remove from the queue
    features.erase(features.begin());

    return ptr_feature;
}

 size_t FeatureQueue::size(){
    return features.size(); 
}

 void FeatureQueue::set_batch_size(uint16_t batch_size){
    num_batch_size = batch_size;
}

 void FeatureQueue::reg_matching_fun(MatchingFun_t ptr_fun) {
    ptr_matching_fun = ptr_fun; 
}

 bool FeatureQueue::remove_front(size_t n){
    if (n>features.size()) return false; // too many items to remove

    for (int i=0; i<n; i++) pop();

    return true;
}

 bool FeatureQueue::send_batch(){
    try {
        // connect to server
        auto matching_node = ptr_scheduler->next_available();
        std::string proxy_string = "Matching -d:udp -h "+matching_node->ip()+" -p "+std::to_string(matching_node->port());
        Ice::CommunicatorHolder ich(ICECLIENTCONFIGFILE);
        auto base = ich->stringToProxy(proxy_string);
        auto processor = Ice::uncheckedCast<remote::MatchingInterfacePrx>(base);

        if(!processor)
        {
            throw std::runtime_error("Invalid proxy");
        }

        auto size_of_feature_struct = sizeof(Feature);
        int aligned = size_of_feature_struct*num_batch_size/UDP_PACKET_SIZE;
        auto size_of_block= (num_batch_size/aligned)*size_of_feature_struct;
        auto noaligned = (aligned*UDP_PACKET_SIZE!=size_of_feature_struct*num_batch_size)? true: false;
        auto total_size = features.size()*size_of_feature_struct;
        auto total_pieces = aligned+(noaligned? 1:0);

        for (int i=0;i<num_batch_size;i++) {
            auto feature = features.at(i);
            std::pair<const Ice::Byte*, const Ice::Byte*> ptr_bytes;
            ptr_bytes.first =  feature.ptr_feature;
            ptr_bytes.second = ptr_bytes.first + FEATURE_LENGTH;
            processor->match(feature.num_stream_id, feature.num_timestamp,
                    feature.bbox.top, feature.bbox.bottom, feature.bbox.left, feature.bbox.right,
                    ptr_bytes);
        }
        // remove the data after sending
        features.erase(features.begin(), features.begin()+num_batch_size);
    }
    catch(const std::exception& e)
        {
        std::cerr << e.what() << std::endl;
    }
    return true;
}

 bool FeatureQueue::do_matching(){
    if (nullptr == ptr_matching_fun ) {
        std::cerr<<"error: inference call back function is not registered"<<std::endl;
        throw std::exception();
        return false;
    }

    Features_t batch_features;
    batch_features.assign(features.begin(),features.begin()+num_batch_size);
    remove_front(num_batch_size);

    std::cout<<"Classification Object batch"<<std::endl;
    ptr_matching_fun(batch_features);

    return true;
}

