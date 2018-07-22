cmake_minimum_required (VERSION 3.4)
project (sample2)


add_executable(sample2 main.cpp)
target_link_libraries (sample2 rt  inference ocv_reader ocv_render inference_engine tbb tbbmalloc ${OpenCV_LIBS} dl )
