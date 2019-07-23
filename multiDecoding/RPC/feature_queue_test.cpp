#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <feature_queue.hpp>

void dummy_matching(Features_t& features) {
    std::cout<<"matching trigered"<<std::endl;
};

