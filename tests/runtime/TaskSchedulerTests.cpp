#include "TaskScheduler.h"

#include <catch2/catch_test_macros.hpp>

#include <crtdbg.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// A frame counter keeps the tests deterministic: the scheduler only sees timestamps passed to Tick().
using Scheduler = TaskScheduler<std::uint64_t>;

TEST_CASE("Schedule runs a task once when its delay has elapsed", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int calls = 0;
	scheduler.Schedule([&] { ++calls; }, 10);

	scheduler.Tick(9);
	CHECK(calls == 0);

	scheduler.Tick(10);
	CHECK(calls == 1);

	scheduler.Tick(20);
	CHECK(calls == 1);
}

TEST_CASE("Schedule delays are measured from the last ticked timestamp", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	scheduler.Tick(100);

	int calls = 0;
	scheduler.Schedule([&] { ++calls; }, 5);

	scheduler.Tick(104);
	CHECK(calls == 0);

	scheduler.Tick(105);
	CHECK(calls == 1);
}

TEST_CASE("Tasks due on the same tick run in invoke timestamp order", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	std::vector<int> order;
	scheduler.Schedule([&] { order.push_back(3); }, 7);
	scheduler.Schedule([&] { order.push_back(1); }, 2);
	scheduler.Schedule([&] { order.push_back(2); }, 5);

	scheduler.Tick(10);
	CHECK(order == std::vector{ 1, 2, 3 });
}

TEST_CASE("A task scheduled from a running task runs on a later tick", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int innerCalls = 0;
	scheduler.Schedule([&] { scheduler.Schedule([&] { ++innerCalls; }, 0); }, 10);

	scheduler.Tick(10);
	CHECK(innerCalls == 0);

	scheduler.Tick(10);
	CHECK(innerCalls == 1);
}

TEST_CASE("ScheduleInterval repeats until the subscription is destroyed", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int calls = 0;
	std::optional<Scheduler::IntervalSubscription> subscription = scheduler.ScheduleInterval([&] { ++calls; }, 10);

	scheduler.Tick(9);
	CHECK(calls == 0);

	scheduler.Tick(10);
	scheduler.Tick(20);
	scheduler.Tick(30);
	CHECK(calls == 3);

	subscription.reset();
	scheduler.Tick(40);
	CHECK(calls == 3);
}

TEST_CASE("ScheduleInterval keeps running after Detach", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int calls = 0;
	scheduler.ScheduleInterval([&] { ++calls; }, 10).Detach();

	scheduler.Tick(10);
	scheduler.Tick(20);
	CHECK(calls == 2);
}

TEST_CASE("ScheduleInterval skips missed periods instead of firing a burst", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int calls = 0;
	auto subscription = scheduler.ScheduleInterval([&] { ++calls; }, 10);

	scheduler.Tick(55);
	CHECK(calls == 1);

	// Due at 20..50 were missed; the next run is one interval after the late tick.
	scheduler.Tick(64);
	CHECK(calls == 1);

	scheduler.Tick(65);
	CHECK(calls == 2);
}

TEST_CASE("ScheduleInterval keeps its cadence when ticks are on time", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int calls = 0;
	auto subscription = scheduler.ScheduleInterval([&] { ++calls; }, 10);

	scheduler.Tick(13);
	CHECK(calls == 1);

	// Next run stays at 20, not 13 + 10.
	scheduler.Tick(20);
	CHECK(calls == 2);
}

TEST_CASE("An interval task can cancel itself from its own callback", "[TaskScheduler]")
{
	Scheduler scheduler(0);
	int calls = 0;
	std::optional<Scheduler::IntervalSubscription> subscription;
	subscription = scheduler.ScheduleInterval(
		[&] {
			++calls;
			subscription.reset();
		},
		10);

	scheduler.Tick(10);
	scheduler.Tick(20);
	CHECK(calls == 1);
}

TEST_CASE("A scheduler can be destroyed while it owns interval subscriptions", "[TaskScheduler]")
{
	// The pending interval task owns its own subscription (as WaitFor does), so destroying the scheduler's queues
	// unsubscribes from the scheduler while it is being destroyed. The unsubscribe list must still be alive then;
	// otherwise the id is pushed into an already destroyed vector, which on MSVC leaks a fresh allocation.
#ifdef _DEBUG
	_CrtMemState before;
	_CrtMemCheckpoint(&before);
#endif

	int calls = 0;
	{
		Scheduler scheduler(0);
		auto subscription = std::make_shared<Scheduler::IntervalSubscription>();
		*subscription = scheduler.ScheduleInterval([&calls, subscription] { ++calls; }, 10);
		scheduler.Tick(10);
	}

#ifdef _DEBUG
	_CrtMemState after;
	_CrtMemCheckpoint(&after);
	_CrtMemState difference;
	CHECK_FALSE(_CrtMemDifference(&difference, &before, &after));
#endif
	CHECK(calls == 1);
}
