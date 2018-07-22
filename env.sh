# To build:

# To execute:

export DL_SDK_INCLUDE=/opt/intel/computer_vision_sdk_2018.2.319/deployment_tools/inference_engine/include
export DL_SDK_LIB=/opt/intel/computer_vision_sdk_2018.2.319/deployment_tools/inference_engine/lib/ubuntu_16.04/intel64

export PKG_CONFIG_PATH=/usr/lib64/pkgconfig

# for all tutorials and samples
source /opt/intel/computer_vision_sdk_2018.2.319/bin/setupvars.sh

# for samples
if [ -d "/opt/intel/tbb" ]; then
    source /opt/intel/tbb/bin/tbbvars.sh
fi

