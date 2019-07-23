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
*  @file     object_queue.cpp                                                       *
*  @brief    head file for Object classes                                     *
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

#include "object_queue.hpp"
#include <iostream>
#include <mutex>
#include <thread>
#include <default.hpp>
#include <remote_interface.hpp>

int ObjectQueue::counter=0;
Ice::CommunicatorHolder* ObjectQueue::ptr_ich; 
SchedulerBase* ObjectQueue::ptr_scheduler; 
Configuration* ObjectQueue::ptr_config;

ObjectQueue::ObjectQueue(QueueRole role):enum_role(role), num_batch_size(1) {
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

ObjectQueue::ObjectQueue(ObjectQueue& o){
    enum_role = o.enum_role;
    num_batch_size = o.num_batch_size;
    for (auto& original_obj:o.data_queue) {
        auto *ptr_new_obj = new Object(*original_obj);
        data_queue.push_back(ptr_new_obj);
    }
    ptr_classification_fun = o.ptr_classification_fun;
}

ObjectQueue::~ObjectQueue(){
    for(ObjectData_t::iterator it=data_queue.begin(); it!=data_queue.end(); it++) {
       if (*it != nullptr) delete *it;
    }
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

bool ObjectQueue::push(Object &object){
    Object_ptr_t ptr_clone_Object = new Object(object);
    data_queue.push_back(ptr_clone_Object);

    bool event_triggered = (data_queue.size()>=num_batch_size);
    if (event_triggered) {
        switch (enum_role) {
            case CLIENT:
                send_batch();
                break;
            case SERVER:
                do_classification();
                break;
        }
    }
    return event_triggered;
}

Object_ptr_t ObjectQueue::pop(){
    if (!size()) return nullptr; // queue is empty

    // pop front
    ObjectData_t::iterator it_first = data_queue.begin();

    // remove from the queue
    data_queue.erase(it_first);

    return *it_first;;
}

size_t ObjectQueue::size(){
    return data_queue.size(); 
}

void ObjectQueue::set_batch_size(uint16_t batch_size){
    num_batch_size = batch_size;
}

void ObjectQueue::reg_classification_fun(ClassificationFun_t ptr_fun) {
    ptr_classification_fun = ptr_fun; 
}

bool ObjectQueue::remove_front(size_t n){
    if (n>data_queue.size()) return false; // too many items to remove

    for (int i=0; i<n; i++) pop();

    return true;
}

bool ObjectQueue::send_batch(){
    try {
        // connect to server
        auto classifier_node = ptr_scheduler->next_available();
        std::string proxy_string = "Classifier -d:udp -h "+classifier_node->ip()+" -p "+std::to_string(classifier_node->port());
        Ice::CommunicatorHolder ich(ICECLIENTCONFIGFILE);
        auto base = ich->stringToProxy(proxy_string);
        auto processor = Ice::uncheckedCast<remote::ClassifierInterfacePrx>(base);

        if(!processor)
        {
            throw std::runtime_error("Invalid proxy");
        }

        for (int i=0; i<num_batch_size; i++) {
            Object_ptr_t ptr_object = pop();
            // send a object
            auto size = ptr_object->get_image()->size();
            int  aligned_pieces = size/UDP_PACKET_SIZE;  
            auto nonaligned_size = size -aligned_pieces*UDP_PACKET_SIZE;
            int  nonaligned_pieces = nonaligned_size>0? 1:0;
            auto total_pieces = aligned_pieces + nonaligned_pieces; 
            std::pair<const Ice::Byte*, const Ice::Byte*> ptr_bytes;
            ptr_bytes.first = reinterpret_cast<Ice::Byte*>(ptr_object->get_image()->image());
            ptr_bytes.second = ptr_bytes.first + UDP_PACKET_SIZE;

            BBox_ptr_t ptr_bbox = ptr_object->get_bbox();
            StreamImage_ptr_t ptr_image = ptr_object->get_image();
            for (int i=0 ; i<aligned_pieces; i++) {
                processor->classify(ptr_image->id(),ptr_image->timestamp(),
                                    ptr_image->width(), ptr_image->height(),
                                    ptr_image->quality(), ptr_image->color(),
                                    ptr_image->format(), total_pieces, size, i, ptr_bytes , 
                                    UDP_PACKET_SIZE, ptr_bbox->top, ptr_bbox->bottom,
                                    ptr_bbox->left,ptr_bbox->right);
                ptr_bytes.first =  ptr_bytes.second;
                ptr_bytes.second = ptr_bytes.first + UDP_PACKET_SIZE;
            }
            if (nonaligned_pieces) {
                auto nonaligned_size = size -aligned_pieces*UDP_PACKET_SIZE;
                ptr_bytes.second = ptr_bytes.first + nonaligned_size;
                processor->classify(ptr_image->id(),ptr_image->timestamp(),
                                    ptr_image->width(), ptr_image->height(),
                                    ptr_image->quality(), ptr_image->color(),
                                    ptr_image->format(), total_pieces-1, size, i, ptr_bytes , 
                                    nonaligned_size, ptr_bbox->top, ptr_bbox->bottom,
                                    ptr_bbox->left,ptr_bbox->right);
            }
        }
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return true;
}

bool ObjectQueue::do_classification(){
    if (nullptr == ptr_classification_fun ) {
        std::cerr<<"error: inference call back function is not registered"<<std::endl;
        throw std::exception();
        return false;
    }

    ObjectData_t batch_Objects;
    for (int i=0; i<num_batch_size; i++) {
        Object_ptr_t ptr_Object = pop();
        batch_Objects.push_back(ptr_Object);
    }
    std::cout<<"Classification Object batch"<<std::endl;
    ptr_classification_fun(batch_Objects);

    return true;
}

