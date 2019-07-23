#ifndef __DSS_POC_DEFAULT_HPP__
#define __DSS_POC_DEFAULT_HPP__

#include <cstdint>
#define SYS_CONFIGFILE "example_config.json"
#define ICESERVERCONFIGFILE "/home/lxia1/Documents/myprojects/videoanalyticpoc/RPC/network/config.server"
#define ICECLIENTCONFIGFILE "/home/lxia1/Documents/myprojects/videoanalyticpoc/RPC/network/config.client"

#define UDP_PACKET_SIZE 61440
#define FEATURE_LENGTH  512

using Feature_Precision_t = uint8_t;

#endif