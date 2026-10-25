#include "ResourceLock.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_same_v<decltype(*std::declval<ResourceLock<int>&>().Acquire()), int&>);
static_assert(std::is_same_v<decltype(*std::declval<const ResourceLock<int>&>().Acquire()), const int&>);

TEST_CASE("ResourceLock constructs the value in place", "[ResourceLock]")
{
	ResourceLock<std::string> lock(std::in_place, 3u, 'x');
	CHECK(*lock.Acquire() == "xxx");
	CHECK(lock.Acquire()->size() == 3);
}

TEST_CASE("ResourceLock serializes concurrent access", "[ResourceLock]")
{
	constexpr int threadCount = 8;
	constexpr int incrementsPerThread = 20'000;

	ResourceLock<int> counter(std::in_place, 0);
	{
		std::vector<std::jthread> threads;
		for (int i = 0; i < threadCount; ++i) {
			threads.emplace_back([&counter] {
				for (int j = 0; j < incrementsPerThread; ++j) {
					auto value = counter.Acquire();
					// Separate read and write widen the race window if the lock did not exclude other threads.
					const int current = *value;
					*value = current + 1;
				}
			});
		}
	}

	CHECK(*std::as_const(counter).Acquire() == threadCount * incrementsPerThread);
}
