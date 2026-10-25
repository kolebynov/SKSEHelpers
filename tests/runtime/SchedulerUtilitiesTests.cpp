#include "SchedulerUtilities.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

using Scheduler = TaskScheduler<std::uint64_t>;

TEST_CASE("Debounce runs only the last call within the delay", "[SchedulerUtilities]")
{
	Scheduler scheduler(0);
	std::vector<int> calls;
	auto debounced = Debounce(scheduler, [&](int value) { calls.push_back(value); }, 10);

	debounced(1);
	scheduler.Tick(5);
	debounced(2);

	// The first call is due here but was superseded.
	scheduler.Tick(10);
	CHECK(calls.empty());

	scheduler.Tick(15);
	CHECK(calls == std::vector{ 2 });

	debounced(3);
	scheduler.Tick(25);
	CHECK(calls == std::vector{ 2, 3 });
}

TEST_CASE("Debounce passes a snapshot of the call arguments", "[SchedulerUtilities]")
{
	Scheduler scheduler(0);
	std::string received;
	auto debounced = Debounce(scheduler, [&](const std::string& value) { received = value; }, 10);

	std::string argument = "first";
	debounced(argument);
	argument = "changed";

	scheduler.Tick(10);
	CHECK(received == "first");
}

TEST_CASE("WaitFor runs the continuation once after the predicate becomes true", "[SchedulerUtilities]")
{
	Scheduler scheduler(0);
	bool ready = false;
	int predicateChecks = 0;
	int continuations = 0;
	WaitFor(
		scheduler,
		[&] {
			++predicateChecks;
			return ready;
		},
		[&] { ++continuations; }, 10);

	scheduler.Tick(10);
	CHECK(predicateChecks == 1);
	CHECK(continuations == 0);

	ready = true;
	scheduler.Tick(20);
	CHECK(continuations == 1);

	// The interval task cancelled itself: neither the predicate nor the continuation run again.
	scheduler.Tick(30);
	scheduler.Tick(40);
	CHECK(predicateChecks == 2);
	CHECK(continuations == 1);
}
