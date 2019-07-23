#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <object_queue.hpp>

void dummy_classification(ObjectData_t& objects) {
    std::cout<<"classification trigered"<<std::endl;
};
