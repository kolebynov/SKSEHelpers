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

// Duration usable by TaskScheduler: ordered, trivially copyable (stored atomically), and `duration + duration` yields a
// duration again (e.g. std::chrono::duration, float game seconds, uint64_t frame counts).
template <typename T>
concept SchedulerDuration = std::totally_ordered<T> && std::is_trivially_copyable_v<T> && requires(const T& duration) {
	{ duration + duration } -> std::convertible_to<T>;
};

// One type for both delays and points in time: the current time is the duration elapsed since an origin chosen by the
// consumer (e.g. steady_clock::now().time_since_epoch(), game time, frame number).
// The scheduler never reads a clock: the consumer supplies the current time through the constructor and Tick().
// Schedule() and ScheduleInterval() compute invoke times from the time of the last Tick(), so everything scheduled
// between two ticks with the same delay becomes due on the same tick.
// Non-movable: interval subscriptions keep a pointer to the scheduler.
template <SchedulerDuration TDuration = std::chrono::steady_clock::duration>
class TaskScheduler
{
public:
	using Duration = TDuration;
	using IntervalSubscription = Observables::GenericSubscription<TaskScheduler>;

	explicit TaskScheduler(TDuration initialTime) noexcept : _time(initialTime) {}

	TaskScheduler(const TaskScheduler&) = delete;
	TaskScheduler(TaskScheduler&&) = delete;
	TaskScheduler& operator=(const TaskScheduler&) = delete;
	TaskScheduler& operator=(TaskScheduler&&) = delete;
	~TaskScheduler() = default;

	void Tick(TDuration time)
	{
		_time.store(time, std::memory_order_relaxed);

		// Callbacks routinely schedule more work, so they must never run while a queue lock is held:
		// std::mutex is not recursive and re-entering Schedule()/ScheduleInterval() would self-deadlock.
		std::vector<Task> readyTasks;
		{
			auto tasks = _tasks.Acquire();
			while (!tasks->empty() && tasks->top().InvokeTime <= time) {
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
			while (!intervalTasks->empty() && intervalTasks->top().InvokeTime <= time) {
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
			const TDuration next = task.InvokeTime + task.Interval;
			task.InvokeTime = next > time ? next : static_cast<TDuration>(time + task.Interval);
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
			.InvokeTime = static_cast<TDuration>(_time.load(std::memory_order_relaxed) + delay),
		});
	}

	template <std::invocable TFunc>
	IntervalSubscription ScheduleInterval(TFunc&& action, Duration interval)
	{
		const Observables::SubscriptionId id = _nextIntervalId++;
		_intervalTasks.Acquire()->emplace(IntervalTask{
			.Action = std::forward<TFunc>(action),
			.InvokeTime = static_cast<TDuration>(_time.load(std::memory_order_relaxed) + interval),
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
		TDuration InvokeTime;
	};

	struct IntervalTask
	{
		std::move_only_function<void()> Action;
		TDuration InvokeTime;
		TDuration Interval;
		Observables::SubscriptionId TaskId;
	};

	template <typename TTask>
	struct TaskComparer
	{
		constexpr bool operator()(const TTask& left, const TTask& right) const noexcept
		{
			return left.InvokeTime > right.InvokeTime;
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

	// Declared first so it is destroyed last: destroying queued tasks may destroy IntervalSubscriptions they own (e.g.
	// WaitFor), which call Unsubscribe() on this scheduler.
	ResourceLock<std::vector<Observables::SubscriptionId>> _unsubscribedIntervalTasks{};
	ResourceLock<std::priority_queue<Task, std::vector<Task>, TaskComparer<Task>>> _tasks{};
	ResourceLock<std::priority_queue<IntervalTask, std::vector<IntervalTask>, TaskComparer<IntervalTask>>> _intervalTasks{};
	std::atomic<TDuration> _time;
	std::atomic<Observables::SubscriptionId> _nextIntervalId = 1;
};
