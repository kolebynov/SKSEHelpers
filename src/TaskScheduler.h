#pragma once

#include "Observables/GenericSubscription.h"
#include "ResourceLock.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

// Timestamp usable by TaskScheduler: ordered, trivially copyable (stored atomically), and `timestamp + (timestamp - timestamp)`
// yields a timestamp again (e.g. std::chrono::time_point, float game seconds, uint64_t frame counters).
template <typename T>
concept SchedulerTimestamp = std::totally_ordered<T> && std::is_trivially_copyable_v<T> && requires(const T& timestamp) {
	{ timestamp + (timestamp - timestamp) } -> std::convertible_to<T>;
};

// The scheduler never reads a clock: the consumer supplies the current timestamp through the constructor and Tick().
// Schedule() and ScheduleInterval() compute invoke timestamps from the timestamp of the last Tick(), so everything scheduled
// between two ticks with the same delay becomes due on the same tick.
// Non-movable: interval subscriptions keep a pointer to the scheduler.
template <SchedulerTimestamp TTimestamp = std::chrono::steady_clock::time_point>
class TaskScheduler
{
public:
	using Timestamp = TTimestamp;
	using Duration = decltype(std::declval<const TTimestamp&>() - std::declval<const TTimestamp&>());
	using IntervalSubscription = Observables::GenericSubscription<TaskScheduler>;

	explicit TaskScheduler(TTimestamp initialTimestamp) noexcept : _timestamp(initialTimestamp) {}

	TaskScheduler(const TaskScheduler&) = delete;
	TaskScheduler(TaskScheduler&&) = delete;
	TaskScheduler& operator=(const TaskScheduler&) = delete;
	TaskScheduler& operator=(TaskScheduler&&) = delete;
	~TaskScheduler() = default;

	void Tick(TTimestamp timestamp)
	{
		_timestamp.store(timestamp, std::memory_order_relaxed);

		// Callbacks routinely schedule more work, so they must never run while a queue lock is held:
		// std::mutex is not recursive and re-entering Schedule()/ScheduleInterval() would self-deadlock.
		std::vector<Task> readyTasks;
		{
			auto tasks = _tasks.Acquire();
			while (!tasks->empty() && tasks->top().InvokeTimestamp <= timestamp) {
				readyTasks.emplace_back(std::move(const_cast<Task&>(tasks->top())));
				tasks->pop();
			}
		}

		for (auto& task : readyTasks) {
			task.Action();
		}

		std::vector<IntervalTask> readyIntervalTasks;
		{
			auto intervalTasks = _intervalTasks.Acquire();
			while (!intervalTasks->empty() && intervalTasks->top().InvokeTimestamp <= timestamp) {
				readyIntervalTasks.emplace_back(std::move(const_cast<IntervalTask&>(intervalTasks->top())));
				intervalTasks->pop();
			}
		}

		std::vector<IntervalTask> repeatingTasks;
		repeatingTasks.reserve(readyIntervalTasks.size());
		for (auto& task : readyIntervalTasks) {
			if (ConsumeUnsubscribe(task.TaskId)) {
				continue;
			}

			task.Action();

			if (ConsumeUnsubscribe(task.TaskId)) {
				continue;
			}

			// Keep the cadence; if ticks fell behind, skip missed periods instead of firing a burst on following ticks.
			const TTimestamp next = task.InvokeTimestamp + task.Interval;
			task.InvokeTimestamp = next > timestamp ? next : static_cast<TTimestamp>(timestamp + task.Interval);
			repeatingTasks.emplace_back(std::move(task));
		}

		if (repeatingTasks.empty()) {
			return;
		}

		auto intervalTasks = _intervalTasks.Acquire();
		for (auto& task : repeatingTasks) {
			intervalTasks->emplace(std::move(task));
		}
	}

	template <std::invocable TFunc>
	void Schedule(TFunc&& action, Duration delay)
	{
		_tasks.Acquire()->emplace(Task{
			.Action = std::forward<TFunc>(action),
			.InvokeTimestamp = static_cast<TTimestamp>(_timestamp.load(std::memory_order_relaxed) + delay),
		});
	}

	template <std::invocable TFunc>
	IntervalSubscription ScheduleInterval(TFunc&& action, Duration interval)
	{
		const Observables::SubscriptionId id = _nextIntervalId++;
		_intervalTasks.Acquire()->emplace(IntervalTask{
			.Action = std::forward<TFunc>(action),
			.InvokeTimestamp = static_cast<TTimestamp>(_timestamp.load(std::memory_order_relaxed) + interval),
			.Interval = interval,
			.TaskId = id,
		});

		return { this, id };
	}

	void Unsubscribe(Observables::SubscriptionId subscriptionId)
	{
		_unsubscribedIntervalTasks.Acquire()->emplace_back(subscriptionId);
	}

private:
	struct Task
	{
		std::move_only_function<void()> Action;
		TTimestamp InvokeTimestamp;
	};

	struct IntervalTask
	{
		std::move_only_function<void()> Action;
		TTimestamp InvokeTimestamp;
		Duration Interval;
		Observables::SubscriptionId TaskId;
	};

	template <typename TTask>
	struct TaskComparer
	{
		constexpr bool operator()(const TTask& left, const TTask& right) const noexcept
		{
			return left.InvokeTimestamp > right.InvokeTimestamp;
		}
	};

	bool ConsumeUnsubscribe(Observables::SubscriptionId id)
	{
		auto unsubscribedIntervalTasks = _unsubscribedIntervalTasks.Acquire();
		if (auto it = std::ranges::find(*unsubscribedIntervalTasks, id); it != unsubscribedIntervalTasks->end()) {
			std::swap(*it, unsubscribedIntervalTasks->back());
			unsubscribedIntervalTasks->pop_back();
			return true;
		}

		return false;
	}

	ResourceLock<std::priority_queue<Task, std::vector<Task>, TaskComparer<Task>>> _tasks{};
	ResourceLock<std::priority_queue<IntervalTask, std::vector<IntervalTask>, TaskComparer<IntervalTask>>> _intervalTasks{};
	ResourceLock<std::vector<Observables::SubscriptionId>> _unsubscribedIntervalTasks{};
	std::atomic<TTimestamp> _timestamp;
	std::atomic<Observables::SubscriptionId> _nextIntervalId = 1;
};
