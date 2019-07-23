#include <gtest/gtest.h>
#include "object.hpp"

TEST(object, basic_test) {
    BBox bbox;
    bbox.top=0;
    bbox.left=5;
    bbox.bottom=10;
    bbox.right=20;
    StreamImage image(1, "", 1L, 10, 20, 100);

    Object o1(bbox, image);
    Object o2 = o1;
    ASSERT_FALSE(o1.get_bbox()==o2.get_bbox());
    ASSERT_FALSE(o1.get_image()==o2.get_image());
    ASSERT_EQ(o1.get_bbox()->left, o2.get_bbox()->left);
    ASSERT_EQ(o1.get_bbox()->right, o2.get_bbox()->right);
    ASSERT_EQ(o1.get_bbox()->top, o2.get_bbox()->top);
    ASSERT_EQ(o1.get_bbox()->bottom, o2.get_bbox()->bottom);
}

