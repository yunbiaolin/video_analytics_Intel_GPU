#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <image_queue.hpp>

void dummy_detection(ImageData_t& images, void* object) {
    std::cout<<"detection trigered"<<std::endl;
};

uint64_t get_timestamp(){
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000+tv.tv_usec/1000;
}

std::string uint64_to_string(uint64_t value ) {
    std::ostringstream os;
    os << value;
    return os.str();
}

TEST(image_queue, store_and_retrive_an_image) {
    ImageQueue queue(CLIENT);
    queue.set_batch_size(2);

    // set image timestamp
    uint64_t stamp = get_timestamp();

    std::string desc = "test_image";
    auto timestamp = stamp;
    StreamImage image1(1, desc, timestamp, 1920, 1080, 100);
    size_t image_size = 102400; // 100KB
    char* image_buffer = (char*) std::malloc(image_size);
    image1.set_image(image_buffer, image_size);
    image1.set_timestamp(stamp);
    ASSERT_EQ(stamp, image1.timestamp());


    //store an image
    ASSERT_FALSE(queue.push(image1)); // batch size is 2, no event should be triggered
    ASSERT_EQ(queue.size(), 1);

    //retrive the image
    StreamImage *ptr_image2=queue.pop();
    ASSERT_EQ(ptr_image2->id(), 1);
    ASSERT_EQ(ptr_image2->width(), 1920);
    ASSERT_EQ(ptr_image2->height(), 1080);
    ASSERT_EQ(ptr_image2->quality(), 100);
    ASSERT_EQ(ptr_image2->format(), UNKNOW_FORMAT);
    ASSERT_EQ(ptr_image2->size(), image_size);
    ASSERT_EQ(memcmp(image_buffer, ptr_image2->image(), image_size),0);
    ASSERT_STREQ(desc.c_str(),ptr_image2->desc().c_str());

    ASSERT_EQ(queue.size(), 0);
    
    std::free(image_buffer);
}

TEST(image_queue, pop_empty_image) {
    ImageQueue queue(CLIENT);
    ASSERT_EQ(queue.pop(), nullptr);
}

TEST(image_queue, trigger_client_data_send) {
    ImageQueue queue(CLIENT);

    RoundRobinScheduler* ptr_rs= new RoundRobinScheduler("Detector");
    std::vector<NodeWithState_ptr_t> nodes;
    queue.get_config()->get_nodes(nodes);
    ptr_rs->attach(&nodes);
    queue.set_scheduler(ptr_rs);

    queue.set_batch_size(2);
    queue.reg_detection_fun(dummy_detection, nullptr);

    //creat the 1st dummy image
    std::string desc1 = "image1";
    auto timestamp = get_timestamp();
    size_t image_size = 102400; // 100KB

    StreamImage image1(1, desc1, timestamp, 1920, 1080, 100);
    char* image1_buffer = (char*) std::malloc(image_size);
    image1.set_image(image1_buffer, image_size);
    //creat the 2nd dummy image
    std::string desc2 = "image2";
    StreamImage image2(1, desc2, timestamp, 1920, 1080, 100);
    char* image2_buffer = (char*) std::malloc(image_size);
    image2.set_image(image2_buffer, image_size);

    ASSERT_FALSE(queue.push(image1)); // batch size is 2, no event should be triggered
    ASSERT_TRUE(queue.push(image2)); //  event should be triggered

    std::free(image1_buffer);
    std::free(image2_buffer);
}

TEST(image_queue, trigger_server_inference) {
    ImageQueue queue(SERVER);
    queue.set_batch_size(2);
    queue.reg_detection_fun(dummy_detection, nullptr);

    size_t image_size = 102400; // 100KB
    auto timestamp = get_timestamp();

    //creat the 1st dummy image
    std::string desc1 = "image1";
    StreamImage image1(1, desc1, timestamp, 1920, 1080, 100);
    char* image1_buffer = (char*) std::malloc(image_size);
    image1.set_image(image1_buffer, image_size);
    //creat the 2nd dummy image
    std::string desc2 = "image2";
    StreamImage image2(1, desc2, timestamp, 1920, 1080, 100);
    char* image2_buffer = (char*) std::malloc(image_size);
    image2.set_image(image2_buffer, image_size);

    ASSERT_FALSE(queue.push(image1)); // default batch size is 1, no event should be triggered
    ASSERT_TRUE(queue.push(image2)); //  event should be triggered

    std::free(image1_buffer);
    std::free(image2_buffer);
}

TEST(image_queue, remove_front) {
    ImageQueue queue(SERVER);
    queue.set_batch_size(100);

    size_t image_size = 102400; // 100KB
    int total_dummy=99;
    auto timestamp = get_timestamp();

    //creat and push the dummy images
    for (int i=0; i<total_dummy; i++){
        std::string desc = "image"+ std::to_string(i);
        //std::cout<<desc<<std::endl;
        StreamImage image(1, desc, timestamp, 1920, 1080, 100);
        char* image_buffer = (char*) std::malloc(image_size);
        image.set_image(image_buffer, image_size);
        queue.push(image);
        std::free(image_buffer);
    }
    ASSERT_EQ(queue.size(),total_dummy);

    int num_removal=50;
    ASSERT_TRUE(queue.remove_front(num_removal));
    ASSERT_EQ(queue.size(),total_dummy-num_removal);
//    StreamImage_ptr_t ptr_image= queue.pop();
//    ASSERT_STREQ("image50",ptr_image->desc().c_str());
}
