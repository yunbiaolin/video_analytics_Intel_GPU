#include <utility.hpp>
#include <string>
#include <vector>

LocalIpAddresses::LocalIpAddresses() {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmp_ptr_addr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        std::string* ptr_str_address = new std::string;
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmp_ptr_addr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char address_buffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp_ptr_addr, address_buffer, INET_ADDRSTRLEN);
            *ptr_str_address = address_buffer;
            get_addrs().push_back(ptr_str_address);
//            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmp_ptr_addr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char address_buffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmp_ptr_addr, address_buffer, INET6_ADDRSTRLEN);
            *ptr_str_address = address_buffer;
            get_addrs().push_back(ptr_str_address);
//            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}

LocalIpAddresses::~LocalIpAddresses() {
    for (auto&ptr_str_address:get_addrs()){
        delete ptr_str_address;
    }
    get_addrs().clear();
}

IpAddresses& LocalIpAddresses::get_addrs() {
    return ip_addrs;
}
