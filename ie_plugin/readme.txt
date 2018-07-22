This project includes two parts

samples
  - sub project with 
      - object detection application

ie_plugin
  - wrapper for Inference Engine
  - inut format converter

Build
  - Download and install the latest CV SDK
  - Create two environment variables for DL SDK includes and libraries paths: DL_SDK_INCLUDE, DL_SDK_LIB
    Variables will be used to resolve DL SDK dependencies.

  - mkdir build && cd build
  - cmake ..
  - make all