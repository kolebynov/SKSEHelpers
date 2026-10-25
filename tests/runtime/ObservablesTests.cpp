#include "GenericUtilities.h"
#include "Observables/CombineLatest.h"
#include "Observables/Observable.h"
#include "Observables/ReactiveObject.h"
#include "Observables/Subject.h"
#include "Observables/TrackedValue.h"
#include "Observables/Transform.h"
#include "Observables/Window.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace Observables;

static_assert(Observable<Subject<int>>);
static_assert(Observable<ReactiveObject<int>>);
static_assert(!Observable<int>);

TEST_CASE("Subject notifies every subscriber until its subscription is destroyed", "[Observables][Subject]")
{
	Subject<int> subject;
	std::vector<int> first;
	std::vector<int> second;
	auto firstSubscription = subject.Subscribe([&](int value) { first.push_back(value); });
	std::optional<Subject<int>::Subscription> secondSubscription = subject.Subscribe([&](int value) { second.push_back(value); });

	subject.Notify(1);
	secondSubscription.reset();
	subject.Notify(2);

	CHECK(first == std::vector{ 1, 2 });
	CHECK(second == std::vector{ 1 });
}

TEST_CASE("Subject keeps a detached observer subscribed", "[Observables][Subject]")
{
	Subject<int> subject;
	int calls = 0;
	subject.Subscribe([&](int) { ++calls; }).Detach();

	subject.Notify(1);
	subject.Notify(2);
	CHECK(calls == 2);
}

TEST_CASE("Assigning a subscription unsubscribes the previous one", "[Observables][Subject]")
{
	Subject<int> subject;
	int oldCalls = 0;
	int newCalls = 0;
	auto subscription = subject.Subscribe([&](int) { ++oldCalls; });
	subscription = subject.Subscribe([&](int) { ++newCalls; });

	subject.Notify(1);
	CHECK(oldCalls == 0);
	CHECK(newCalls == 1);
}

TEST_CASE("SubscribeAndNotify calls the new observer immediately", "[Observables][Subject]")
{
	Subject<int> subject;
	std::vector<int> received;
	auto subscription = subject.SubscribeAndNotify([&](int value) { received.push_back(value); }, 7);
	CHECK(received == std::vector{ 7 });

	subject.Notify(8);
	CHECK(received == std::vector{ 7, 8 });
}

TEST_CASE("An observer can unsubscribe itself during a notification", "[Observables][Subject]")
{
	Subject<int> subject;
	int selfCalls = 0;
	int otherCalls = 0;
	std::optional<Subject<int>::Subscription> self;
	self = subject.Subscribe([&](int) {
		++selfCalls;
		self.reset();
	});
	auto other = subject.Subscribe([&](int) { ++otherCalls; });

	subject.Notify(1);
	subject.Notify(2);
	CHECK(selfCalls == 1);
	CHECK(otherCalls == 2);
}

TEST_CASE("An observer can unsubscribe another observer during a notification", "[Observables][Subject]")
{
	Subject<int> subject;
	int victimCalls = 0;
	std::optional<Subject<int>::Subscription> victim = subject.Subscribe([&](int) { ++victimCalls; });
	auto killer = subject.Subscribe([&](int) { victim.reset(); });

	// Map iteration order decides whether the victim still sees the first value; it must not see later ones.
	subject.Notify(1);
	const int callsAfterFirst = victimCalls;
	subject.Notify(2);
	CHECK(callsAfterFirst <= 1);
	CHECK(victimCalls == callsAfterFirst);
}

TEST_CASE("Observers subscribed during a notification start with the next one", "[Observables][Subject]")
{
	Subject<int> subject;
	std::vector<Subject<int>::Subscription> added;
	int addedCalls = 0;
	// Enough subscriptions to force the observer map to rehash while it is being iterated.
	auto spawner = subject.Subscribe([&](int value) {
		if (value != 1) {
			return;
		}

		for (int i = 0; i < 64; ++i) {
			added.push_back(subject.Subscribe([&](int) { ++addedCalls; }));
		}
	});

	subject.Notify(1);
	CHECK(addedCalls == 0);

	subject.Notify(2);
	CHECK(addedCalls == 64);
}

