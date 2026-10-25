#include "GenericUtilities.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <ranges>
#include <vector>

TEST_CASE("NearlyEqualComparer compares with its epsilon", "[GenericUtilities]")
{
	constexpr NearlyEqualComparer<double> defaultEpsilon;
	CHECK(defaultEpsilon(1.0, 1.05));
	CHECK(defaultEpsilon(1.05, 1.0));
	CHECK_FALSE(defaultEpsilon(1.0, 1.2));
	CHECK_FALSE(defaultEpsilon(1.2, 1.0));

	constexpr NearlyEqualComparer<double, 0.001> narrow;
	CHECK(narrow(1.0, 1.0005));
	CHECK_FALSE(narrow(1.0, 1.05));
}

TEST_CASE("SafeRange iterates a container or nothing for null", "[GenericUtilities]")
{
	std::vector<int> values{ 1, 2, 3 };
	CHECK(std::ranges::equal(SafeRange(&values), values));

	std::vector<int>* missing = nullptr;
	CHECK(std::ranges::empty(SafeRange(missing)));
}
