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

ImageQueue::ImageQueue(QueueRole role):enum_role(role), num_batch_size(1) {}

ImageQueue::~ImageQueue(){
    for(ImageData_t::iterator it=data_queue.begin(); it!=data_queue.end(); it++) {
       delete *it;
    }
}

bool ImageQueue::push(StreamImage &image){
    StreamImage_ptr_t ptr_clone_image = new StreamImage(image.id(),image.desc(),image.width(), image.height(), image.quality());
    ptr_clone_image->set_image(image.image(),image.size());
    data_queue.push_back(ptr_clone_image);
    bool event_triggered = (data_queue.size()>=num_batch_size);
    if (event_triggered) {
        switch (enum_role) {
            case CLIENT:
                send_batch();
                break;
            case SERVER:
                do_inference();
                break;
        }
    }
    return event_triggered;
}

StreamImage_ptr_t ImageQueue::pop(){
    if (!size()) return nullptr; // queue is empty

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

void ImageQueue::reg_inference_fun(InferenceFun_t ptr_fun) {
    ptr_inference_fun = ptr_fun; 
}

void ImageQueue::reg_scheduler_fun(SchedulerFun_t ptr_fun) {
    ptr_scheduler_fun = ptr_fun; 
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
}

bool ImageQueue::send_batch(){
    if (nullptr == ptr_scheduler_fun) {
        std::cerr<<"error: inference call back function is not registered"<<std::endl;
        throw std::exception();
        return false;
    }
    for (int i=0; i<num_batch_size; i++) {
        StreamImage_ptr_t ptr_image = pop();
        std::cout<<"Sending image (id="<<ptr_image->id()<<", desc="<<ptr_image->desc()<<")"<<std::endl;
        //Todo: send a image
    }
    return true;
}

bool ImageQueue::do_inference(){
    if (nullptr == ptr_inference_fun) {
        std::cerr<<"error: inference call back function is not registered"<<std::endl;
        throw std::exception();
        return false;
    }
    std::vector<StreamImage_ptr_t> ptr_image_array;
    for (int i=0; i<num_batch_size; i++) {
        StreamImage_ptr_t ptr_image = pop();
        ptr_image_array.push_back(ptr_image);
    }
    std::cout<<"Inferencing image batch"<<std::endl;
    //Todo: inference image. The batch is in the ptr_image_array.

    return true;
}