TEST_CASE("An observer subscribed and unsubscribed in one notification is never called", "[Observables][Subject]")
{
	Subject<int> subject;
	int transientCalls = 0;
	auto spawner = subject.Subscribe([&](int value) {
		if (value == 1) {
			std::optional<Subject<int>::Subscription> transient = subject.Subscribe([&](int) { ++transientCalls; });
			transient.reset();
		}
	});

	subject.Notify(1);
	subject.Notify(2);
	CHECK(transientCalls == 0);
}

TEST_CASE("SubscribeAndNotify during a notification", "[Observables][Subject]")
{
	Subject<int> subject;
	std::vector<int> received;
	std::optional<Subject<int>::Subscription> added;
	auto spawner = subject.Subscribe([&](int value) {
		if (value == 1) {
			added = subject.SubscribeAndNotify([&](int addedValue) { received.push_back(addedValue); }, 10);
		}
	});

	subject.Notify(1);
	CHECK(received == std::vector{ 10 });

	subject.Notify(2);
	CHECK(received == std::vector{ 10, 2 });
}

TEST_CASE("Unsubscribing during a nested notification", "[Observables][Subject]")
{
	Subject<int> subject;
	std::vector<int> selfReceived;
	std::optional<Subject<int>::Subscription> self;
	std::optional<Subject<int>::Subscription> reentrant;
	auto subscribeSelf = [&] {
		self = subject.Subscribe([&](int value) {
			selfReceived.push_back(value);
			if (value == 1) {
				self.reset();
			}
		});
	};
	auto subscribeReentrant = [&] {
		reentrant = subject.Subscribe([&](int value) {
			if (value == 1) {
				subject.Notify(2);
			}
		});
	};

	// Both orders, since map iteration order decides which observer the outer notification visits first.
	SECTION("reentrant observer subscribed first")
	{
		subscribeReentrant();
		subscribeSelf();
	}

	SECTION("self-unsubscribing observer subscribed first")
	{
		subscribeSelf();
		subscribeReentrant();
	}

	subject.Notify(1);
	CHECK(std::ranges::count(selfReceived, 1) == 1);
	CHECK(selfReceived.back() == 1);

	const auto callsBefore = selfReceived.size();
	subject.Notify(3);
	CHECK(selfReceived.size() == callsBefore);
}

TEST_CASE("TrackedValue reports changes according to its comparer", "[Observables][TrackedValue]")
{
	SECTION("default comparer")
	{
		TrackedValue<int> value(1);
		CHECK_FALSE(value.IsChanged());
		CHECK_FALSE(value.Set(1));
		CHECK(value.Set(2));
		CHECK(value.Get() == 2);
		CHECK_FALSE(value.Set(2));
	}

	SECTION("custom comparer")
	{
		TrackedValue<float, NearlyEqualComparer<float>> value(1.0f);
		CHECK_FALSE(value.Set(1.05f));
		CHECK(value.Get() == 1.05f);
		CHECK(value.Set(2.0f));
	}
}

TEST_CASE("ReactiveObject notifies only when the value changes", "[Observables][ReactiveObject]")
{
	ReactiveObject<std::string> object("a");
	std::vector<std::string> received;
	auto subscription = object.Subscribe([&](const std::string& value) { received.push_back(value); });

	object.Set("a");
	object.Set("b");
	object.Set(std::string("b"));
	object.Set(std::string("c"));

	CHECK(received == std::vector<std::string>{ "b", "c" });
	CHECK(object.Value() == "c");
}

TEST_CASE("ReactiveObject can send the current value on subscribe", "[Observables][ReactiveObject]")
{
	ReactiveObject<int> object(5);
	std::vector<int> direct;
	std::vector<int> adapted;
	auto directSubscription = object.Subscribe([&](const int& value) { direct.push_back(value); }, true);
	auto adaptedSubscription = object.ToNotifyOnSubscribe().Subscribe([&](const int& value) { adapted.push_back(value); });

	object.Set(6);
	CHECK(direct == std::vector{ 5, 6 });
	CHECK(adapted == std::vector{ 5, 6 });
}

