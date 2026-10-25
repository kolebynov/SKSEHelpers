#pragma once

#include "TaskScheduler.h"

#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

template <SchedulerDuration TDuration, class TFunc>
auto Debounce(TaskScheduler<TDuration>& scheduler, TFunc&& func, std::type_identity_t<TDuration> delay)
{
	auto counter = std::make_shared<std::atomic_uint32_t>(0);
	auto funcShared = std::make_shared<std::decay_t<TFunc>>(std::forward<TFunc>(func));

	return [&scheduler, func = std::move(funcShared), counter = std::move(counter), delay]<class... TCallArgs>(
			   TCallArgs&&... args) mutable {
		auto tag = counter->fetch_add(1, std::memory_order_relaxed) + 1;
		// Debounced work runs later, so capture a snapshot of the call arguments now.
		auto scheduledArgs = std::make_unique<std::tuple<std::decay_t<TCallArgs>...>>(std::forward<TCallArgs>(args)...);

		scheduler.Schedule(
			[func, counter, tag, scheduledArgs = std::move(scheduledArgs)]() mutable {
				if (counter->load(std::memory_order_relaxed) != tag) {
					return;
				}

				std::apply(
					[&func]<typename... TArgs>(TArgs&&... storedArgs) { std::invoke(*func, std::forward<TArgs>(storedArgs)...); },
					*scheduledArgs);
			},
			delay);
	};
}

template <SchedulerDuration TDuration, std::invocable TPredicate, std::invocable TContinuation>
	requires std::same_as<bool, std::invoke_result_t<TPredicate>>
void WaitFor(TaskScheduler<TDuration>& scheduler, TPredicate&& predicate, TContinuation&& continuation,
	std::type_identity_t<TDuration> checkInterval)
{
	auto subPtr = std::make_shared<typename TaskScheduler<TDuration>::IntervalSubscription>();
	*subPtr = scheduler.ScheduleInterval(
		[predicate = std::forward<TPredicate>(predicate), continuation = std::forward<TContinuation>(continuation),
			subPtr] mutable {
			if (std::invoke(predicate)) {
				std::invoke(continuation);
				subPtr.reset();
			}
		},
		checkInterval);
}
