#include <gtest/gtest.h>

#include "version.hpp"

TEST(Smoke, Works) { EXPECT_EQ(1 + 1, 2); }

TEST(Smoke, ModuleName) { EXPECT_EQ(qi::geometry::moduleName(), "geometry"); }
