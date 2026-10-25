#pragma once

#include <cmath>
#include <concepts>
#include <ranges>
#include <type_traits>

template <std::floating_point T, T Epsilon = static_cast<T>(0.1)>
struct NearlyEqualComparer
{
	bool operator()(const T& a, const T& b) const { return std::abs(a - b) < Epsilon; }
};

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

struct TruePredicate
{
	template <class... TArgs>
	constexpr bool operator()(TArgs&&...) const noexcept
	{
		return true;
	}
};
