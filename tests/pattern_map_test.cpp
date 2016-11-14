#include <gtest/gtest.h>
#include "../src/pattern_map.h"

using namespace servlet;

TEST(pattern_map_test, exact_not_exact)
{
    pattern_map<int> pm;
    pm.add("/", true, 1);
    pm.add("/", false, 2);
    ASSERT_EQ(*pm.get(std::string{"/"}), 1);
    ASSERT_EQ(*pm.get(std::string{"/a"}), 2);
}
