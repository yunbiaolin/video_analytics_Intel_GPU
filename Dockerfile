FROM ubuntu:16.04
MAINTAINER monique.jones@intel.com
LABEL Name=tutorials-base-test Version=0.1.0


RUN apt-get update \
    && apt install -y software-properties-common cpio \
    && rm -rf /var/lib/apt/lists/*

RUN add-apt-repository -y ppa:teejee2008/ppa

RUN apt-get update && apt-get install -y \
        git \
        libssl-dev \
        dh-autoreconf \
        cmake \
        libgl1-mesa-dev \
        libpciaccess-dev \
        build-essential \
        curl \
        kmod \
        imagemagick \
        gedit \
        mplayer \
        unzip \
        yasm \
        libjpeg-dev \
        linux-firmware \
        ukuu \
	openssl \
	libnuma1 \
	libpciaccess0 \
	xz-utils \
	bc \
	curl \
	libssl-dev\
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
        libopencv-dev \
        checkinstall \
        pkg-config \
        libgflags-dev \
        udev \
    && rm -rf /var/lib/apt/lists/*	


WORKDIR /vait
COPY . /vait	
COPY opt /opt

RUN cd /vait/
RUN /bin/bash -c "./install_OCL_driver.sh install"



RUN mkdir /tmp/vait && cp install.py /tmp/vait/ &&  cp silent.cfg /tmp/vait/
RUN cd /tmp/vait && python install.py 
RUN rm -r /tmp/vait


ENV MFX_HOME=/opt/intel/mediasdk
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu:/usr/lib
ENV LIBVA_DRIVERS_PATH=/usr/lib/x86_64-linux-gnu/dri
ENV LIBVA_DRIVER_NAME=iHD
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/intel/tbb/lib

RUN cd /vait/
RUN /bin/bash -c "source env.sh && mkdir build && cd build && cmake ..&& make"

WORKDIR /vait/build/video_analytics_example
CMD /bin/bash -c "source /vait/env.sh && ./video_analytics_example -i /vait/test_content/video/cars_1920x1080.h264  -m /vait/test_content/IR/SSD/SSD_GoogleNet_v2_fp16.xml -l /vait/test_content/IR/SSD/pascal_voc_classes.txt -batch 1 -fr 50 -d GPU"
