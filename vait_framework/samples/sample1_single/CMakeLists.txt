cmake_minimum_required (VERSION 3.4)
project (sample1)


add_executable(sample1 main.cpp)
target_link_libraries (sample1 rt  inference ocv_reader ocv_render inference_engine tbb tbbmalloc ${OpenCV_LIBS} dl )
