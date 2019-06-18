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
*  @file     image_queue.cpp                                                       *
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

#include "image_queue.hpp"
#include <iostream>
#include <mutex>
#include <thread>
#if USE_ICE
#include <Ice/Ice.h>
#include <remote_interface.hpp>
#endif
#include <configuration.hpp>
#include <utility.hpp>
#include <default.hpp>

int ImageQueue::counter=0;
#if USE_ICE
Ice::CommunicatorHolder* ImageQueue::ptr_ich; 
#endif
SchedulerBase* ImageQueue::ptr_scheduler; 
Configuration* ImageQueue::ptr_config;

ImageQueue::ImageQueue(QueueRole role):enum_role(role), num_batch_size(1) {
    counter++;
    // get node inforamtion from configuration file
    if (ptr_config == nullptr) {
        ptr_config = new Configuration();
        ptr_config->init_from_json(SYS_CONFIGFILE);
    }

    switch(role){
        case CLIENT:
#if USE_ICE        
            if (ptr_ich == nullptr) ptr_ich = new Ice::CommunicatorHolder(ICECLIENTCONFIGFILE);
#endif

            break;
        case SERVER:
            break;
    }
}

ImageQueue::~ImageQueue(){
    for(ImageData_t::iterator it=data_queue.begin(); it!=data_queue.end(); it++) {
       delete *it;
    }
    counter --;
    if (counter==0) {
#if USE_ICE        
        if (ptr_ich != nullptr) {
            delete ptr_ich;
            ptr_ich=nullptr;
        }
#endif
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

bool ImageQueue::push(StreamImage &image){
    StreamImage_ptr_t ptr_clone_image = new StreamImage(image.id(),image.desc(),image.timestamp(),image.width(), image.height(), image.quality());
    ptr_clone_image->set_image(image.image(),image.size());
    data_queue.push_back(ptr_clone_image);

    bool event_triggered = (data_queue.size()>=num_batch_size);
    if (event_triggered) {
        switch (enum_role) {
            case CLIENT:
                send_batch();
                break;
            case SERVER:
                do_detection();
                break;
        }
    }
    return event_triggered;
}

StreamImage_ptr_t ImageQueue::pop(){
    if (size()==0) return nullptr; // queue is empty

    // pop front
    ImageData_t::iterator it_first = data_queue.begin();

    // remove from the queue
    data_queue.erase(it_first);

    return *it_first;;
}

size_t ImageQueue::size(){
    return data_queue.size(); 
}

void ImageQueue::set_batch_size(uint16_t batch_size){
    num_batch_size = batch_size;
}

void ImageQueue::reg_detection_fun(DetectionFun_t ptr_fun) {
    ptr_detection_fun = ptr_fun; 
}

void ImageQueue::set_scheduler(SchedulerBase* ptr_s) {
    ptr_scheduler = ptr_s; 
}

bool ImageQueue::remove_front(size_t n){
    if (n>data_queue.size()) return false; // too many items to remove

    for (int i=0; i<n; i++) pop();
/*    // free the image buffers
    for (ImageData_t::iterator it = data_queue.begin(); it<data_queue.begin()+n; it++) {
        StreamImage_ptr_t ptr_image = *it;
        ptr_image->delete_image_buffer();
    }
    // remove the items 
    for (size_t i=0; i<n; i++) {
        data_queue.erase(data_queue.begin());
    } */
    return true;
}

bool ImageQueue::send_batch(){

    try {
        // connect to server
#if USE_ICE        
        auto detector_node = ptr_scheduler->next_available();
        std::string proxy_string = "Detector -d:udp -h "+detector_node->ip()+" -p "+std::to_string(detector_node->port());
        Ice::CommunicatorHolder ich(ICECLIENTCONFIGFILE);
        auto base = ich->stringToProxy(proxy_string);
        auto processor = Ice::uncheckedCast<remote::DetectorInterfacePrx>(base);

        if(!processor)
        {
            throw std::runtime_error("Invalid proxy");
        }

        for (int i=0; i<num_batch_size; i++) {
            StreamImage_ptr_t ptr_image = pop();
            // send a image
            auto size = ptr_image->size();
            char* buffer = ptr_image->image();
            int  aligned_pieces = size/UDP_PACKET_SIZE;  
            auto nonaligned_size = size -aligned_pieces*UDP_PACKET_SIZE;
            int  nonaligned_pieces = nonaligned_size>0? 1:0;
            auto total_pieces = aligned_pieces + nonaligned_pieces; 
            std::pair<const Ice::Byte*, const Ice::Byte*> ptr_bytes;
            ptr_bytes.first = reinterpret_cast<Ice::Byte*>(buffer);
            ptr_bytes.second = ptr_bytes.first + UDP_PACKET_SIZE;

            auto stream_id = ptr_image->id();
            auto timstamp = ptr_image->timestamp();
            auto width = ptr_image->width();
            auto height = ptr_image->height();
            auto quality = ptr_image->quality();
            auto color = ptr_image->color();
            auto format = ptr_image->format();
            // start sending
            for (int i=0 ; i<aligned_pieces; i++) {
                processor->detect(stream_id,timstamp, width, height, quality, color, format, total_pieces, size, i, ptr_bytes , UDP_PACKET_SIZE);
                ptr_bytes.first =  ptr_bytes.second;
                ptr_bytes.second = ptr_bytes.first + UDP_PACKET_SIZE;
            }
            if (nonaligned_pieces) {
                auto nonaligned_size = size -aligned_pieces*UDP_PACKET_SIZE;
                ptr_bytes.second = ptr_bytes.first + nonaligned_size;
                processor->detect(stream_id,timstamp, width, height, quality, color , format, total_pieces, size, total_pieces-1, ptr_bytes , nonaligned_size);
            }
        }
#endif
    }
    catch(const std::exception& e){
            std::cerr << e.what() << std::endl;
    }
    return true;
}

bool ImageQueue::do_detection(){
    if (nullptr == ptr_detection_fun) {
        std::cerr<<"error: inference call back function is not registered"<<std::endl;
        throw std::exception();
        return false;
    }

    ImageData_t batch_images;
    for (int i=0; i<num_batch_size; i++) {
        StreamImage_ptr_t ptr_image = pop();
        batch_images.push_back(ptr_image);
    }
    std::cout<<"Detection image batch"<<std::endl;
    ptr_detection_fun(batch_images);

    return true;
}

Configuration* ImageQueue::get_config(){
    return ptr_config;
}
