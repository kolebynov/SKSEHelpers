#pragma once

#include "Observables/GenericSubscription.h"
#include "ResourceLock.h"

#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <queue>
#include <vector>

class TaskScheduler
{
public:
	using IntervalSubscription = Observables::GenericSubscription<TaskScheduler>;

	static TaskScheduler& Instance();

	void Tick();

	template <std::invocable TFunc>
	void Schedule(TFunc&& action, std::chrono::milliseconds delay)
	{
		_tasks.Acquire()->emplace(Task{
			.Action = std::forward<TFunc>(action),
			.InvokeTimestamp = std::chrono::steady_clock::now() + delay,
		});
	}

	template <std::invocable TFunc>
	IntervalSubscription ScheduleInterval(TFunc&& action, std::chrono::milliseconds interval)
	{
		const Observables::SubscriptionId id = _nextIntervalId++;
		_intervalTasks.Acquire()->emplace(IntervalTask{
			.Action = std::forward<TFunc>(action),
			.InvokeTimestamp = std::chrono::steady_clock::now() + interval,
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
		std::move_only_function<void ()> Action;
		std::chrono::steady_clock::time_point InvokeTimestamp;
	};

	struct IntervalTask
	{
		std::move_only_function<void ()> Action;
		std::chrono::steady_clock::time_point InvokeTimestamp;
		std::chrono::milliseconds Interval;
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

	bool ConsumeUnsubscribe(Observables::SubscriptionId id);

	ResourceLock<std::priority_queue<Task, std::vector<Task>, TaskComparer<Task>>> _tasks{};
	ResourceLock<std::priority_queue<IntervalTask, std::vector<IntervalTask>, TaskComparer<IntervalTask>>> _intervalTasks{};
	ResourceLock<std::vector<Observables::SubscriptionId>> _unsubscribedIntervalTasks{};
	std::atomic<Observables::SubscriptionId> _nextIntervalId = 1;
};