TEST_CASE("Transform maps every value of the source", "[Observables][Transform]")
{
	Subject<int> subject;
	std::vector<std::string> received;
	auto subscription = (subject | Transform([](int value) {
		return std::to_string(value * 2);
	})).Subscribe([&](std::string value) { received.push_back(std::move(value)); });

	subject.Notify(1);
	subject.Notify(5);
	CHECK(received == std::vector<std::string>{ "2", "10" });
}

TEST_CASE("Transform subscriptions unsubscribe from the source", "[Observables][Transform]")
{
	Subject<int> subject;
	int calls = 0;
	{
		auto subscription = (subject | Transform([](int value) { return value; })).Subscribe([&](int) { ++calls; });
		subject.Notify(1);
	}

	subject.Notify(2);
	CHECK(calls == 1);
}

TEST_CASE("Window emits a sliding window once it is filled", "[Observables][Window]")
{
	Subject<int> subject;
	std::vector<std::array<int, 3>> received;
	auto subscription = (subject | Window<3>()).Subscribe([&](const std::array<int, 3>& window) { received.push_back(window); });

	subject.Notify(1);
	subject.Notify(2);
	CHECK(received.empty());

	subject.Notify(3);
	subject.Notify(4);
	subject.Notify(5);
	using Values = std::array<int, 3>;
	CHECK(received == std::vector{ Values{ { 1, 2, 3 } }, Values{ { 2, 3, 4 } }, Values{ { 3, 4, 5 } } });
}

TEST_CASE("Window state is per subscription", "[Observables][Window]")
{
	Subject<int> subject;
	auto windowed = subject | Window<2>();
	std::vector<std::array<int, 2>> early;
	std::vector<std::array<int, 2>> late;

	auto earlySubscription = windowed.Subscribe([&](const std::array<int, 2>& window) { early.push_back(window); });
	subject.Notify(1);
	auto lateSubscription = windowed.Subscribe([&](const std::array<int, 2>& window) { late.push_back(window); });
	subject.Notify(2);
	subject.Notify(3);

	using Values = std::array<int, 2>;
	CHECK(early == std::vector{ Values{ { 1, 2 } }, Values{ { 2, 3 } } });
	CHECK(late == std::vector{ Values{ { 2, 3 } } });
}

TEST_CASE("CombineLatest emits latest values after every source has emitted", "[Observables][CombineLatest]")
{
	Subject<int> numbers;
	Subject<std::string> names;
	using Values = std::tuple<int, std::string>;
	std::vector<Values> received;
	auto subscription = CombineLatest(numbers, names).Subscribe([&](const Values& values) { received.push_back(values); });

	numbers.Notify(1);
	numbers.Notify(2);
	CHECK(received.empty());

	names.Notify("a");
	numbers.Notify(3);
	names.Notify("b");
	CHECK(received == std::vector<Values>{ { 2, "a" }, { 3, "a" }, { 3, "b" } });
}

TEST_CASE("CombineLatest subscription unsubscribes from every source", "[Observables][CombineLatest]")
{
	Subject<int> first;
	Subject<int> second;
	int calls = 0;
	{
		auto subscription = CombineLatest(first, second).Subscribe([&](const std::tuple<int, int>&) { ++calls; });
		first.Notify(1);
		second.Notify(2);
	}

	first.Notify(3);
	second.Notify(4);
	CHECK(calls == 1);
}

TEST_CASE("CombineLatest keeps working after Detach", "[Observables][CombineLatest]")
{
	Subject<int> first;
	Subject<int> second;
	int calls = 0;
	CombineLatest(first, second).Subscribe([&](const std::tuple<int, int>&) { ++calls; }).Detach();

	first.Notify(1);
	second.Notify(2);
	first.Notify(3);
	CHECK(calls == 2);
}
