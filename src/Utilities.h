#pragma once

#include "TaskScheduler.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <concepts>
#include <functional>
#include <memory>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

template <std::floating_point T, T Epsilon = static_cast<T>(0.1)>
struct NearlyEqualComparer
{
	bool operator()(const T& a, const T& b) const { return std::abs(a - b) < Epsilon; }
};

template <class TFunc>
auto Debounce(TFunc&& func, std::chrono::milliseconds delay)
{
	auto counter = std::make_shared<std::atomic_uint32_t>(0);
	auto funcShared = std::make_shared<std::decay_t<TFunc>>(std::forward<TFunc>(func));

	return [func = std::move(funcShared), counter = std::move(counter), delay]<class... TCallArgs>(TCallArgs&&... args) mutable {
		auto tag = counter->fetch_add(1, std::memory_order_relaxed) + 1;
		// Debounced work runs later, so capture a snapshot of the call arguments now.
		auto scheduledArgs = std::make_unique<std::tuple<std::decay_t<TCallArgs>...>>(std::forward<TCallArgs>(args)...);

		TaskScheduler::Instance().Schedule(
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

template <std::invocable TPredicate, std::invocable TContinuation>
	requires std::same_as<bool, std::invoke_result_t<TPredicate>>
void WaitFor(TPredicate&& predicate, TContinuation&& continuation, std::chrono::milliseconds checkInterval)
{
	auto subPtr = std::make_shared<TaskScheduler::IntervalSubscription>();
	*subPtr = TaskScheduler::Instance().ScheduleInterval(
		[predicate = std::forward<TPredicate>(predicate), continuation = std::forward<TContinuation>(continuation),
			subPtr] mutable {
			if (std::invoke(predicate)) {
				std::invoke(continuation);
				subPtr.reset();
			}
		},
		checkInterval);
}

template <std::ranges::range T>
auto SafeRange(T* ptr)
{
	using ContainerType = std::decay_t<T>;

	[[likely]]
	if (ptr) {
		return std::ranges::subrange(ptr->begin(), ptr->end());
	}

	return std::ranges::subrange(typename ContainerType::iterator{}, typename ContainerType::iterator{});
}

inline bool IsActive(const RE::ActiveEffect* ef) { return ef && !ef->flags.any(RE::ActiveEffect::Flag::kInactive); }

inline auto GetActorActiveEffects(RE::Actor& actor)
{
	return SafeRange(actor.GetActiveEffectList()) | std::views::filter([](const RE::ActiveEffect* ae) { return IsActive(ae); });
}

struct TruePredicate
{
	template <class... TArgs>
	constexpr bool operator()(TArgs&&...) const noexcept
	{
		return true;
	}
};

template <std::invocable<const RE::TESBoundObject&> TFilter = TruePredicate>
auto GetEquippedItems(RE::Actor& actor, TFilter&& filter = TruePredicate{})
{
	return actor.GetInventory(std::forward<TFilter>(filter), false) | std::views::filter([](const auto& pair) {
		const auto& [_, invData] = pair;
		const auto& [count, entry] = invData;
		return count > 0 && entry->object && entry->IsWorn();
	}) | std::views::transform([](auto& pair) { return std::move(pair.second.second); });
}

inline void DispelByAdjustElapsedTime(RE::ActiveEffect* activeEffect)
{
	if (activeEffect) {
		activeEffect->elapsedSeconds = activeEffect->duration;
	}
}

inline float ChangeGlobalAtomic(RE::TESGlobal& global, float delta)
{
	static_assert(std::atomic_ref<float>::is_always_lock_free);

	SKSE::stl::atomic_ref value{ global.value };
	float expected = value.load(std::memory_order_relaxed);
	float desired;
	do {
		desired = expected + delta;
	} while (!value.compare_exchange_weak(expected, desired, std::memory_order_relaxed));

	return desired;
}
