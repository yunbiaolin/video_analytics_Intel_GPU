#include <gtest/gtest.h>

#include "configuration.hpp"
#include "scheduler.hpp"
#include <default.hpp>

#define EXPECTED_DETECTOR1 "127.0.0.1"
#define EXPECTED_CLASSIFIER1 "192.168.1.4"
#define EXPECTED_CLASSIFIER2 "192.168.1.3"
#define EXPECTED_NOT_AVAILABLE_CLASSIFIER "192.168.1.4"

TEST(roundrobin_scheduler, get_next_node) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));

    RoundRobinScheduler rs("Detector");
    std::vector<NodeWithState_ptr_t> nodes;
    conf.get_nodes(nodes);
    rs.attach(&nodes);
    NodeWithState_ptr_t ptr_node = rs.next_available();
    ASSERT_STREQ(EXPECTED_DETECTOR1, ptr_node->ip().c_str());
    ASSERT_STREQ("Detector", ptr_node->type().c_str());
    ASSERT_EQ(ptr_node->port(), 10000);
    ASSERT_TRUE(ptr_node->is_alive());
}

TEST(roundrobin_scheduler, node_aliveness) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));

    RoundRobinScheduler rs("Classifier");
    std::vector<NodeWithState_ptr_t> nodes;
    conf.get_nodes(nodes);
    rs.attach(&nodes);
    NodeWithState_ptr_t ptr_node = rs.next_available();
    
    // disable the 1st node
    ptr_node->disable();
    ASSERT_FALSE(ptr_node->is_alive());

    // note should not be in the available node list
    for (int i=0; i<nodes.size(); i++) {
        ptr_node = rs.next_available();
        ASSERT_STRNE(EXPECTED_NOT_AVAILABLE_CLASSIFIER, ptr_node->ip().c_str());
    }
}

TEST(roundrobin_scheduler, availiable_size) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));
    ASSERT_EQ(conf.size_of_nodes(), 5);

    RoundRobinScheduler rs("Detector");
    std::vector<NodeWithState_ptr_t> nodes;
    conf.get_nodes(nodes);
    rs.attach(&nodes);

    ASSERT_EQ(rs.available_size(),1);
    NodeWithState_ptr_t ptr_node = rs.next_available();
    
    // disable the 1st detector node
    ptr_node->disable();
    ASSERT_EQ(rs.available_size(),0);
}
