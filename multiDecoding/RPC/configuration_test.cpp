#include <gtest/gtest.h>
#include "configuration.hpp"
#include <default.hpp>

TEST(read_configuration_from_json, missing_json) {
    Configuration conf;
    EXPECT_FALSE(conf.init_from_json("not_exist.json"));
}

TEST(read_configuration_from_json, read_and_parse_streams) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));
    ASSERT_EQ(conf.size_of_streams(), 4);
}

TEST(read_configuration_from_json, read_and_parse_nodes) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));
    ASSERT_EQ(conf.size_of_nodes(), 5);
}

TEST(read_configuration_from_json, get_streams) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));
    std::vector<StreamImage_ptr_t> test_streams;
    conf.get_streams(test_streams);
    ASSERT_EQ(test_streams.size(),4);
}

TEST(read_configuration_from_json, get_nodes) {
    Configuration conf;
    EXPECT_TRUE(conf.init_from_json(SYS_CONFIGFILE));
    std::vector<NodeWithState_ptr_t> test_nodes;
    conf.get_nodes(test_nodes);
    ASSERT_EQ(test_nodes.size(),5);
}
