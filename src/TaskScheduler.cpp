#include "TaskScheduler.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace std::chrono;

TaskScheduler& TaskScheduler::Instance()
{
	static TaskScheduler instance;
	return instance;
}

void TaskScheduler::Tick()
{
	const auto timestamp = steady_clock::now();

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

	const auto timestampPlusOneMs = timestamp + 1ms;
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

		task.InvokeTimestamp = std::max(task.InvokeTimestamp + task.Interval, timestampPlusOneMs);
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

bool TaskScheduler::ConsumeUnsubscribe(Observables::SubscriptionId id)
{
	auto unsubscribedIntervalTasks = _unsubscribedIntervalTasks.Acquire();
	if (auto it = std::ranges::find(*unsubscribedIntervalTasks, id); it != unsubscribedIntervalTasks->end()) {
		std::swap(*it, unsubscribedIntervalTasks->back());
		unsubscribedIntervalTasks->pop_back();
		return true;
	}

	return false;
}
