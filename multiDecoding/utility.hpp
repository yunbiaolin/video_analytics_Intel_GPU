#ifndef __DSS_POC_UTILITY_HPP__
#define __DSS_POC_UTILITY_HPP__

#include <vector>
#include <string>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <configuration.hpp>
/** 
  * @brief enum of queue roles 
  * CLIENT trigers RPC call. SERVER trigers local queue data processing.
  * are supported.
  */
enum QueueRole {
    CLIENT = 0,
    SERVER 
};

/** 
  * @brief Scheduler function callback type
  */
typedef NodeWithState_ptr_t (*SchedulerFun_t)();

/** 
  * @brief IP addresses type
  */
typedef std::vector<std::string*> IpAddresses;

/** 
  * @brief LocalIpAddresses class
  */
class LocalIpAddresses {
    private:
        IpAddresses ip_addrs;
    public:
        LocalIpAddresses();
        LocalIpAddresses& operator=(const LocalIpAddresses&) = delete;
        ~LocalIpAddresses();
        IpAddresses& get_addrs();
};

#endif