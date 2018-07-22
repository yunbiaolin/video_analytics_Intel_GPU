cmake_minimum_required (VERSION 3.4)
project (worker_test)


add_executable(worker_test main.cpp)
target_link_libraries (worker_test rt  inference ocv_reader ocv_render file_render inference_engine tbb tbbmalloc ${OpenCV_LIBS} dl )
