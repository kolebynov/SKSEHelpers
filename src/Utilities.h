#pragma once

#include "GenericUtilities.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <atomic>
#include <concepts>
#include <ranges>
#include <utility>

inline bool IsActive(const RE::ActiveEffect* ef) { return ef && !ef->flags.any(RE::ActiveEffect::Flag::kInactive); }

inline auto GetActorActiveEffects(RE::Actor& actor)
{
	return SafeRange(actor.GetActiveEffectList()) | std::views::filter([](const RE::ActiveEffect* ae) { return IsActive(ae); });
}

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
