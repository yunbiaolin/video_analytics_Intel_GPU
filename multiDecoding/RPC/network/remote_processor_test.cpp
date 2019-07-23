#include <gtest/gtest.h>
//#include <opencv2/core.hpp>
//#include <opencv2/imgcodecs.hpp>
#include <fstream>
#include <iostream>
#include <Ice/Ice.h>
#include <cstdio>
#include <string>
#include <image_queue.hpp>
#include <object_queue.hpp>
#include <remote_interface.hpp>
#include <configuration.hpp>
#include <scheduler.hpp>
#include <utility.hpp>
#include <default.hpp>

#define SOURCEFILE "/home/lxia1/Documents/myprojects/DSSPoc/network/frame1.jpg"
#define VALIDATIONFILE "validation.jpg"


void dummy_detection(ImageData_t& ptr_images, void* object);

TEST(remote_processor, transfer_image_in_UDP) {
    int packet_size = 61440; //60*1024;
    ImageQueue queue(CLIENT);
    queue.set_batch_size(100);
    queue.reg_detection_fun(dummy_detection, nullptr);

    //read image file
    std::ifstream fin(SOURCEFILE , std::ios::in | std::ios::binary |std::ios::ate );
    std::streamsize size = fin.tellg();
    ASSERT_GT(size,0);
    fin.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    fin.read(buffer.data(), size);

    
    try{
        Configuration conf;
        conf.init_from_json(SYS_CONFIGFILE);

        RoundRobinScheduler rs("Detector");
        std::vector<NodeWithState_ptr_t> nodes;
        conf.get_nodes(nodes);
        rs.attach(&nodes);

        auto detector_node = rs.next_available();
        std::string proxy_string = "Detector -d:udp -h "+detector_node->ip()+" -p "+std::to_string(detector_node->port());
        std::cout<<"Proxy: "<<proxy_string<<std::endl;
        Ice::CommunicatorHolder ich(ICECLIENTCONFIGFILE);
        auto base = ich->stringToProxy(proxy_string);
        auto processor = Ice::uncheckedCast<remote::DetectorInterfacePrx>(base);

        if(!processor)
        {
            throw std::runtime_error("Invalid proxy");
        }
    
        std::remove(VALIDATIONFILE);
    
        using Clock = std::chrono::high_resolution_clock;
        long long int time_since_epoch = Clock::now().time_since_epoch().count();

        int  aligned_pieces = size/packet_size;  
        auto nonaligned_size = size -aligned_pieces*packet_size;
        int  nonaligned_pieces = nonaligned_size>0? 1:0;
        auto total_pieces = aligned_pieces + nonaligned_pieces; 
        std::pair<const Ice::Byte*, const Ice::Byte*> ptr_bytes;
        ptr_bytes.first = reinterpret_cast<Ice::Byte*>(buffer.data());
        ptr_bytes.second = ptr_bytes.first + packet_size;

        /*
        using Clock = std::chrono::high_resolution_clock;
        constexpr auto num = Clock::period::num;
        constexpr auto den = Clock::period::den;
        std::cout << Clock::now().time_since_epoch().count() << " [" << num << '/' << den << "] units since epoch\n"; */

        for (int i=0 ; i<aligned_pieces; i++) {
            processor->detect(1,time_since_epoch, 1008, 567, 75, 2, 1, total_pieces, size, i, ptr_bytes , packet_size);
            ptr_bytes.first =  ptr_bytes.second;
            ptr_bytes.second = ptr_bytes.first + packet_size;
        }
        if (nonaligned_pieces) {
            auto nonaligned_size = size -aligned_pieces*packet_size;
            ptr_bytes.second = ptr_bytes.first + nonaligned_size;
            processor->detect(1,time_since_epoch, 1008, 567, 75, 2, 1, total_pieces, size, total_pieces-1, ptr_bytes , nonaligned_size);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

}

void dummy_classification(ObjectData_t& ptr_objects);
TEST(remote_processor, transfer_object_in_UDP) {
    
    ObjectQueue queue(CLIENT);
    queue.set_batch_size(100);
    queue.reg_classification_fun(dummy_classification);

    //read image file
    std::ifstream fin(SOURCEFILE , std::ios::in | std::ios::binary |std::ios::ate );
    std::streamsize size = fin.tellg();
    ASSERT_GT(size,0);
    fin.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    fin.read(buffer.data(), size);

    
    try{
        Configuration conf;
        conf.init_from_json(SYS_CONFIGFILE);

        RoundRobinScheduler rs("Classifier");
        std::vector<NodeWithState_ptr_t> nodes;
        conf.get_nodes(nodes);
        rs.attach(&nodes);

        auto detector_node = rs.next_available();
        std::string proxy_string = "Classifier -d:udp -h "+detector_node->ip()+" -p "+std::to_string(detector_node->port());
        std::cout<<"Proxy: "<<proxy_string<<std::endl;
        Ice::CommunicatorHolder ich(ICECLIENTCONFIGFILE);
        auto base = ich->stringToProxy(proxy_string);
        auto processor = Ice::uncheckedCast<remote::ClassifierInterfacePrx>(base);

        if(!processor)
        {
            throw std::runtime_error("Invalid proxy");
        }
    
        std::remove(VALIDATIONFILE);
    
        using Clock = std::chrono::high_resolution_clock;
        long long int time_since_epoch = Clock::now().time_since_epoch().count();

        int  aligned_pieces = size/UDP_PACKET_SIZE;  
        auto nonaligned_size = size -aligned_pieces*UDP_PACKET_SIZE;
        int  nonaligned_pieces = nonaligned_size>0? 1:0;
        auto total_pieces = aligned_pieces + nonaligned_pieces; 
        std::pair<const Ice::Byte*, const Ice::Byte*> ptr_bytes;
        ptr_bytes.first = reinterpret_cast<Ice::Byte*>(buffer.data());
        ptr_bytes.second = ptr_bytes.first + UDP_PACKET_SIZE;

        /*
        using Clock = std::chrono::high_resolution_clock;
        constexpr auto num = Clock::period::num;
        constexpr auto den = Clock::period::den;
        std::cout << Clock::now().time_since_epoch().count() << " [" << num << '/' << den << "] units since epoch\n"; */

        int left = 0;
        int right = 10;
        int top = 5;
        int bottom= 20;

        for (int i=0 ; i<aligned_pieces; i++) {
            processor->classify(1,time_since_epoch, 1008, 567, 75, 2, 1, total_pieces, size, i, ptr_bytes , UDP_PACKET_SIZE, top, bottom, left, right);
            ptr_bytes.first =  ptr_bytes.second;
            ptr_bytes.second = ptr_bytes.first + UDP_PACKET_SIZE;
        }
        if (nonaligned_pieces) {
            auto nonaligned_size = size -aligned_pieces*UDP_PACKET_SIZE;
            ptr_bytes.second = ptr_bytes.first + nonaligned_size;
            processor->classify(1,time_since_epoch, 1008, 567, 75, 2, 1, total_pieces, size, total_pieces-1, ptr_bytes , nonaligned_size, top, bottom, left, right);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

}
