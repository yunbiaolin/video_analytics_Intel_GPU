# Video Analytics on Intel GPU

## Introduction

 * This repository shows how to do video analytics integraton on Intel Platform with best peformance, including multiple streams decoding, video processing and inference with OpenVINO. some of codes was borrowed from Intel's VAIT code. thanks for their great work!!! you can refer more on VAIT(https://software.intel.com/en-us/articles/vaitpreview)

### Key Features:

 * Decoding and video processing with MSDK
 * Inference with GPU with OpenVINO
 * scale from 1 stream to multiple streams.  the maximum supported is 48 for 1080p now.
 * RGBP color format supported with latest MSDK + Open Source media driver
 * Docker based environment for user easy to setup the environment
 * Efficient display with XCB is introduced - Thanks Yuming.li@intel.com.
 * Workload balance between CPU and GPU is introduced.  you need to manual enable it by define ENABLE_WORKLOAD_BALANCE in main.cpp
 * Batch across differnt streams is introduced
 * MobileNetSSD as an example to show benfit of this architecture

### WIP features:
 * Further improve the workload scheduing with Intel's telemtry library
 * Add other key features after object detection into the pipeline
 
### Expected Usages:

 * Reference to learn OpenVINO, MSDK, and OpenCL
 * Reference source code about how to use MediaSDK pipe/plugin

## Requirements

 * Intel MediaSDK - https://github.com/Intel-Media-SDK/MediaSDK.git
 * Intel Media Driver - https://github.com/intel/media-driver.git
 * Intel Computer Vision SDK - https://software.intel.com/en-us/openvino-toolkit/choose-download 
 * Intel OpenCL driver needs to be installed correctly - 

## pre-requisites installation without Docker

 * Ubuntu 16.04.3
   Install OS and finish "sudo apt-get update; sudo apt-get upgrade; reboot"
   Copy "vait_prerequisites_install_ubuntu.py" and "silent.cfg" to other folder (ex: ~/tmp)

   - SKL/BXT
	   Script will create many folders and download packages in current folder, so you'd be better run this from other directory, not in the VAIT folder.

	   tmp$ sudo vait_prerequisites_install_ubuntu.py
	   tmp$ reboot
## installation with Docker

 * Ubuntu 16.04.3
   Install OS and finish "sudo apt-get update; sudo apt-get upgrade; reboot"

 * docker build -t <any name>:<any:version> .
 * docker run -ti --device /dev/dri:/dev/dri IMAGEID /bin/bash

## build

 * Ubuntu 16.04.3

   $ source env.sh
   $ mkdir build
   $ cd build
   $ cmake ..
   $ make

## execution

 * Ubuntu 16.04.3

   .\build\video_analytics_example\video_analytics_example -d GPU -c 1 -batch 6

#streams are hardcoded in the main.cpp, so no need to specify the source clip.

### Q&A
## XCB display issue
You may meet XCB display issue, please follow up below steps to check whether it an solve your issue.
https://github.com/teltek/Galicaster/issues/543

sudo apt remove  xserver-xorg-core-hwe-16.04 #Also remove all the dependencies
sudo apt install ubuntu-desktop xorg xserver-xorg-core #Also install the required dependencies
