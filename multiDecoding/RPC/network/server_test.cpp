#include <Ice/Ice.h>
#include <IceUtil/Config.h>
#include <fstream>
#include <remote_processor.h>
#include <remote_interface.hpp>
#include <utility.hpp>
#include <default.hpp>


using namespace std;
using namespace Ice;
using namespace remote;

#define VALIDATIONFILE "validation.jpg"

Ice::CommunicatorHolder ich(ICESERVERCONFIGFILE);

bool start_detect_server(const std::string& ip, uint32_t port){
    try {
        std::string endpoint_string = "udp -h "+ip+" -p "+std::to_string(port);
        auto adapter = ich->createObjectAdapterWithEndpoints("DetectorAdapter", endpoint_string);
        auto servant = make_shared<DetectorI>();
        adapter->add(servant,  Ice::stringToIdentity("Detector"));
        adapter->activate();
    }
    catch(const std::exception& e) {
        cerr << e.what() << endl;
        return false;
    }
    return true;
}

bool start_classifier_server(const std::string& ip, uint32_t port){
    try {
        std::string endpoint_string = "udp -h "+ip+" -p "+std::to_string(port);
        auto adapter = ich->createObjectAdapterWithEndpoints("ClassifierAdapter", endpoint_string);
        auto servant = make_shared<ClassifierI>();
        adapter->add(servant,  Ice::stringToIdentity("Classifier"));
        adapter->activate();
    }
    catch(const std::exception& e) {
        cerr << e.what() << endl;
        return false;
    }
    return true;
}


bool start_matching_server(const std::string& ip, uint32_t port){
    try {
        std::string endpoint_string = "udp -h "+ip+" -p "+std::to_string(port);
        auto adapter = ich->createObjectAdapterWithEndpoints("MatchingAdapter", endpoint_string);
        auto servant = make_shared<MatchingI>();
        adapter->add(servant,  Ice::stringToIdentity("Matching"));
        adapter->activate();
    }
    catch(const std::exception& e) {
        cerr << e.what() << endl;
        return false;
    }
    return true;
};


int main(int argc, char* argv[]){
    
        //get local ip addres
        LocalIpAddresses ip_addrs;

        //get node configuration
        Configuration conf;
        conf.init_from_json(SYS_CONFIGFILE);
        std::vector<NodeWithState_ptr_t> nodes;
        conf.get_nodes(nodes);

        for (auto& local_ip:ip_addrs.get_addrs()){
            for (auto& node:nodes) {
                if (node->type()=="Detector") // detector node
                {
                    if (*local_ip==node->ip()) {
                        if (start_detect_server(*local_ip, node->port())) 
                            std::cout<<"Detector is running at "<<*local_ip<<":"<<std::to_string(node->port())<<std::endl;
                            else
                            std::cout<<"Failed to run detector at "<<*local_ip<<":"<<std::to_string(node->port())<<std::endl;
                        }
                    continue;
                }

                if (node->type()=="Classifier") // classifier node
                {
                    if (*local_ip==node->ip()) {
                        if (start_classifier_server(*local_ip, node->port())) 
                            std::cout<<"classifier is running at "<<*local_ip<<":"<<std::to_string(node->port())<<std::endl;
                            else
                            std::cout<<"Failed to run classifier at "<<*local_ip<<":"<<std::to_string(node->port())<<std::endl;
                    }
                    continue;
                }

                if (node->type()=="Matching") // matching node
                {
                    if (*local_ip==node->ip()) {
                        if (start_matching_server(*local_ip, node->port())) 
                            std::cout<<"matching is running at "<<*local_ip<<":"<<std::to_string(node->port())<<std::endl;
                            else
                            std::cout<<"Failed to run classifier at "<<*local_ip<<":"<<std::to_string(node->port())<<std::endl;
                    }
                    continue;
                }
            }
        }

        ich->waitForShutdown();

    return 0;
}